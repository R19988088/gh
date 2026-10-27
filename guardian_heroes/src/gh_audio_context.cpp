// XMA context execution derived from Xenia, Copyright 2021 Ben Vanik.
// Released under the BSD license; see rexglue-sdk-main/LICENSE.
#include "gh_audio_context.h"
#include "gh_xma_packet.h"
#include <algorithm>
#include <cassert>
#include <format>

namespace gh::audio {
namespace {
// Packet bit extraction retains the original byte padding convention used by
// XMAFRAMES, while reading only bytes within the packet span.
class BitStream {
 public:
  BitStream(uint8_t* bytes, size_t bits) : bytes_(bytes), size_(bits) {}
  size_t offset_bits() const { return offset_; }
  void SetOffset(size_t value) { assert(value <= size_); offset_ = std::min(value, size_); }
  size_t BitsRemaining() const { return size_ - offset_; }
  void Advance(size_t bits) { SetOffset(offset_ + bits); }
  uint64_t Peek(size_t bits) const {
    assert(bits <= 57 && bits <= BitsRemaining());
    uint64_t word = 0;
    const size_t byte = offset_ / 8;
    const size_t available = std::min(size_t{8}, (size_ + 7) / 8 - byte);
    for (size_t i = 0; i < available; ++i) word |= uint64_t(bytes_[byte+i]) << (56-i*8);
    return (word >> (64 - (offset_ % 8 + bits))) & ((uint64_t{1} << bits)-1);
  }
  uint64_t Read(size_t bits) { auto value = Peek(bits); Advance(bits); return value; }
  size_t Copy(uint8_t* out, size_t bits) {
    const size_t padding = offset_ % 8;
    const size_t byte = offset_ / 8;
    size_t left = bits, written = 0;
    if (padding) {
      assert(left >= 8-padding);
      out[written++] = uint8_t(Peek(8-padding));
      left -= 8-padding;
      Advance(8-padding);
    }
    if (left >= 8) {
      std::memcpy(out+written, bytes_+byte+written, left/8);
      written += left/8;
      Advance(left/8*8);
      left %= 8;
    }
    if (left) { out[written] = uint8_t(Peek(left) << (8-left)); Advance(left); }
    return padding;
  }
 private:
  uint8_t* bytes_;
  size_t size_, offset_ = 0;
};
}  // namespace

void OutputRing::Write(const uint8_t* data, size_t count) {
  count = std::min(count, size_t(capacity_));
  if (!count) return;
  assert(count <= write_count());
  const size_t first = std::min(count, size_t(capacity_ - write_));
  std::memcpy(buffer_ + write_, data, first);
  std::memcpy(buffer_, data + first, count - first);
  write_ = (write_ + uint32_t(count)) % capacity_;
}
}  // namespace gh::audio

