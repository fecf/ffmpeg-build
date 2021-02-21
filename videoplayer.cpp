
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/log.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#include <chrono>
#include <mutex>
#include <thread>
#include <utility>
#include <condition_variable>
#include "videoplayer.h"

struct StreamContext {
    int index;
    AVCodecContext *codec_ctx;
    AVFrame *frame = av_frame_alloc();

    StreamContext(int index, AVCodecContext *codecCtx) : index(index), codec_ctx(codecCtx) {}

    ~StreamContext() {
        avcodec_free_context(&codec_ctx);
        av_frame_free(&frame);
    }
};

using std::chrono::steady_clock;

struct VideoContext {
    std::string path;
    std::string error;
    bool at_end = false;

    AVFormatContext *format_ctx{};

    const AVStream *video_stream_info{};
    const AVStream *audio_stream_info{};

    // We have to convert from YUV420p to RGB32 for easier display in skia,
    // ffmpeg has assembly routines for this, so we expect this to be much faster
    SwsContext *resample_ctx{};
    std::unique_ptr<uint8_t[]> rgb_frame;
    int rgb_frame_stride;
    int rgb_frame_size;

    std::unique_ptr<StreamContext> video_stream;
    std::unique_ptr<StreamContext> audio_stream;

    std::mutex mutex;
    bool playing{};
    std::condition_variable stop_condition;
    std::thread decoding_thread;

    ~VideoContext() noexcept {
        stop();

        if (format_ctx) {
            avformat_free_context(format_ctx);
        }

        if (resample_ctx) {
            sws_freeContext(resample_ctx);
        }
    }

    void setError(int errorCode) noexcept {
        char errString[AV_ERROR_MAX_STRING_SIZE]{0};
        this->error.assign(av_make_error_string(errString, AV_ERROR_MAX_STRING_SIZE, errorCode));
    }

    std::unique_ptr<StreamContext> create_stream_context(AVCodecParameters *codec_params, int index) noexcept {
        auto codec = avcodec_find_decoder(codec_params->codec_id);
        if (!codec) {
            this->error =
                    std::string("Failed to find decoder for ") + std::to_string(codec_params->codec_tag);
            return nullptr;
        }

        auto codec_ctx = avcodec_alloc_context3(codec);
        if (!codec_ctx) {
            this->error = "Failed to allocate codec context.";
            return nullptr;
        }

        auto result = std::make_unique<StreamContext>(index, codec_ctx);

        int err;
        if ((err = avcodec_parameters_to_context(codec_ctx, codec_params)) < 0) {
            this->setError(err);
            return nullptr;
        }

        if ((err = avcodec_open2(codec_ctx, codec, nullptr)) < 0) {
            this->setError(err);
            return nullptr;
        }

        return result;
    }

    void play(VideoPlayer::OnVideoFrame onVideoFrame, VideoPlayer::OnAudioSamples onAudioSamples,
              VideoPlayer::OnStop onStop) noexcept;

    void stop() noexcept;

private:
    void run_decode_loop(VideoPlayer::OnVideoFrame onVideoFrame,
                         VideoPlayer::OnAudioSamples onAudioSamples, VideoPlayer::OnStop onStop) noexcept;
};

VideoPlayer::VideoPlayer() noexcept {
    video = std::make_unique<VideoContext>();

    video->format_ctx = avformat_alloc_context();
}

VideoPlayer::~VideoPlayer() noexcept = default;

bool VideoPlayer::open(const char *path) noexcept {
    video->path = path;

    int err;
    if ((err = avformat_open_input(&video->format_ctx, path, nullptr, nullptr)) != 0) {
        video->setError(err);
        return false;
    }

    if ((err = avformat_find_stream_info(video->format_ctx, nullptr)) < 0) {
        avformat_close_input(&video->format_ctx);
        video->setError(err);
        return false;
    }

    for (int i = 0; i < (int) video->format_ctx->nb_streams; i++) {
        auto codec_params = video->format_ctx->streams[i]->codecpar;

        // specific for video and audio
        if (codec_params->codec_type == AVMEDIA_TYPE_VIDEO) {
            video->video_stream = std::move(video->create_stream_context(codec_params, i));
            video->video_stream_info = video->format_ctx->streams[i];
        } else if (codec_params->codec_type == AVMEDIA_TYPE_AUDIO) {
            video->audio_stream = std::move(video->create_stream_context(codec_params, i));
            video->audio_stream_info = video->format_ctx->streams[i];
        } else {
            continue;
        }
    }

    // Create the YUV420->RGB resampler
    if (video->video_stream_info) {
        auto width = video->video_stream_info->codecpar->width;
        auto height = video->video_stream_info->codecpar->height;
        video->resample_ctx = sws_getContext(video->video_stream->codec_ctx->width,
                                             video->video_stream->codec_ctx->height,
                                             video->video_stream->codec_ctx->pix_fmt,
                                             width, height, AV_PIX_FMT_RGB32,
                                             0, nullptr, nullptr, nullptr);

        // Allocate space for the RGB32 frame
        video->rgb_frame_stride = width * 4;
        video->rgb_frame_size = video->rgb_frame_stride * height;
        video->rgb_frame = std::make_unique<uint8_t[]>(video->rgb_frame_size);
    }

    return true;
}

