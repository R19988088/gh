#include "gh_audio_factory.h"
#include "gh_audio_nop_system.h"
#include "gh_audio_sdl_system.h"

namespace gh::audio {
std::unique_ptr<rex::system::IAudioSystem> CreateSdlAudioSystem(
    rex::runtime::FunctionDispatcher* dispatcher) {
  return sdl::SDLAudioSystem::Create(dispatcher);
}
std::unique_ptr<rex::system::IAudioSystem> CreateNopAudioSystem(
    rex::runtime::FunctionDispatcher* dispatcher) {
  return nop::NopAudioSystem::Create(dispatcher);
}
}  // namespace gh::audio
