
#include "videoplayer.h"

#define RESULT_TRUE 1
#define RESULT_FALSE 0

extern "C" {

VideoPlayer *BinkPlayer_Create() noexcept {
    return new VideoPlayer;
}

void BinkPlayer_Free(VideoPlayer *player) noexcept {
    delete player;
}

int BinkPlayer_Open(VideoPlayer *player, char *path) noexcept {
    return player->open(path) ? RESULT_TRUE : RESULT_FALSE;
}

using OnVideoFrame = void(double time, uint8_t *rgb32Data, int size, int stride);
using OnAudioSamples = void(float *planes[], int sampleCount, bool interleaved);
using OnStop = void();

void BinkPlayer_Play(VideoPlayer *player,
                     OnVideoFrame *onVideoFrame,
                     OnAudioSamples *onAudioSamples,
                     OnStop *onStop) noexcept {
    player->play(onVideoFrame, onAudioSamples, onStop);
}

void BinkPlayer_Stop(VideoPlayer *player) noexcept {
    player->stop();
}

int BinkPlayer_AtEnd(VideoPlayer *player) noexcept {
    return player->atEnd() ? RESULT_TRUE : RESULT_FALSE;
}

const char *BinkPlayer_Error(VideoPlayer *player) noexcept {
    return player->error().c_str();
}

int BinkPlayer_HasVideo(VideoPlayer *player) noexcept {
    return player->hasVideo() ? RESULT_TRUE : RESULT_FALSE;
}

int BinkPlayer_Width(VideoPlayer *player) noexcept {
    return player->width();
}

int BinkPlayer_Height(VideoPlayer *player) noexcept {
    return player->height();
}

int BinkPlayer_HasAudio(VideoPlayer *player) noexcept {
    return player->hasAudio() ? RESULT_TRUE : RESULT_FALSE;
}

int BinkPlayer_AudioSampleRate(VideoPlayer *player) noexcept {
    return player->audioSampleRate();
}

int BinkPlayer_AudioChannels(VideoPlayer *player) noexcept {
    return player->audioChannels();
}

}
