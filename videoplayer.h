
#pragma once

#include <functional>
#include <memory>
#include <string>

struct VideoContext;

enum class BinkColorSpace { Jpeg, Mpeg };

class VideoPlayer {
 public:
  // Format is ALWAYS RGBx
  using OnVideoFrame = std::function<void(double time, uint8_t* pixelData, int size, int stride)>;
  using OnAudioSamples = std::function<void(float* planes[], int sampleCount, bool interleaved)>;
  using OnStop = std::function<void()>;

  VideoPlayer() noexcept;
  virtual ~VideoPlayer() noexcept;

  bool open(const char* path) noexcept;

  void play(OnVideoFrame onVideoFrame, OnAudioSamples onAudioSamples, OnStop onStop) noexcept;
  void stop() noexcept;

  bool atEnd() const noexcept;

  const std::string& error() const noexcept;

  bool hasVideo() const noexcept;
  int width() const noexcept;
  int height() const noexcept;

  /**
   * @return True if this video has audio data.
   */
  bool hasAudio() const noexcept;

  /**
   * @return The audio sample rate in samples per second.
   */
  int audioSampleRate() const noexcept;

  /**
   * @return The number of audio channels, guaranteed to be 1 or 2.
   */
  int audioChannels() const noexcept;

 private:
  std::unique_ptr<VideoContext> video;
};
