/**
 * @file        audio/downmix.h
 * @brief       Output-stage mix parameters: 5.1 fold, 5.1 matrix and master gain
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */
#pragma once

#include "gh_audio.h"

namespace rex::audio {

using StereoFold = gh::audio::StereoFold;
using SurroundMix = gh::audio::SurroundMix;

/// Safe to call from any thread. The output stage picks the new values up on
/// its next device callback.
using gh::audio::SetStereoFold;
using gh::audio::GetStereoFold;

using gh::audio::SetSurroundMix;
using gh::audio::GetSurroundMix;

/// Linear master gain applied to the stereo fold and to 5.1 passthrough alike.
/// 1.0 is unity. Above unity can clip, and the output stage clamps.
using gh::audio::SetOutputGain;
using gh::audio::GetOutputGain;

}  // namespace rex::audio