namespace gh::audio {



const uint32_t XmaContext::kBitsPerPacketHeader;
const uint32_t XmaContext::kOutputMaxSizeBytes;

XmaContext::XmaContext() = default;

XmaContext::~XmaContext() = default;

int XmaContext::Setup(uint32_t id, GuestAudioMemory memory, uint32_t guest_ptr) {
  id_ = id;
  memory_ = std::move(memory);
  guest_ptr_ = guest_ptr;

  if (!decoder_.Initialize()) {
    Log(std::format("XmaContext {}: {}", id_, decoder_.LastError()));
    return 1;
  }
  set_is_allocated(false);
  set_is_enabled(false);
  ResetDecoderState();
  work_completion_event_.Reset();
  return 0;
}

bool XmaContext::Work() {
  if (!is_allocated() || !is_enabled()) {
    return false;
  }

  std::lock_guard<std::mutex> lock(lock_);
  // Inline MMIO kicks and the background sweep may both observe an enabled
  // context before either acquires the lock. Consume each enable only once.
  if (!is_allocated() || !is_enabled()) return false;
  set_is_enabled(false);

  auto context_ptr = memory_.virtual_address(guest_ptr());
  XmaContextData data(context_ptr);
  const XmaContextData initial_data = data;

  if (!data.output_buffer_valid) {
    return true;
  }

  OutputRing output_rb = PrepareOutputRingBuffer(&data);

  // Consume-only context: no input, just drain remaining subframes.
  if (data.IsConsumeOnlyContext()) {
    if (current_frame_remaining_subframes_ == 0) {
      return true;
    }
    Consume(&output_rb, &data);
    data.output_buffer_write_offset = output_rb.write_offset() / kOutputBytesPerBlock;
    StoreContextMerged(data, initial_data, context_ptr);
    return true;
  }

  // Minimum free blocks needed before attempting a decode.
  // Use subframe_decode_count (clamped to 1) instead of full frame size.
  const uint32_t effective_sdc = std::max(static_cast<uint32_t>(1), data.subframe_decode_count);
  const int32_t minimum_subframe_decode_count =
      static_cast<int32_t>(effective_sdc) + data.output_buffer_padding;

  if (minimum_subframe_decode_count > remaining_subframe_blocks_in_output_buffer_) {
    StoreContextMerged(data, initial_data, context_ptr);
    return true;
  }

  while (remaining_subframe_blocks_in_output_buffer_ >= minimum_subframe_decode_count &&
         (!memory_.cancelled || !memory_.cancelled())) {
    Decode(&data);
    Consume(&output_rb, &data);

    if (!data.IsAnyInputBufferValid() || data.error_status == 4) {
      break;
    }
  }

  data.output_buffer_write_offset = output_rb.write_offset() / kOutputBytesPerBlock;

  if (output_rb.empty()) {
    data.output_buffer_valid = 0;
  }

  StoreContextMerged(data, initial_data, context_ptr);
  return true;
}

void XmaContext::Enable() {
  std::lock_guard<std::mutex> lock(lock_);
  set_is_enabled(true);
}

bool XmaContext::Block(bool poll) {
  if (!lock_.try_lock()) {
    if (poll) {
      return false;
    }
    lock_.lock();
  }
  lock_.unlock();
  return true;
}

void XmaContext::Clear() {
  std::lock_guard<std::mutex> lock(lock_);
  Log(std::format("XmaContext: reset context {}", id()));

  auto context_ptr = memory_.virtual_address(guest_ptr());
  XmaContextData data(context_ptr);
  ClearLocked(&data);
  data.Store(context_ptr);
}

void XmaContext::ClearLocked(XmaContextData* data) {
  data->input_buffer_0_valid = 0;
  data->input_buffer_1_valid = 0;
  data->output_buffer_valid = 0;

  data->input_buffer_read_offset = kBitsPerPacketHeader;
  data->output_buffer_read_offset = 0;
  data->output_buffer_write_offset = 0;

  ResetDecoderState();
}

void XmaContext::ResetDecoderState() {
  decoder_.Reset();
  raw_frame_.fill(0);
  decoded_frame_.fill(0);
  carry_frame_.fill(0);
  carry_valid_ = false;
  pending_output_limit_ = 0;
  pending_start_skip_ = 0;
  current_frame_remaining_subframes_ = 0;
  loop_frame_output_limit_ = 0;
  loop_start_skip_pending_ = false;
}

void XmaContext::Disable() {
  std::lock_guard<std::mutex> lock(lock_);
  set_is_enabled(false);
}

void XmaContext::Release() {
  std::lock_guard<std::mutex> lock(lock_);
  assert(is_allocated());

  set_is_allocated(false);
  ResetDecoderState();
  auto context_ptr = memory_.virtual_address(guest_ptr());
  std::memset(context_ptr, 0, sizeof(XmaContextData));
}

XmaContext::State XmaContext::SaveState() {
  std::lock_guard lock(lock_);
  State state;
  state.id = id_;
  state.guest_ptr = guest_ptr_;
  state.allocated = is_allocated();
  state.enabled = is_enabled();
  state.completion_signaled = work_completion_event_.signaled();
  state.decoder = decoder_.SaveState();
  state.raw_frame = raw_frame_;
  state.carry_frame = carry_frame_;
  state.remaining_subframe_blocks = remaining_subframe_blocks_in_output_buffer_;
  state.remaining_subframes = current_frame_remaining_subframes_;
  state.loop_output_limit = loop_frame_output_limit_;
  state.loop_start_skip_pending = loop_start_skip_pending_;
  state.carry_valid = carry_valid_;
  state.pending_output_limit = pending_output_limit_;
  state.pending_start_skip = pending_start_skip_;
  return state;
}

bool XmaContext::RestoreState(const State& state) {
  std::lock_guard lock(lock_);
  if (state.version != 1 || state.id != id_ || state.guest_ptr != guest_ptr_ ||
      state.remaining_subframes > 8 || state.loop_output_limit > 8 ||
      state.pending_output_limit > 8 || state.pending_start_skip > 8 ||
      !decoder_.RestoreState(state.decoder)) return false;
  raw_frame_ = state.raw_frame;
  carry_frame_ = state.carry_frame;
  remaining_subframe_blocks_in_output_buffer_ = state.remaining_subframe_blocks;
  current_frame_remaining_subframes_ = state.remaining_subframes;
  loop_frame_output_limit_ = state.loop_output_limit;
  loop_start_skip_pending_ = state.loop_start_skip_pending;
  carry_valid_ = state.carry_valid;
  pending_output_limit_ = state.pending_output_limit;
  pending_start_skip_ = state.pending_start_skip;
  set_is_allocated(state.allocated);
  set_is_enabled(state.enabled);
  if (state.completion_signaled) work_completion_event_.Set();
  else work_completion_event_.Reset();
  return true;
}

void XmaContext::SwapInputBuffer(XmaContextData* data) {
  if (data->current_buffer == 0) {
    data->input_buffer_0_valid = 0;
  } else {
    data->input_buffer_1_valid = 0;
  }
  data->current_buffer ^= 1;
  data->input_buffer_read_offset = kBitsPerPacketHeader;
}

void XmaContext::UpdateLoopStatus(XmaContextData* data) {
  if (data->loop_count == 0) {
    return;
  }

  const uint32_t loop_start = std::max(kBitsPerPacketHeader, data->loop_start);
  const uint32_t loop_end = std::max(kBitsPerPacketHeader, data->loop_end);

  if (data->input_buffer_read_offset != loop_end) {
    return;
  }

  data->input_buffer_read_offset = loop_start;
  loop_start_skip_pending_ = true;

  if (data->loop_count < 255) {
    data->loop_count--;
  }
}

int XmaContext::GetSampleRate(int id) {
  return kIdToSampleRate[std::min(id, 3)];
}

int16_t XmaContext::GetPacketNumber(size_t size, size_t bit_offset) {
  if (bit_offset < kBitsPerPacketHeader) {
    assert(false);
    return -1;
  }
  if (bit_offset >= (size << 3)) {
    assert(false);
    return -1;
  }
  size_t byte_offset = bit_offset >> 3;
  size_t packet_number = byte_offset / kBytesPerPacket;
  return static_cast<int16_t>(packet_number);
}

uint32_t XmaContext::GetCurrentInputBufferSize(XmaContextData* data) {
  return data->GetCurrentInputBufferPacketCount() * kBytesPerPacket;
}

uint8_t* XmaContext::GetCurrentInputBuffer(XmaContextData* data) {
  return memory_.physical_address(data->GetCurrentInputBufferAddress());
}

uint32_t XmaContext::GetAmountOfBitsToRead(uint32_t remaining_stream_bits, uint32_t frame_size) {
  return std::min(remaining_stream_bits, frame_size);
}

const uint8_t* XmaContext::GetNextPacket(XmaContextData* data, uint32_t next_packet_index,
                                         uint32_t current_input_packet_count) {
  if (next_packet_index < current_input_packet_count) {
    return memory_.physical_address(data->GetCurrentInputBufferAddress()) +
           next_packet_index * kBytesPerPacket;
  }

  const uint8_t next_buffer_index = data->current_buffer ^ 1;
  if (!data->IsInputBufferValid(next_buffer_index)) {
    return nullptr;
  }

  const uint32_t next_buffer_address = data->GetInputBufferAddress(next_buffer_index);
  if (!next_buffer_address) {
    Log(std::format("XmaContext {}: Buffer marked valid but has null pointer!", id()));
    return nullptr;
  }

  return memory_.physical_address(next_buffer_address);
}

uint32_t XmaContext::GetNextPacketReadOffset(uint8_t* buffer, uint32_t next_packet_index,
                                             uint32_t current_input_packet_count) {
  while (next_packet_index < current_input_packet_count) {
    uint8_t* next_packet = buffer + (next_packet_index * kBytesPerPacket);
    const uint32_t packet_frame_offset = xma::GetPacketFrameOffset(next_packet);

    if (packet_frame_offset <= kMaxFrameSizeinBits) {
      return (next_packet_index * kBitsPerPacket) + packet_frame_offset;
    }
    next_packet_index++;
  }

  return kBitsPerPacketHeader;
}

OutputRing XmaContext::PrepareOutputRingBuffer(XmaContextData* data) {
  const uint32_t output_capacity = data->output_buffer_block_count * kOutputBytesPerBlock;
  const uint32_t output_read_offset = data->output_buffer_read_offset * kOutputBytesPerBlock;
  const uint32_t output_write_offset = data->output_buffer_write_offset * kOutputBytesPerBlock;

  if (output_capacity > kOutputMaxSizeBytes) {
    Log(std::format(
        "XmaContext {}: Output buffer exceeds expected size! "
        "(Actual: {} Max: {})",
        id(), output_capacity, kOutputMaxSizeBytes));
  }

  uint8_t* output_buffer = memory_.physical_address(data->output_buffer_ptr);

  OutputRing output_rb(output_buffer, output_capacity);
  output_rb.set_read_offset(output_read_offset);
  output_rb.set_write_offset(output_write_offset);
  remaining_subframe_blocks_in_output_buffer_ =
      static_cast<int32_t>(output_rb.write_count()) / kOutputBytesPerBlock;

  return output_rb;
}

kPacketInfo XmaContext::GetPacketInfo(uint8_t* packet, uint32_t frame_offset) {
  kPacketInfo packet_info = {};

  const uint32_t first_frame_offset = xma::GetPacketFrameOffset(packet);
  BitStream stream(packet, kBitsPerPacket);
  stream.SetOffset(first_frame_offset);

  if (frame_offset < first_frame_offset) {
    packet_info.current_frame_ = 0;
    packet_info.current_frame_size_ = first_frame_offset - frame_offset;
  }

  while (true) {
    if (stream.BitsRemaining() < kBitsPerFrameHeader) {
      break;
    }

    const uint64_t frame_size = stream.Peek(kBitsPerFrameHeader);
    if (frame_size == 0 || frame_size == xma::kMaxFrameLength) {
      break;
    }

    if (stream.offset_bits() == frame_offset) {
      packet_info.current_frame_ = packet_info.frame_count_;
      packet_info.current_frame_size_ = static_cast<uint32_t>(frame_size);
    }

    packet_info.frame_count_++;

    if (frame_size > stream.BitsRemaining()) {
      break;
    }

    stream.Advance(frame_size - 1);

    if (stream.Read(1) == 0) {
      break;
    }
  }

  if (xma::IsPacketXma2Type(packet)) {
    const uint8_t xma2_frame_count = xma::GetPacketFrameCount(packet);
    if (xma2_frame_count > packet_info.frame_count_) {
      if (packet_info.current_frame_size_ == 0) {
        packet_info.current_frame_ = packet_info.frame_count_;
      }
      packet_info.frame_count_ = xma2_frame_count;
    }
  }
  return packet_info;
}

void XmaContext::StoreContextMerged(const XmaContextData& data,
                                    const XmaContextData& initial_data, uint8_t* context_ptr) {
  XmaContextData fresh(context_ptr);

  fresh.loop_count = data.loop_count;
  fresh.output_buffer_write_offset = data.output_buffer_write_offset;
  if (initial_data.input_buffer_0_valid && !data.input_buffer_0_valid) {
    fresh.input_buffer_0_valid = 0;
  }
  if (initial_data.input_buffer_1_valid && !data.input_buffer_1_valid) {
    fresh.input_buffer_1_valid = 0;
  }

  if (initial_data.output_buffer_valid && !data.output_buffer_valid) {
    fresh.output_buffer_valid = 0;
  }

  fresh.input_buffer_read_offset = data.input_buffer_read_offset;
  fresh.error_status = data.error_status;
  fresh.current_buffer = data.current_buffer;
  fresh.output_buffer_read_offset = data.output_buffer_read_offset;

  fresh.Store(context_ptr);
}

void XmaContext::Consume(OutputRing* output_rb, const XmaContextData* data) {
  if (!current_frame_remaining_subframes_) {
    return;
  }

  if (loop_frame_output_limit_ > 0) {
    const uint8_t total_subframes = (kBytesPerFrameChannel / kOutputBytesPerBlock)
                                    << data->is_stereo;
    const uint8_t consumed = total_subframes - current_frame_remaining_subframes_;
    if (consumed >= loop_frame_output_limit_) {
      remaining_subframe_blocks_in_output_buffer_ -= data->output_buffer_padding;
      current_frame_remaining_subframes_ = 0;
      loop_frame_output_limit_ = 0;
      return;
    }
  }

  const uint8_t effective_sdc = std::max(static_cast<uint32_t>(1), data->subframe_decode_count);
  int8_t subframes_to_write = std::min(static_cast<int8_t>(current_frame_remaining_subframes_),
                                       static_cast<int8_t>(effective_sdc));

  if (loop_frame_output_limit_ > 0) {
    const uint8_t total_subframes = (kBytesPerFrameChannel / kOutputBytesPerBlock)
                                    << data->is_stereo;
    const uint8_t consumed = total_subframes - current_frame_remaining_subframes_;
    const int8_t remaining_until_limit = static_cast<int8_t>(loop_frame_output_limit_ - consumed);
    if (subframes_to_write > remaining_until_limit) {
      subframes_to_write = remaining_until_limit;
    }
  }

  const int8_t raw_frame_read_offset =
      ((kBytesPerFrameChannel / kOutputBytesPerBlock) << data->is_stereo) -
      current_frame_remaining_subframes_;

  output_rb->Write(raw_frame_.data() + (kOutputBytesPerBlock * raw_frame_read_offset),
                   subframes_to_write * kOutputBytesPerBlock);

  const int8_t headroom = (current_frame_remaining_subframes_ - subframes_to_write == 0)
                              ? data->output_buffer_padding
                              : 0;

  remaining_subframe_blocks_in_output_buffer_ -= subframes_to_write + headroom;
  current_frame_remaining_subframes_ -= subframes_to_write;
}

int XmaContext::PrepareDecoder(int sample_rate, bool is_two_channel) {
  return decoder_.Prepare(GetSampleRate(sample_rate), is_two_channel);
}

void XmaContext::PreparePacket(uint32_t frame_size, uint32_t frame_padding) {
  prepared_packet_size_ = 1 + ((frame_padding + frame_size) / 8) +
                         (((frame_padding + frame_size) % 8) ? 1 : 0);
  auto padding_end = prepared_packet_size_ * 8 - (8 + frame_padding + frame_size);
  assert(padding_end < 8);
  xma_frame_[0] = ((frame_padding & 7) << 5) | ((padding_end & 7) << 2);
}

void XmaContext::Decode(XmaContextData* data) {
  

  if (!data->IsAnyInputBufferValid()) {
    return;
  }

  if (current_frame_remaining_subframes_ > 0) {
    return;
  }

  if (!data->IsCurrentInputBufferValid()) {
    SwapInputBuffer(data);
    if (!data->IsCurrentInputBufferValid()) {
      return;
    }
  }

  uint8_t* current_input_buffer = GetCurrentInputBuffer(data);

  input_buffer_.fill(0);

  // Loop-end frame: decode it here (output limited to loop_subframe_end),
  // jump to loop_start afterwards in the next-offset step.
  bool is_loop_end_frame = false;
  if (data->loop_count > 0) {
    const uint32_t loop_end = std::max(kBitsPerPacketHeader, data->loop_end);
    is_loop_end_frame = (data->input_buffer_read_offset == loop_end);
  }

  if (!data->output_buffer_block_count) {
    Log(std::format("XmaContext {}: Error - Received 0 for output_buffer_block_count!", id()));
    return;
  }

  if (data->input_buffer_read_offset < kBitsPerPacketHeader) {
    data->input_buffer_read_offset = kBitsPerPacketHeader;
  }

  const uint32_t current_input_size = GetCurrentInputBufferSize(data);
  const uint32_t current_input_packet_count = current_input_size / kBytesPerPacket;

  const int16_t packet_index = GetPacketNumber(current_input_size, data->input_buffer_read_offset);

  if (packet_index == -1) {
    Log(std::format("XmaContext {}: Invalid packet index. Input read offset: {}", id(),
                 static_cast<uint32_t>(data->input_buffer_read_offset)));
    return;
  }

  uint8_t* packet = current_input_buffer + (packet_index * kBytesPerPacket);
  const uint32_t packet_first_frame_offset = xma::GetPacketFrameOffset(packet);
  uint32_t relative_offset = data->input_buffer_read_offset % kBitsPerPacket;

  if (relative_offset < packet_first_frame_offset) {
    data->input_buffer_read_offset = (packet_index * kBitsPerPacket) + packet_first_frame_offset;
    relative_offset = packet_first_frame_offset;
  }

  const uint8_t skip_count = xma::GetPacketSkipCount(packet);

  // Full packet skip (0xFF) -- no new frames begin in this packet.
  if (skip_count == 0xFF) {
    uint32_t next_input_offset =
        GetNextPacketReadOffset(current_input_buffer, packet_index + 1, current_input_packet_count);
    if (next_input_offset == kBitsPerPacketHeader) {
      SwapInputBuffer(data);
    }
    data->input_buffer_read_offset = next_input_offset;
    return;
  }

  kPacketInfo packet_info = GetPacketInfo(packet, relative_offset);
  const uint32_t packet_to_skip = skip_count + 1;
  const uint32_t next_packet_index = packet_index + packet_to_skip;

  // Frame header split across packet boundary.
  if (packet_info.current_frame_size_ == 0) {
    const uint8_t* next_packet = GetNextPacket(data, next_packet_index, current_input_packet_count);
    if (!next_packet) {
      SwapInputBuffer(data);
      return;
    }
    std::memcpy(input_buffer_.data(), packet + kBytesPerPacketHeader, kBytesPerPacketData);
    std::memcpy(input_buffer_.data() + kBytesPerPacketData, next_packet + kBytesPerPacketHeader,
                kBytesPerPacketData);

    BitStream combined(input_buffer_.data(), (kBitsPerPacket - kBitsPerPacketHeader) * 2);
    combined.SetOffset(relative_offset - kBitsPerPacketHeader);

    uint64_t frame_size = combined.Peek(kBitsPerFrameHeader);
    if (frame_size == xma::kMaxFrameLength) {
      data->error_status = 4;
      return;
    }
    packet_info.current_frame_size_ = static_cast<uint32_t>(frame_size);
  }

  BitStream stream(current_input_buffer, (packet_index + 1) * kBitsPerPacket);
  stream.SetOffset(data->input_buffer_read_offset);

  const uint64_t bits_to_copy = GetAmountOfBitsToRead(static_cast<uint32_t>(stream.BitsRemaining()),
                                                      packet_info.current_frame_size_);

  if (bits_to_copy == 0) {
    Log(std::format("XmaContext {}: There are no bits to copy!", id()));
    SwapInputBuffer(data);
    return;
  }

  if (packet_info.isLastFrameInPacket()) {
    if (stream.BitsRemaining() < packet_info.current_frame_size_) {
      const uint8_t* next_packet =
          GetNextPacket(data, next_packet_index, current_input_packet_count);
      if (!next_packet) {
        data->error_status = 4;
        return;
      }
      std::memcpy(input_buffer_.data() + kBytesPerPacketData, next_packet + kBytesPerPacketHeader,
                  kBytesPerPacketData);
    }
  }

  std::memcpy(input_buffer_.data(), packet + kBytesPerPacketHeader, kBytesPerPacketData);

  stream = BitStream(input_buffer_.data(), (kBitsPerPacket - kBitsPerPacketHeader) * 2);
  stream.SetOffset(relative_offset - kBitsPerPacketHeader);

  xma_frame_.fill(0);

  const uint32_t padding_start =
      static_cast<uint8_t>(stream.Copy(xma_frame_.data() + 1, packet_info.current_frame_size_));

  decoded_frame_.fill(0);

  const int prepared = PrepareDecoder(data->sample_rate, bool(data->is_stereo));
  if (prepared == 1) {
    // Reopening the codec restarts its output; the carried tail belongs to the
    // old stream.
    carry_valid_ = false;
  }
  PreparePacket(packet_info.current_frame_size_, padding_start);
  const bool decoded = prepared >= 0 && decoder_.Decode(
      {xma_frame_.data(), prepared_packet_size_}, decoded_frame_);
  if (!decoded && !decoder_.LastError().empty()) {
    Log(std::format("XmaContext {}: {}", id_, decoder_.LastError()));
  }
  if (decoded) {
    // Realign the decoder's output onto the bitstream's sample numbering: the
    // samples of the frame just decoded run from kDecoderStartPadding into this
    // block and finish in the head of the next one.
    const size_t pad_bytes = kDecoderStartPadding * kBytesPerSample << data->is_stereo;
    const size_t carry_bytes = (kBytesPerFrameChannel << data->is_stereo) - pad_bytes;

    // Loop end: limit output to subframes 0..loop_subframe_end.
    const uint8_t decoded_output_limit =
        is_loop_end_frame ? static_cast<uint8_t>((data->loop_subframe_end + 1) << data->is_stereo)
                          : 0;
    // Loop start: skip leading subframes per loop_subframe_skip. skip == 4
    // means the whole frame is a warm-up frame (frame-aligned loop start):
    // decode seeds the codec state, output is fully discarded.
    const uint8_t decoded_start_skip =
        loop_start_skip_pending_ ? static_cast<uint8_t>(data->loop_subframe_skip << data->is_stereo)
                                 : 0;
    loop_start_skip_pending_ = false;

    if (carry_valid_) {
      std::memcpy(raw_frame_.data(), carry_frame_.data(), carry_bytes);
      std::memcpy(raw_frame_.data() + carry_bytes, decoded_frame_.data(), pad_bytes);

      current_frame_remaining_subframes_ = 4 << data->is_stereo;
      loop_frame_output_limit_ = pending_output_limit_;
      current_frame_remaining_subframes_ -=
          std::min(pending_start_skip_, current_frame_remaining_subframes_);
    }

    std::memcpy(carry_frame_.data(), decoded_frame_.data() + pad_bytes, carry_bytes);
    carry_valid_ = true;
    pending_output_limit_ = decoded_output_limit;
    pending_start_skip_ = decoded_start_skip;
  } else {
    // A dropped frame breaks the carry's adjacency; re-prime rather than splice
    // two blocks that are not neighbors.
    carry_valid_ = false;
  }

  // Compute where to go next.
  if (is_loop_end_frame) {
    UpdateLoopStatus(data);
    return;
  }

  if (!packet_info.isLastFrameInPacket()) {
    const uint32_t next_frame_offset =
        (data->input_buffer_read_offset + bits_to_copy) % kBitsPerPacket;
    data->input_buffer_read_offset = (packet_index * kBitsPerPacket) + next_frame_offset;
    return;
  }

  uint32_t next_input_offset =
      GetNextPacketReadOffset(current_input_buffer, next_packet_index, current_input_packet_count);

  if (next_input_offset == kBitsPerPacketHeader) {
    SwapInputBuffer(data);
    if (data->IsAnyInputBufferValid()) {
      next_input_offset = xma::GetPacketFrameOffset(
          memory_.physical_address(data->GetCurrentInputBufferAddress()));

      if (next_input_offset > kMaxFrameSizeinBits) {
        SwapInputBuffer(data);
        return;
      }
    }
  }
  data->input_buffer_read_offset = next_input_offset;
}

}  // namespace gh::audio
