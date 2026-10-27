#include "gh_audio_runtime.h"
#include "gh_xma_packet.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <thread>
#include <vector>
using namespace std::chrono_literals;
using namespace gh::audio;
namespace {
uint32_t Bits(const uint8_t* bytes, size_t offset, size_t count) {
  uint32_t result = 0;
  for (size_t i = 0; i < count; ++i)
    result = (result << 1) | ((bytes[(offset+i)/8] >> (7-(offset+i)%8)) & 1);
  return result;
}
struct Guest {
  std::vector<uint8_t> bytes = std::vector<uint8_t>(0x40000);
  uint8_t* At(uint32_t address) { assert(address < bytes.size()); return bytes.data()+address; }
  GuestAudioMemory Memory() { return {[this](uint32_t a) { return At(a); },
      [this](uint32_t a) { return At(a); }, {}, {}}; }
};
void TestContext(const char* bank_path) {
  Guest guest;
  std::ifstream bank(bank_path, std::ios::binary);
  assert(bank);
  bank.seekg(8192);
  bank.read(reinterpret_cast<char*>(guest.At(0x10000)), 4096);
  assert(bank.gcount() == 4096);
  XmaFrameDecoder reference;
  assert(reference.Initialize() && reference.Prepare(44100, true) == 1);
  std::array<std::array<uint8_t, 2048>, 8> decoded;
  std::vector<std::vector<uint8_t>> assembled_frames;
  std::vector<std::vector<uint8_t>> decoder_states;
  std::vector<uint32_t> offsets;
  auto* packet = guest.At(0x10000);
  size_t packet_index = 0;
  size_t offset = 32 + Bits(packet, 6, 15);
  for (auto& frame : decoded) {
    offsets.push_back(uint32_t(offset));
    const size_t bits = Bits(packet+packet_index*2048, offset, 15);
    const size_t leading = offset%8;
    const size_t bytes = (leading+bits+7)/8;
    const size_t trailing = bytes*8-leading-bits;
    std::vector<uint8_t> payload;
    payload.insert(payload.end(), packet+packet_index*2048+4, packet+(packet_index+1)*2048);
    payload.insert(payload.end(), packet+(packet_index+1)*2048+4, packet+(packet_index+2)*2048);
    std::vector<uint8_t> assembled(bytes+1);
    assembled[0] = uint8_t((leading<<5) | (trailing<<2));
    std::memcpy(assembled.data()+1, payload.data()+(offset-32)/8, bytes);
    assembled[1] &= uint8_t(0xFF >> leading);
    assembled.back() &= uint8_t(0xFF << trailing);
    assert(reference.Decode(assembled, frame));
    decoder_states.push_back(reference.SaveState());
    assembled_frames.push_back(assembled);
    offset += bits;
    if (offset >= 2048*8) {
      ++packet_index;
      offset = 32 + Bits(packet+packet_index*2048, 6, 15);
    }
  }
  // Each checkpoint must resume in a fresh codec without replaying earlier
  // packets. Compare every later sample against the uninterrupted decoder.
  bool history_changes_pcm = false;
  for (size_t checkpoint = 0; checkpoint + 1 < decoded.size(); ++checkpoint) {
    XmaFrameDecoder restored;
    assert(restored.RestoreState(decoder_states[checkpoint]));
    assert(!restored.RestoreState({decoder_states[checkpoint].data(), 1}));
    std::array<uint8_t, 2048> actual;
    for (size_t next = checkpoint + 1; next < decoded.size(); ++next) {
      assert(restored.Decode(assembled_frames[next], actual));
      assert(actual == decoded[next]);
    }
    XmaFrameDecoder without_history;
    assert(without_history.Prepare(44100, true) == 1);
    assert(without_history.Decode(assembled_frames[checkpoint + 1], actual));
    history_changes_pcm |= actual != decoded[checkpoint + 1];
  }
  assert(history_changes_pcm && "fixture must distinguish restored overlap from a reset decoder");
  // The context's carry path aligns each frame's 320-sample tail with the
  // following frame's 192-sample head. The independent byte extraction above
  // also checks the project's bit cursor against the actual packet encoding.
  std::vector<uint8_t> expected;
  for (size_t i = 0; i < 3; ++i) {
    expected.insert(expected.end(), decoded[i].begin()+768, decoded[i].end());
    expected.insert(expected.end(), decoded[i+1].begin(), decoded[i+1].begin()+768);
  }
  XmaContext context;
  assert(context.Setup(0, guest.Memory(), 0x100) == 0);
  auto configure = [&](uint32_t read, uint32_t write, uint32_t blocks, uint32_t decode) {
    std::memset(guest.At(0x100), 0, 64);
    XmaContextData data(guest.At(0x100));
    data.input_buffer_0_ptr = 0x10000;
    data.input_buffer_0_packet_count = 1;
    data.input_buffer_0_valid = 1;
    data.input_buffer_read_offset = 32;
    data.output_buffer_ptr = 0x20000;
    data.output_buffer_valid = 1;
    data.output_buffer_block_count = blocks;
    data.output_buffer_read_offset = read;
    data.output_buffer_write_offset = write;
    data.sample_rate = 2;
    data.is_stereo = 1;
    data.subframe_decode_count = decode;
    data.Store(guest.At(0x100));
    context.set_is_allocated(true);
  };
  configure(0, 0, 31, 8);
  context.Enable(); assert(context.Work());
  assert(!context.Work());
  XmaContextData data(guest.At(0x100));
  assert(data.output_buffer_write_offset == 24);
  assert(std::equal(expected.begin(), expected.end(), guest.At(0x20000)));
  assert(data.input_buffer_read_offset == offsets[4]);
  context.Clear();
  data = XmaContextData(guest.At(0x100));
  assert(!data.input_buffer_0_valid && !data.output_buffer_valid);
  assert(data.input_buffer_read_offset == 32 && data.output_buffer_write_offset == 0);

  configure(20, 20, 31, 8);
  context.Enable(); assert(context.Work());
  data = XmaContextData(guest.At(0x100));
  assert(data.output_buffer_write_offset == 13);
  for (size_t i = 0; i < expected.size(); ++i)
    assert(guest.At(0x20000)[(20*256+i)%(31*256)] == expected[i]);
  context.Clear();

  // Traverse a frame spanning two guest input buffers and merge status back
  // without losing the second buffer's validity or its bit cursor.
  configure(0, 0, 31, 8);
  data = XmaContextData(guest.At(0x100));
  data.input_buffer_1_ptr = 0x10800;
  data.input_buffer_1_packet_count = 1;
  data.input_buffer_1_valid = 1;
  data.Store(guest.At(0x100));
  context.Enable(); assert(context.Work());
  data = XmaContextData(guest.At(0x100));
  data.output_buffer_read_offset = data.output_buffer_write_offset;
  data.Store(guest.At(0x100));
  const auto split_state = context.SaveState();
  const auto split_memory = guest.bytes;
  context.Enable(); assert(context.Work());
  const auto split_result = guest.bytes;
  guest.bytes = split_memory;
  assert(context.RestoreState(split_state));
  context.Enable(); assert(context.Work());
  assert(guest.bytes == split_result);
  data = XmaContextData(guest.At(0x100));
  assert(!data.input_buffer_0_valid && data.input_buffer_1_valid && data.current_buffer == 1);
  assert(data.input_buffer_read_offset == offsets[7] && data.output_buffer_write_offset == 17);
  std::vector<uint8_t> split_expected;
  for (size_t i = 3; i < 6; ++i) {
    split_expected.insert(split_expected.end(), decoded[i].begin()+768, decoded[i].end());
    split_expected.insert(split_expected.end(), decoded[i+1].begin(), decoded[i+1].begin()+768);
  }
  for (size_t i = 0; i < split_expected.size(); ++i)
    assert(guest.At(0x20000)[(24*256+i)%(31*256)] == split_expected[i]);
  context.Clear();

  // One loop: end on frame 2/subframe 1, then discard the whole start frame
  // as warm-up. Decode history remains continuous across the wrap.
  reference.Reset();
  assert(reference.Prepare(44100, true) == 1);
  const unsigned order[] = {0, 1, 2, 0, 1, 2};
  std::array<uint8_t, 2048> previous{}, current{};
  std::vector<uint8_t> loop_expected;
  for (size_t step = 0; step < std::size(order); ++step) {
    assert(reference.Decode(assembled_frames[order[step]], current));
    if (step && step != 4) {
      std::array<uint8_t, 2048> aligned;
      std::memcpy(aligned.data(), previous.data()+768, 1280);
      std::memcpy(aligned.data()+1280, current.data(), 768);
      const size_t output_bytes = step == 3 ? 1024 : 2048;
      loop_expected.insert(loop_expected.end(), aligned.begin(), aligned.begin()+output_bytes);
    }
    previous = current;
  }
  configure(0, 0, 31, 8);
  data = XmaContextData(guest.At(0x100));
  data.loop_count = 1;
  data.loop_start = offsets[0];
  data.loop_end = offsets[2];
  data.loop_subframe_end = 1;
  data.loop_subframe_skip = 4;
  data.Store(guest.At(0x100));
  context.Enable(); assert(context.Work());
  data = XmaContextData(guest.At(0x100));
  assert(data.loop_count == 0 && data.output_buffer_write_offset == 28);
  assert(std::equal(loop_expected.begin(), loop_expected.end(), guest.At(0x20000)));
  context.Clear();

  // Keep a frame partially consumed while crossing loop end/start. Restoring
  // both host context and guest bytes must reproduce ring bytes and bit cursor.
  configure(0, 0, 3, 1);
  data = XmaContextData(guest.At(0x100));
  data.loop_count = 2;
  data.loop_start = offsets[0];
  data.loop_end = offsets[2];
  data.loop_subframe_end = 1;
  data.loop_subframe_skip = 4;
  data.Store(guest.At(0x100));
  context.Enable(); assert(context.Work());
  for (unsigned checkpoint = 0; checkpoint < 12; ++checkpoint) {
    const auto state = context.SaveState();
    const auto before = guest.bytes;
    auto advance = [&] {
      XmaContextData cursor(guest.At(0x100));
      cursor.output_buffer_read_offset = cursor.output_buffer_write_offset;
      cursor.output_buffer_valid = 1;
      cursor.Store(guest.At(0x100));
      context.Enable();
      assert(context.Work());
    };
    advance();
    const auto after = guest.bytes;
    guest.bytes = before;
    assert(context.RestoreState(state));
    advance();
    assert(guest.bytes == after);
  }
  context.Clear();

  // Force a partial frame, then drain it through the input-free consume path.
  configure(0, 0, 3, 1);
  context.Enable(); assert(context.Work());
  assert(std::equal(expected.begin(), expected.begin()+768, guest.At(0x20000)));
  data = XmaContextData(guest.At(0x100));
  data.input_buffer_0_packet_count = 0;
  data.input_buffer_1_packet_count = 0;
  data.output_buffer_read_offset = 0;
  data.Store(guest.At(0x100));
  context.Enable(); assert(context.Work());
  assert(std::equal(expected.begin()+768, expected.begin()+1024, guest.At(0x20000)));
  context.Release();
  assert(std::all_of(guest.At(0x100), guest.At(0x100)+64, [](uint8_t b) { return !b; }));
}
void TestXmaRuntime() {
  Guest guest;
  XmaRuntime runtime;
  assert(runtime.Initialize(guest.Memory(), 0x1000, 0x1000));
  assert(runtime.context_array_ptr() == 0x1000);
  std::vector<uint32_t> contexts;
  for (uint32_t i = 0; i < XmaRuntime::kContextCount; ++i) contexts.push_back(runtime.AllocateContext());
  assert(contexts.front() == 0x1000 && contexts.back() == 0x5FC0);
  assert(runtime.AllocateContext() == 0);
  runtime.ReleaseContext(contexts[3]);
  assert(runtime.AllocateContext() == contexts[3]);
  assert(std::byteswap(runtime.ReadRegister(0x1818)) == 1);
  assert(std::byteswap(runtime.ReadRegister(0x1818)) == 2);
  // All groups including context 319 use project MMIO state and context locks.
  runtime.WriteRegister(0x1940+9*4, std::byteswap(0x80000000u));
  runtime.WriteRegister(0x1A40+9*4, std::byteswap(0x80000000u));
  assert(runtime.BlockOnContext(contexts.back(), false));
  runtime.Pause();
  assert(runtime.paused());
  XmaRuntime::State state;
  assert(runtime.SaveState(state));
  runtime.ReleaseContext(contexts[3]);
  assert(std::byteswap(runtime.ReadRegister(0x1818)) == 3);
  assert(runtime.RestoreState(state));
  assert(runtime.AllocateContext() == 0);
  assert(std::byteswap(runtime.ReadRegister(0x1818)) == 3);
  auto invalid = state;
  invalid.contexts[0].decoder.resize(1);
  assert(!runtime.RestoreState(invalid));
  runtime.Stop(); // Shutdown must wake a paused worker before joining it.
}
void TestClients() {
  ClientScheduler clients(4);
  std::atomic<unsigned> dispatched{0}, submitted{0}, closed{0}, freed{0};
  bool endpoint_paused = false;
  bool can_resume = true;
  std::vector<uint8_t> endpoint_state{1, 7, 13};
  assert(clients.Attach(0, {123, 456, 789}, {
      [&](uint32_t samples) { assert(samples == 0xABC); ++submitted; },
      [&] { ++closed; }, [&] { ++freed; },
      [&] { endpoint_paused = true; return true; },
      [&] { endpoint_paused = !can_resume; return can_resume; },
      [&](auto& state) { assert(endpoint_paused); state = endpoint_state; return true; },
      [&](auto state) { assert(endpoint_paused); endpoint_state.assign(state.begin(), state.end()); return true; }}));
  clients.Start();
  std::thread worker([&] { clients.Run([&](size_t index, ClientScheduler::Callback cb) {
    assert(index == 0 && cb.function == 123 && cb.argument == 456 && cb.wrapped_argument == 789);
    assert(clients.Submit(index, 0xABC));
    ++dispatched;
  }); });
  auto until = [&](unsigned count) {
    const auto deadline = std::chrono::steady_clock::now()+2s;
    while (dispatched.load() < count && std::chrono::steady_clock::now() < deadline)
      std::this_thread::yield();
    assert(dispatched == count);
  };
  until(4);
  ClientScheduler::State state;
  assert(!clients.SaveState(state));
  clients.Pause();
  assert(clients.FrameConsumed(0));
  assert(clients.FrameConsumed(0));
  assert(clients.SaveState(state) && state.clients[0].credits == 2);
  endpoint_state = {99};
  assert(clients.FrameConsumed(0));
  assert(clients.RestoreState(state));
  assert(endpoint_state == std::vector<uint8_t>({1, 7, 13}));
  auto invalid = state; invalid.clients[0].callback.wrapped_argument = 1;
  assert(!clients.RestoreState(invalid));
  assert(dispatched == 4);
  can_resume = false;
  assert(!clients.Resume() && clients.paused() && endpoint_paused);
  assert(clients.SaveState(state) && state.clients[0].credits == 2);
  can_resume = true;
  clients.Resume();
  until(6);
  clients.Detach(0);
  assert(closed == 1 && freed == 1 && submitted == 6);
  assert(clients.Attach(0, {}, {{}, [&] { ++closed; }, [&] { ++freed; }}));
  clients.Pause();
  clients.Stop();
  worker.join();
  clients.CloseAll(false);
  assert(closed == 2 && freed == 1);
}
}  // namespace
int main(int argc, char** argv) {
  assert(argc == 2);
  TestContext(argv[1]);
  TestXmaRuntime();
  TestClients();
  std::puts("Audio runtime: exact XMA decoder checkpoints, carry/subframe/loop and "
            "cross-buffer restore; MMIO/pool, endpoint/credit restore and pause/shutdown passed");
}
