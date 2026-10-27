#pragma once
#include "gh_audio_context.h"
namespace rex::audio {
using XMA_CONTEXT_DATA = gh::audio::XmaContextData;
using XmaContext = gh::audio::XmaContext;
#pragma pack(push, 1)
struct Xma2ExtraData { uint8_t raw[34]; };
#pragma pack(pop)
static_assert(sizeof(Xma2ExtraData) == 34);
}
