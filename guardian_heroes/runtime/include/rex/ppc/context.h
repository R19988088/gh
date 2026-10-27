/**
 * @file        ppc/context.h
 * @brief       PPC thread context, guest function macros, and interrupt handling
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 *
 * @remarks     Based on XenonRecomp/UnleashedRecomp PPCContext infrastructure
 */

#pragma once
#include "guest_context.h"
#include <bit>
#include <rex/ppc/func.h>
#include <rex/types.h>
#include <simde/x86/avx.h>
#include <simde/x86/sse4.1.h>

constexpr float kPack2101010_Min10 = std::bit_cast<float>(0x403FFE01u);
constexpr float kPack2101010_Max10 = std::bit_cast<float>(0x404001FFu);
constexpr float kPack2101010_Min2 = std::bit_cast<float>(0x40400000u);
constexpr float kPack2101010_Max2 = std::bit_cast<float>(0x40400003u);
namespace rex::ppc {
using Register = GuestRegister;
using XERRegister = GuestXERRegister;
using CRRegister = GuestCRRegister;
using VRegister = GuestVectorRegister;
using FPSCRRegister = GuestFPSCRRegister;
constexpr uint32_t kRoundNearest = 0;
constexpr uint32_t kRoundTowardZero = 1;
constexpr uint32_t kRoundUp = 2;
constexpr uint32_t kRoundDown = 3;
constexpr uint32_t kRoundMask = 3;
}
using PPCRegister = GuestRegister;
using PPCXERRegister = GuestXERRegister;
using PPCCRRegister = GuestCRRegister;
using PPCVRegister = GuestVectorRegister;
using PPCFPSCRRegister = GuestFPSCRRegister;
