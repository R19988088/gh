// XMA sample conversion adapted from Xenia/ReXGlue under its BSD license.
#include "gh_audio_xma.h"

#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#if __has_include(<libavcodec/xmaframes_state.h>)
#include <libavcodec/xmaframes_state.h>
#define GH_XMA_FRAME_STATE_AVAILABLE 1
#endif
#include <libavutil/error.h>
}

namespace gh::audio {
namespace {

[[maybe_unused]] float ClampSample(float value) {
  const float lower = std::isgreater(value, -1.0f) ? value : -1.0f;
  return std::isless(lower, 1.0f) ? lower : 1.0f;
}

void ConvertFrame(const uint8_t** samples, bool is_two_channel,
                              uint8_t* output_buffer) {
  // Loop through every sample, convert and drop it into the output array.
  // If more than one channel, we need to interleave the samples from each
  // channel next to each other. Always saturate because FFmpeg output is
  // not limited to [-1, 1] (for example 1.095 as seen in 5454082B).
  constexpr float scale = (1 << 15) - 1;
  auto out = reinterpret_cast<int16_t*>(output_buffer);

  // For testing of vectorized versions, stereo audio is common in 4D5307E6,
  // since the first menu frame; the intro cutscene also has more than 2
  // channels.
#if defined(__x86_64__) || defined(_M_X64)
  static_assert(XmaFrameDecoder::kSamplesPerFrame % 8 == 0);
  const auto in_channel_0 = reinterpret_cast<const float*>(samples[0]);
  const __m128 scale_mm = _mm_set1_ps(scale);
  if (is_two_channel) {
    const auto in_channel_1 = reinterpret_cast<const float*>(samples[1]);
    const __m128i shufmask = _mm_set_epi8(14, 15, 6, 7, 12, 13, 4, 5, 10, 11, 2, 3, 8, 9, 0, 1);
    for (uint32_t i = 0; i < XmaFrameDecoder::kSamplesPerFrame; i += 4) {
      // Load 8 samples, 4 for each channel.
      __m128 in_mm0 = _mm_loadu_ps(&in_channel_0[i]);
      __m128 in_mm1 = _mm_loadu_ps(&in_channel_1[i]);
      // Rescale.
      in_mm0 = _mm_mul_ps(in_mm0, scale_mm);
      in_mm1 = _mm_mul_ps(in_mm1, scale_mm);
      // Cast to int32.
      __m128i out_mm0 = _mm_cvtps_epi32(in_mm0);
      __m128i out_mm1 = _mm_cvtps_epi32(in_mm1);
      // Saturated cast and pack to int16.
      __m128i out_mm = _mm_packs_epi32(out_mm0, out_mm1);
      // Interleave channels and byte swap.
      out_mm = _mm_shuffle_epi8(out_mm, shufmask);
      // Store, as [out + i * 4] movdqu.
      _mm_storeu_si128(reinterpret_cast<__m128i*>(&out[i * 2]), out_mm);
    }
  } else {
    const __m128i shufmask = _mm_set_epi8(14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1);
    for (uint32_t i = 0; i < XmaFrameDecoder::kSamplesPerFrame; i += 8) {
      // Load 8 samples, as [in_channel_0 + i * 4] and
      // [in_channel_0 + i * 4 + 16] movups.
      __m128 in_mm0 = _mm_loadu_ps(&in_channel_0[i]);
      __m128 in_mm1 = _mm_loadu_ps(&in_channel_0[i + 4]);
      // Rescale.
      in_mm0 = _mm_mul_ps(in_mm0, scale_mm);
      in_mm1 = _mm_mul_ps(in_mm1, scale_mm);
      // Cast to int32.
      __m128i out_mm0 = _mm_cvtps_epi32(in_mm0);
      __m128i out_mm1 = _mm_cvtps_epi32(in_mm1);
      // Saturated cast and pack to int16.
      __m128i out_mm = _mm_packs_epi32(out_mm0, out_mm1);
      // Byte swap.
      out_mm = _mm_shuffle_epi8(out_mm, shufmask);
      // Store, as [out + i * 2] movdqu.
      _mm_storeu_si128(reinterpret_cast<__m128i*>(&out[i]), out_mm);
    }
  }
#else
  uint32_t o = 0;
  for (uint32_t i = 0; i < XmaFrameDecoder::kSamplesPerFrame; i++) {
    for (uint32_t j = 0; j <= uint32_t(is_two_channel); j++) {
      // Select the appropriate array based on the current channel.
      auto in = reinterpret_cast<const float*>(samples[j]);

      // Raw samples sometimes aren't within [-1, 1]
      float scaled_sample = ClampSample(in[i]) * scale;

      // Convert the sample and output it in big endian.
      auto sample = static_cast<int16_t>(scaled_sample);
      out[o++] = std::byteswap(sample);
    }
  }
#endif
}


}  // namespace

class XmaFrameDecoder::Impl {
 public:
  ~Impl() {
    avcodec_free_context(&context);
    av_frame_free(&frame);
    av_packet_free(&packet);
  }
  bool Fail(const char* operation, int code = 0) {
    error = operation;
    if (code) {
      char detail[AV_ERROR_MAX_STRING_SIZE];
      av_strerror(code, detail, sizeof(detail));
      error += ": ";
      error += detail;
    }
    return false;
  }
  AVCodec* codec = nullptr;
  AVCodecContext* context = nullptr;
  AVFrame* frame = nullptr;
  AVPacket* packet = nullptr;
  std::string error;
};

XmaFrameDecoder::XmaFrameDecoder() : impl_(std::make_unique<Impl>()) {}
XmaFrameDecoder::~XmaFrameDecoder() = default;

bool XmaFrameDecoder::Initialize() {
  auto& self = *impl_;
  if (self.codec && self.frame && self.packet) return true;
  self.codec = avcodec_find_decoder(AV_CODEC_ID_XMAFRAMES);
  if (!self.codec) return self.Fail("XMAFRAMES codec not found");
  if (!self.frame) self.frame = av_frame_alloc();
  if (!self.packet) self.packet = av_packet_alloc();
  if (!self.frame || !self.packet) return self.Fail("XMA decoder allocation failed");
  return true;
}

int XmaFrameDecoder::Prepare(int sample_rate, bool stereo) {
  auto& self = *impl_;
  if (!Initialize()) return -1;
  if (sample_rate != 24000 && sample_rate != 32000 && sample_rate != 44100 && sample_rate != 48000) {
    self.Fail("Unsupported XMA sample rate");
    return -1;
  }
  const int channels = stereo ? 2 : 1;
  if (self.context && self.context->sample_rate == sample_rate &&
      self.context->channels == channels) return 0;
  avcodec_free_context(&self.context);
  self.context = avcodec_alloc_context3(self.codec);
  if (!self.context) { self.Fail("XMA context allocation failed"); return -1; }
  self.context->sample_rate = sample_rate;
  self.context->channels = channels;
  self.context->flags2 |= AV_CODEC_FLAG2_SKIP_MANUAL;
  const int result = avcodec_open2(self.context, self.codec, nullptr);
  if (result < 0) {
    self.Fail("Opening XMA context failed", result);
    avcodec_free_context(&self.context);
    return -1;
  }
  return 1;
}

void XmaFrameDecoder::Reset() {
  // XMAFRAMES has no flush callback; reopening removes the old MDCT overlap.
  if (impl_->context) {
    impl_->context->sample_rate = 0;
    impl_->context->channels = 0;
  }
}

bool XmaFrameDecoder::Decode(std::span<const uint8_t> packet,
                              std::span<uint8_t> pcm16_big_endian) {
  auto& self = *impl_;
  self.error.clear();
  if (!self.context || !self.context->channels || packet.empty() ||
      packet.size() > static_cast<size_t>(std::numeric_limits<int>::max()) ||
      pcm16_big_endian.size() < kSamplesPerFrame * self.context->channels * sizeof(int16_t)) {
    return self.Fail("Invalid XMA frame or output buffer");
  }
  av_packet_unref(self.packet);
  const int allocated = av_new_packet(self.packet, static_cast<int>(packet.size()));
  if (allocated < 0) return self.Fail("XMA packet allocation failed", allocated);
  std::memcpy(self.packet->data, packet.data(), packet.size());
  int result = avcodec_send_packet(self.context, self.packet);
  if (result < 0) return self.Fail("Sending XMA packet failed", result);
  result = avcodec_receive_frame(self.context, self.frame);
  if (result == AVERROR(EAGAIN)) return false;
  if (result < 0) return self.Fail("Receiving XMA frame failed", result);
  if (self.frame->format != AV_SAMPLE_FMT_FLTP ||
      self.frame->nb_samples < static_cast<int>(kSamplesPerFrame)) {
    return self.Fail("Unexpected XMA decoded sample format");
  }
  const uint8_t* channels[] = {self.frame->data[0], self.frame->data[1]};
  ConvertFrame(channels, self.context->channels == 2, pcm16_big_endian.data());
  return true;
}

const std::string& XmaFrameDecoder::LastError() const { return impl_->error; }

std::vector<uint8_t> XmaFrameDecoder::SaveState() const {
#if defined(GH_XMA_FRAME_STATE_AVAILABLE)
  FFXmaFramesState state{};
  state.version = 1;
  if (impl_->context && impl_->context->channels &&
      ff_xmaframes_save_state(impl_->context, &state) < 0) {
    impl_->Fail("Could not capture XMA frame decoder state");
    return {};
  }
  std::vector<uint8_t> bytes(sizeof(state));
  std::memcpy(bytes.data(), &state, sizeof(state));
  return bytes;
#else
  impl_->Fail("XMA frame state requires the project FFmpeg decoder");
  return {};
#endif
}

bool XmaFrameDecoder::RestoreState(std::span<const uint8_t> bytes) {
#if defined(GH_XMA_FRAME_STATE_AVAILABLE)
  if (bytes.size() != sizeof(FFXmaFramesState)) return impl_->Fail("Invalid XMA decoder state size");
  FFXmaFramesState state;
  std::memcpy(&state, bytes.data(), sizeof(state));
  if (state.version != 1 || state.channels < 0 || state.channels > 2 ||
      (!state.channels && state.sample_rate)) return impl_->Fail("Invalid XMA decoder state");
  XmaFrameDecoder replacement;
  if (!replacement.Initialize()) return impl_->Fail(replacement.LastError().c_str());
  if (state.channels) {
    if (replacement.Prepare(state.sample_rate, state.channels == 2) < 0)
      return impl_->Fail(replacement.LastError().c_str());
    const auto result = ff_xmaframes_restore_state(replacement.impl_->context, &state);
    if (result < 0) return impl_->Fail("Restoring XMA frame decoder state failed", result);
  }
  impl_.swap(replacement.impl_);
  return true;
#else
  (void)bytes;
  return impl_->Fail("XMA frame state requires the project FFmpeg decoder");
#endif
}

}  // namespace gh::audio
