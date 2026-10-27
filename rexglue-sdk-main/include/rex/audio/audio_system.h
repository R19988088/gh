#pragma once
// Preserve the SDK include API; the project owns audio execution.
#include "gh_audio_system.h"
namespace rex::audio {
using gh::audio::AudioSystem;
using gh::audio::kAudioSaveSignature;
}
