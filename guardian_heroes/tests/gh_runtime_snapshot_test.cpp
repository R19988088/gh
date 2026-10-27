#include "gh_system_runtime.h"
#include "gh_kernel.h"
#include "gh_threading.h"
#include "gh_system_xthread.h"
#include "gh_guest_continuation_title.h"
#include "native_memory.h"
#include <rex/chrono/clock.h>
#include <rex/stream.h>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdio>

using namespace std::chrono_literals;
using rex::X_STATUS;
namespace cont = gh::guest::continuation;
namespace {
constexpr uint32_t entry = 0x82000000;
uint32_t slots;
std::atomic<bool> finish{false};
gh::kernel::Event entered(true);
std::atomic<unsigned> entered_mask{0};
std::array<uint64_t, 2> saved_counters{};

void Original(GuestContext&, uint8_t*) { assert(false); }
cont::Transition Count(GuestContext& context, uint8_t* base, cont::Frame& frame) {
  const GuestMemory memory(base);
  const uint32_t address = context.r3.u32;
  const uint32_t index = (address - slots) / 32;
  assert(index < 2);
  if (frame.pc == entry) context.r20.u64 = 100 + index;
  if (finish.load()) {
    memory.Write64(address + 16, context.r20.u64);
    return cont::Transition::Return();
  }
  ++context.r20.u64;
  memory.Write64(address + 8, context.r20.u64);
  if ((entered_mask.fetch_or(1u << index) | (1u << index)) == 3) entered.Set();
  return cont::Transition::Next(entry + 4);
}

class Audio final : public rex::system::IAudioSystem {
 public:
  rex::system::KernelState* kernel = nullptr;
  uint32_t allocation = 0;
  bool paused = false, prepared = false, fail_restore = false;
  uint32_t value = 42;
  rex::X_STATUS Setup(rex::system::KernelState* state) override {
    kernel = state;
    allocation = kernel->memory()->SystemHeapAlloc(16);
    GuestMemory(kernel->memory()->virtual_membase()).Write32(allocation, value);
    return X_STATUS_SUCCESS;
  }
  void Shutdown() override {
    if (allocation) kernel->memory()->SystemHeapFree(allocation);
    allocation = 0;
  }
  void Pause() override { paused = true; }
  bool Resume() override { assert(paused); paused = false; return true; }
  bool Save(rex::stream::ByteStream* stream) override {
    assert(paused);
    const GuestMemory memory(kernel->memory()->virtual_membase());
    for (unsigned i = 0; i < 2; ++i) saved_counters[i] = memory.Read64(slots + i * 32 + 8);
    stream->Write(allocation);
    stream->Write(value);
    return true;
  }
  bool PrepareRestore() override {
    assert(paused);
    // Retire endpoint ownership before restoring the allocation map.
    if (allocation) kernel->memory()->SystemHeapFree(allocation);
    allocation = 0;
    prepared = true;
    return true;
  }
  bool Restore(rex::stream::ByteStream* stream) override {
    assert(paused && prepared);
    allocation = stream->Read<uint32_t>();
    value = stream->Read<uint32_t>();
    assert(GuestMemory(kernel->memory()->virtual_membase()).Read32(allocation) == value);
    if (fail_restore) return false;
    finish = true;  // First restored guest instruction returns its saved register.
    return true;
  }
};
class Graphics final : public rex::system::IGraphicsSystem {
 public:
  bool paused = false;
  uint64_t value = 1234;
  rex::X_STATUS SetupPresentation(rex::ui::WindowedAppContext*) override {
    return X_STATUS_SUCCESS;
  }
  rex::X_STATUS SetupGuestGpu(rex::runtime::FunctionDispatcher*, rex::system::KernelState*) override {
    return X_STATUS_SUCCESS;
  }
  bool has_presentation() const override { return false; }
  void Shutdown() override {}
  void Pause() override { paused = true; }
  void Resume() override { assert(paused); paused = false; }
  bool Save(rex::stream::ByteStream* stream) override {
    assert(paused); stream->Write(value); return true;
  }
  bool Restore(rex::stream::ByteStream* stream) override {
    assert(paused); value = stream->Read<uint64_t>(); return true;
  }
};
}

