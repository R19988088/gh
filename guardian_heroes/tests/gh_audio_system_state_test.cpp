#include "gh_audio_driver.h"
#include "gh_audio_decoder.h"
#include "gh_audio_sdl.h"
#include "gh_audio_system.h"
#include <rex/runtime.h>
#include <rex/stream.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#define CHECK(condition) do { if (!(condition)) { \
  std::fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #condition); \
  std::abort(); \
} } while (false)

using namespace gh::audio;

namespace {
class FixtureDriver final : public AudioDriver {
 public:
  FixtureDriver(rex::memory::Memory* memory, std::function<void()> consumed, bool& fail)
      : AudioDriver(memory), output({}, consumed, {}), fail(fail), on_consumed(std::move(consumed)) {}
  void SubmitFrame(uint32_t pointer) override {
    CHECK(output.Submit({memory_->TranslateVirtual<float*>(pointer), SdlOutput::kFrameSamples}));
  }
  bool Pause() override { return output.Pause(); }
  bool Resume() override { return output.Resume(); }
  bool SaveState(std::vector<uint8_t>& state) override { return output.SaveState(state); }
  bool RestoreState(std::span<const uint8_t> state) override {
    return !fail && output.RestoreState(state);
  }
  SdlOutput output;
  bool& fail;
  std::function<void()> on_consumed;
};

class FixtureAudio final : public AudioSystem {
 public:
  explicit FixtureAudio(rex::runtime::FunctionDispatcher* dispatcher) : AudioSystem(dispatcher) {}
  ~FixtureAudio() override { Shutdown(); }
  ClientScheduler::State State() {
    ClientScheduler::State state; CHECK(clients_.SaveState(state)); return state;
  }
  void ClearCredits() {
    auto state = State(); state.clients[0].credits = 0;
    CHECK(clients_.RestoreState(state));
  }
  std::array<FixtureDriver*, ClientScheduler::kClientCount> drivers{};
  bool fail_restore = false;
 protected:
  rex::X_STATUS CreateDriver(size_t index, std::function<void()> consumed, AudioDriver** driver) override {
    auto* endpoint = new FixtureDriver(memory_, std::move(consumed), fail_restore);
    if (is_paused()) CHECK(endpoint->Pause());
    drivers[index] = endpoint; *driver = endpoint; return X_STATUS_SUCCESS;
  }
  void DestroyDriver(AudioDriver* driver) override {
    for (auto& endpoint : drivers) if (endpoint == driver) endpoint = nullptr;
    delete driver;
  }
};
}

int main() {
  rex::Runtime runtime({});
  rex::RuntimeConfig config; config.tool_mode = true;
  FixtureAudio* audio = nullptr;
  config.audio_factory = [&](auto* dispatcher) {
    auto fixture = std::make_unique<FixtureAudio>(dispatcher); audio = fixture.get(); return fixture;
  };
  CHECK(runtime.Setup(std::move(config)) == X_STATUS_SUCCESS && runtime.audio_system() == audio);
  auto* memory = runtime.memory();
  const auto allocated = [&](uint32_t address) {
    uint32_t size = 0; CHECK(memory->LookupHeap(address)->QuerySize(address, &size)); return size;
  };
  size_t client = 99;
  CHECK(audio->RegisterClient(0, 0x12345678, &client) == X_STATUS_SUCCESS && client == 0);
  audio->Pause();
  audio->ClearCredits();
  audio->drivers[0]->on_consumed();
  CHECK(audio->State().clients[0].credits == 1);
  const auto frame = memory->SystemHeapAlloc(sizeof(float) * SdlOutput::kFrameSamples);
  std::array<float, SdlOutput::kFrameSamples> samples{};
  samples[13] = 0.375f;
  std::memcpy(memory->TranslateVirtual(frame), samples.data(), sizeof(samples));
  audio->SubmitFrame(client, frame);
  const auto context = audio->xma_decoder()->AllocateContext();
  CHECK(context);
  const auto state = audio->State();
  const auto wrapped = state.clients[0].callback.wrapped_argument;
  CHECK(allocated(wrapped));
  std::vector<uint8_t> bytes(16 << 20), memory_bytes(64 << 20);
  rex::stream::ByteStream saved(bytes.data(), bytes.size()), heap(memory_bytes.data(), memory_bytes.size());
  CHECK(audio->Save(&saved) && memory->Save(&heap));
  const auto size = saved.offset();
  CHECK(size > 1024 && *reinterpret_cast<const uint32_t*>(bytes.data()) == kAudioCompleteSaveSignature);
  audio->SubmitFrame(client, frame);
  size_t later = 99;
  CHECK(audio->RegisterClient(0, 0x87654321, &later) == X_STATUS_SUCCESS && later == 1);
  const auto later_wrapped = audio->State().clients[1].callback.wrapped_argument;
  CHECK(later_wrapped && allocated(later_wrapped));
  audio->xma_decoder()->ReleaseContext(context);
  CHECK(audio->PrepareRestore() && !audio->drivers[0] && !audio->drivers[1]);
  heap.set_offset(0); CHECK(memory->Restore(&heap));
  saved.set_offset(0); CHECK(audio->Restore(&saved) && saved.offset() == size);
  CHECK(audio->is_paused() && audio->drivers[0] && !audio->drivers[1]);
  CHECK(audio->drivers[0]->output.QueuedFrames() == 1);
  CHECK(allocated(wrapped) && !allocated(later_wrapped));
  CHECK(rex::memory::load_and_swap<uint32_t>(memory->TranslateVirtual(wrapped)) == 0x12345678);
  std::vector<uint8_t> roundtrip(bytes.size());
  rex::stream::ByteStream again(roundtrip.data(), roundtrip.size());
  CHECK(audio->Save(&again) && again.offset() == size);
  CHECK(std::equal(bytes.begin(), bytes.begin() + size, roundtrip.begin()));
  CHECK(audio->State().clients[0].credits == state.clients[0].credits);

  // Exercise the actual restored AudioSystem callback in a Release build.
  audio->ClearCredits();
  audio->drivers[0]->on_consumed();
  CHECK(audio->State().clients[0].credits == 1);

  // A failed endpoint restore cannot resume the worker or free the incoming heap's argument.
  audio->fail_restore = true; saved.set_offset(0);
  CHECK(!audio->Restore(&saved)); CHECK(!audio->Resume());
  CHECK(audio->is_paused() && allocated(wrapped));
  audio->fail_restore = false; saved.set_offset(0); CHECK(audio->Restore(&saved));

  // Callback-table-only snapshots from the previous stream remain readable.
  std::array<uint8_t, 64> legacy{};
  rex::stream::ByteStream old(legacy.data(), legacy.size());
  old.Write(kAudioSaveSignature); old.Write(uint32_t{1}); old.Write(uint32_t{0});
  old.Write(uint32_t{0}); old.Write(uint32_t{0x12345678}); old.Write(wrapped);
  old.set_offset(0); CHECK(audio->Restore(&old));
  CHECK(audio->drivers[0]->output.QueuedFrames() == 0);
  CHECK(allocated(wrapped));
  audio->UnregisterClient(0); CHECK(!allocated(wrapped));
  memory->SystemHeapFree(frame);
  std::puts("AudioSystem: restored endpoint consumption returns credit, XA2D XMA/endpoint/credits/heap roundtrip, retired arguments, failed restore pause and legacy XAUD passed");
}
