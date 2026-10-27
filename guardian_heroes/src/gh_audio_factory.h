#pragma once

#include <memory>
#include <rex/system/interfaces/audio.h>

namespace rex::runtime { class FunctionDispatcher; }

namespace gh::audio {
std::unique_ptr<rex::system::IAudioSystem> CreateSdlAudioSystem(
    rex::runtime::FunctionDispatcher* dispatcher);
std::unique_ptr<rex::system::IAudioSystem> CreateNopAudioSystem(
    rex::runtime::FunctionDispatcher* dispatcher);
}  // namespace gh::audio
