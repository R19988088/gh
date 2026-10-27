#include "gh_audio.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace gh::audio {
namespace {

bool IsValidFormat(AudioFormat format) noexcept {
  return format.sample_rate > 0 && format.channels >= 1 && format.channels <= 8;
}

float DecodeSample(const uint8_t* sample) noexcept {
  const uint16_t bits = static_cast<uint16_t>(sample[0]) |
                        (static_cast<uint16_t>(sample[1]) << 8);
  const int32_t value = bits < 0x8000 ? bits : static_cast<int32_t>(bits) - 0x10000;
  return value < 0 ? static_cast<float>(value) / 32768.0f
                   : static_cast<float>(value) / 32767.0f;
}

}  // namespace

DecodeResult DecodePacket(PacketCodec codec, std::span<const uint8_t> packet,
                          AudioFormat format, PcmFrames& output) {
  output = {};
  if (codec == PacketCodec::kXma2) return DecodeResult::kUnsupportedCodec;
  if (!IsValidFormat(format)) return DecodeResult::kInvalidFormat;
  std::vector<float> samples;
  const DecodeResult result = DecodePcm16LePacket(packet, format, samples);
  if (result != DecodeResult::kOk) return result;
  output.format = format;
  output.samples = std::move(samples);
  return DecodeResult::kOk;
}

DecodeResult DecodePcm16LePacket(std::span<const uint8_t> packet,
                                 AudioFormat format,
                                 std::vector<float>& output) {
  output.clear();
  if (!IsValidFormat(format)) return DecodeResult::kInvalidFormat;
  const size_t bytes_per_frame = static_cast<size_t>(format.channels) * 2;
  if (packet.empty() || packet.size() % bytes_per_frame != 0) {
    return DecodeResult::kInvalidPacket;
  }
  output.reserve(packet.size() / 2);
  for (size_t offset = 0; offset < packet.size(); offset += 2) {
    output.push_back(DecodeSample(packet.data() + offset));
  }
  return DecodeResult::kOk;
}

MixerQueue::MixerQueue(AudioFormat format, size_t capacity_frames)
    : format_(IsValidFormat(format) ? format : AudioFormat{}),
      capacity_frames_(capacity_frames) {
  if (format_.channels != 0 && capacity_frames_ != 0 && capacity_frames_ <=
                                   std::numeric_limits<size_t>::max() / format_.channels) {
    samples_.resize(capacity_frames_ * format_.channels);
  } else {
    format_ = {};
    capacity_frames_ = 0;
  }
}

bool MixerQueue::IsValidFormat(AudioFormat format) noexcept {
  return gh::audio::IsValidFormat(format);
}

bool MixerQueue::Configure(AudioFormat format) {
  if (!IsValidFormat(format) || capacity_frames_ == 0) return false;
  std::lock_guard lock(mutex_);
  if (queued_frames_ != 0 || capacity_frames_ >
                                  std::numeric_limits<size_t>::max() / format.channels) {
    return false;
  }
  format_ = format;
  samples_.assign(capacity_frames_ * format_.channels, 0.0f);
  read_frame_ = 0;
  return true;
}

AudioFormat MixerQueue::Format() const {
  std::lock_guard lock(mutex_);
  return format_;
}

bool MixerQueue::Enqueue(std::span<const float> frames) {
  std::lock_guard lock(mutex_);
  if (format_.channels == 0 || frames.size() % format_.channels != 0) return false;
  const size_t frame_count = frames.size() / format_.channels;
  if (frame_count > capacity_frames_ - queued_frames_) {
    stats_.overflow_frames += frame_count - (capacity_frames_ - queued_frames_);
    return false;
  }
  for (size_t frame = 0; frame < frame_count; ++frame) {
    const size_t target = ((read_frame_ + queued_frames_ + frame) % capacity_frames_) *
                          format_.channels;
    for (size_t channel = 0; channel < format_.channels; ++channel) {
      samples_[target + channel] = std::clamp(
          frames[frame * format_.channels + channel], -1.0f, 1.0f);
    }
  }
  queued_frames_ += frame_count;
  return true;
}

void MixerQueue::MixInto(std::span<float> output) {
  std::lock_guard lock(mutex_);
  if (format_.channels == 0 || output.size() % format_.channels != 0) return;
  const size_t requested_frames = output.size() / format_.channels;
  if (paused_) return;
  const size_t consumed_frames = std::min(requested_frames, queued_frames_);
  for (size_t frame = 0; frame < consumed_frames; ++frame) {
    const size_t source = ((read_frame_ + frame) % capacity_frames_) * format_.channels;
    for (size_t channel = 0; channel < format_.channels; ++channel) {
      const size_t target = frame * format_.channels + channel;
      output[target] = std::clamp(output[target] + samples_[source + channel], -1.0f, 1.0f);
    }
  }
  read_frame_ = (read_frame_ + consumed_frames) % capacity_frames_;
  queued_frames_ -= consumed_frames;
  if (requested_frames > consumed_frames) {
    stats_.underrun_frames += requested_frames - consumed_frames;
  }
}

void MixerQueue::SetPaused(bool paused) {
  std::lock_guard lock(mutex_);
  paused_ = paused;
}

bool MixerQueue::IsPaused() const {
  std::lock_guard lock(mutex_);
  return paused_;
}

size_t MixerQueue::QueuedFrames() const {
  std::lock_guard lock(mutex_);
  return queued_frames_;
}

MixerQueue::Stats MixerQueue::GetStats() const {
  std::lock_guard lock(mutex_);
  return stats_;
}

}  // namespace gh::audio

namespace gh::audio {
namespace {

// One output device exists, so the mix parameters are process state. The
// reader is the SDL device callback, which runs every 5.33 ms, so a plain
// mutex costs nothing and avoids a torn read across the four weights.
std::mutex g_mutex;
StereoFold g_fold = {};
SurroundMix g_mix = {};
float g_gain = 1.0f;

}  // namespace

void SetStereoFold(const StereoFold& fold) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_fold = fold;
}

StereoFold GetStereoFold() {
  std::lock_guard<std::mutex> lock(g_mutex);
  return g_fold;
}

void SetSurroundMix(const SurroundMix& mix) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_mix = mix;
}

SurroundMix GetSurroundMix() {
  std::lock_guard<std::mutex> lock(g_mutex);
  return g_mix;
}

void SetOutputGain(float linear) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_gain = linear;
}

float GetOutputGain() {
  std::lock_guard<std::mutex> lock(g_mutex);
  return g_gain;
}

}  // namespace gh::audio
