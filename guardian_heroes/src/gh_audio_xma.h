#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace gh::audio {

// Decodes the assembled XMAFRAMES bitstream, including its leading padding byte.
// XmaContext owns guest packet traversal, loop control and the output ring.
class XmaFrameDecoder {
 public:
  static constexpr size_t kSamplesPerFrame = 512;

  XmaFrameDecoder();
  ~XmaFrameDecoder();
  XmaFrameDecoder(const XmaFrameDecoder&) = delete;
  XmaFrameDecoder& operator=(const XmaFrameDecoder&) = delete;

  bool Initialize();
  // Returns 1 when reopening discarded decoder history, 0 for an unchanged
  // stream, and -1 on an unsupported format or codec failure.
  int Prepare(int sample_rate, bool stereo);
  void Reset();
  bool Decode(std::span<const uint8_t> packet, std::span<uint8_t> pcm16_big_endian);
  std::vector<uint8_t> SaveState() const;
  bool RestoreState(std::span<const uint8_t> state);
  const std::string& LastError() const;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace gh::audio