int main() {
  rex::Runtime runtime({});
  rex::RuntimeConfig config;
  auto graphics = std::make_unique<Graphics>();
  auto* gpu = graphics.get();
  config.graphics = std::move(graphics);
  Audio* audio = nullptr;
  config.audio_factory = [&](auto*) {
    auto result = std::make_unique<Audio>(); audio = result.get(); return result;
  };
  assert(runtime.Setup(std::move(config)) == X_STATUS_SUCCESS);
  auto* kernel = runtime.kernel_state();
  auto* memory = runtime.memory();
  assert(runtime.function_dispatcher()->InitializeFunctionTable(entry, 0x100, entry, 0x10000, true));
  assert(runtime.function_dispatcher()->SetFunction(entry, Original));
  cont::Program program;
  program.AddCompiled(entry, {entry, entry + 4}, Count);
  cont::InstallTitleProgram(&program);
  slots = memory->SystemHeapAlloc(64);
  memory->Fill(slots, 64, 0);
  std::array<rex::system::object_ref<rex::system::XThread>, 2> threads;
  std::array<rex::X_HANDLE, 2> handles;
  for (unsigned i = 0; i < 2; ++i) {
    threads[i] = rex::system::make_object<rex::system::XThread>(
        kernel, 16384, 0, entry, slots + i * 32, 0, true);
    handles[i] = threads[i]->handle();
    assert(threads[i]->Create() == X_STATUS_SUCCESS);
  }
  assert(entered.Wait(2s) == gh::kernel::WaitResult::Signaled);
  // Fixed-capacity output must fail before copying past the buffer, and
  // release the coordinated stop so both original hosts can continue.
  std::array<uint8_t, 256> short_buffer{};
  rex::stream::ByteStream short_output(short_buffer.data(), short_buffer.size());
  assert(!runtime.Save(&short_output));
  assert(short_output.offset() == 0 && !audio->paused && !gpu->paused);
  std::vector<uint8_t> snapshot(64 << 20);
  rex::stream::ByteStream output(snapshot.data(), snapshot.size());
  assert(runtime.Save(&output));
  assert(!audio->paused && !gpu->paused);
  const auto bytes = output.offset();
  const auto expected = saved_counters;
  gpu->value = 0;
  audio->value = 0;
  GuestMemory(memory->virtual_membase()).Write32(audio->allocation, 0);
  rex::stream::ByteStream input(snapshot.data(), bytes);
  assert(runtime.Restore(&input));
  assert(input.offset() == bytes && gpu->value == 1234 && audio->value == 42);
  assert(!audio->paused && !gpu->paused);
  for (unsigned i = 0; i < 2; ++i) {
    assert(!threads[i]->is_alive());
    const auto restored = kernel->object_table()->LookupObject<rex::system::XThread>(handles[i]);
    assert(restored && restored.get() != threads[i].get());
    uint64_t timeout = uint64_t(-20000000ll);
    assert(restored->Wait(0, 0, 0, &timeout) == X_STATUS_SUCCESS);
    restored->thread()->Join();
    assert(GuestMemory(memory->virtual_membase()).Read64(slots + i * 32 + 16) == expected[i]);
  }
  // A failed destructive restore keeps devices/timeline stopped; a subsequent
  // API call must not accidentally resume a partly restored allocation map.
  audio->fail_restore = true;
  input.set_offset(0);
  assert(!runtime.Restore(&input));
  assert(audio->paused && gpu->paused);
  rex::chrono::Clock::GuestState stopped;
  assert(rex::chrono::Clock::CaptureGuestState(stopped));
  assert(!runtime.Save(&output) && !runtime.Restore(&input));
  assert(audio->paused && gpu->paused);
  rex::chrono::Clock::GuestState unchanged;
  assert(rex::chrono::Clock::CaptureGuestState(unchanged));
  assert(stopped.tick_count == unchanged.tick_count);
  threads = {};
  runtime.Shutdown();
  cont::InstallTitleProgram(nullptr);
  // Shutdown owns recovery of its failed snapshot clock pause.
  assert(rex::chrono::Clock::PauseGuestClock());
  rex::chrono::Clock::ResumeGuestClock();
  std::puts("Runtime snapshot: two live threads, exact saved registers, heap/device ordering, "
            "fresh hosts and failed-restore paused boundary passed (test devices)");
}