void VideoContext::play(VideoPlayer::OnVideoFrame onVideoFrame,
                        VideoPlayer::OnAudioSamples onAudioSamples, VideoPlayer::OnStop onStop) noexcept {
    stop();

    playing = true;
    decoding_thread = std::thread(
            [this, onVideoFrame{std::move(onVideoFrame)}, onAudioSamples{std::move(onAudioSamples)},
                    onStop{std::move(onStop)}]() { run_decode_loop(onVideoFrame, onAudioSamples, onStop); });
}

void VideoContext::run_decode_loop(VideoPlayer::OnVideoFrame onVideoFrame,
                                   VideoPlayer::OnAudioSamples onAudioSamples,
                                   VideoPlayer::OnStop onStop) noexcept {
    std::unique_lock<std::mutex> ul(mutex, std::defer_lock);

    auto start_time = steady_clock::now();

    auto video_time_base = video_stream_info ? av_q2d(video_stream_info->time_base) : 1;

    AVPacket packet;
    while (!at_end) {
        // Check if stop was requested
        {
            ul.lock();
            if (!playing) {
                at_end = true;
                ul.unlock();
                break;
            }
            ul.unlock();
        }

        // Process packets until we got an unprocessed frame
        if (av_read_frame(format_ctx, &packet) < 0) {
            at_end = true;
            break;
        }

        auto stream_index = packet.stream_index;

        // Always process audio data immediately
        if (audio_stream && stream_index == audio_stream->index) {
            if ((avcodec_send_packet(audio_stream->codec_ctx, &packet)) == 0) {
                while ((avcodec_receive_frame(audio_stream->codec_ctx, audio_stream->frame)) >= 0) {
                    auto frame = audio_stream->frame;

                    // The format is either float interleaved or planar
                    auto interleaved = frame->format == AV_SAMPLE_FMT_FLT;

                    auto samples = reinterpret_cast<float **>(frame->data);
                    onAudioSamples(samples, frame->nb_samples, interleaved);
                }
            }
        }

        // Ignore packets not for the video stream
        if (!video_stream || stream_index != video_stream->index) {
            std::this_thread::yield();
            continue;
        }

        // Compute current video time
        auto now = steady_clock::now();
        auto packet_time = video_time_base * packet.pts;
        auto presentation_time = start_time + std::chrono::duration<double>(packet_time);

        if (presentation_time > now) {
            ul.lock();
            stop_condition.wait_until(ul, presentation_time, [this] { return !playing; });
            if (!playing) {
                ul.unlock();
                break;
            }
            ul.unlock();
        }

        if ((avcodec_send_packet(video_stream->codec_ctx, &packet)) == 0) {
            auto frame = video_stream->frame;
            while ((avcodec_receive_frame(video_stream->codec_ctx, frame)) >= 0) {
                // Resample YUV420->RGB32
                uint8_t *destData = rgb_frame.get();

                // sws expects us to provide arrays for planes
                uint8_t * destPlaneData[1] = { destData }; // RGB32 has one plane
                int destPlaneStrides[1] = { rgb_frame_stride }; // RGB stride

                sws_scale(resample_ctx,
                        // Input data
                          frame->data, frame->linesize,
                        // Input slice (full frame)
                          0, frame->height,
                        // Destination
                          destPlaneData, destPlaneStrides);

                onVideoFrame(packet_time, destData, rgb_frame_size, rgb_frame_stride);
            }
        }
    }

    onStop();
}

void VideoContext::stop() noexcept {
    {
        std::lock_guard<std::mutex> lg(mutex);
        playing = false;
        stop_condition.notify_all();
    }
    if (decoding_thread.joinable()) {
        decoding_thread.join();
    }
}

const std::string &VideoPlayer::error() const noexcept { return video->error; }

bool VideoPlayer::atEnd() const noexcept { return video->at_end; }

bool VideoPlayer::hasAudio() const noexcept { return video->audio_stream_info != nullptr; }

int VideoPlayer::audioSampleRate() const noexcept {
    auto stream_info = video->audio_stream_info;
    if (stream_info) {
        return stream_info->codecpar->sample_rate;
    }
    return 0;
}

int VideoPlayer::audioChannels() const noexcept {
    auto stream_info = video->audio_stream_info;
    if (stream_info) {
        return stream_info->codecpar->channels;
    }
    return 0;
}

bool VideoPlayer::hasVideo() const noexcept { return video->video_stream_info != nullptr; }

int VideoPlayer::width() const noexcept {
    auto stream_info = video->video_stream_info;
    if (stream_info) {
        return stream_info->codecpar->width;
    }
    return 0;
}

int VideoPlayer::height() const noexcept {
    auto stream_info = video->video_stream_info;
    if (stream_info) {
        return stream_info->codecpar->height;
    }
    return 0;
}

void VideoPlayer::play(VideoPlayer::OnVideoFrame onVideoFrame,
                       VideoPlayer::OnAudioSamples onAudioSamples, VideoPlayer::OnStop onStop) noexcept {
    video->play(std::move(onVideoFrame), std::move(onAudioSamples), std::move(onStop));
}

void VideoPlayer::stop() noexcept { video->stop(); }
