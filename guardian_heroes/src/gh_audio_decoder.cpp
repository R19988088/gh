/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include "gh_audio_context.h"
#include "gh_audio_decoder.h"
#include <rex/cvar.h>
#include <rex/dbg.h>
#include <rex/logging.h>
#include <rex/perf/counter.h>
#include <rex/math.h>
#include <rex/memory/ring_buffer.h>
#include <rex/string/buffer.h>
#include <rex/system/function_dispatcher.h>
#include <rex/system/thread_state.h>
#include <rex/system/xthread.h>

extern "C" {
#include "libavutil/log.h"
}  // extern "C"

REXCVAR_DEFINE_BOOL(ffmpeg_verbose, false, "Audio", "Verbose FFmpeg output (debug and above)");

// As with normal Microsoft, there are like twelve different ways to access
// the audio APIs. Early games use XMA*() methods almost exclusively to touch
// decoders. Later games use XAudio*() and direct memory writes to the XMA
// structures (as opposed to the XMA* calls), meaning that we have to support
// both.
//
// The XMA*() functions just manipulate the audio system in the guest context
// and let the normal XmaDecoder handling take it, to prevent duplicate
// implementations. They can be found in xboxkrnl_audio_xma.cc
//
// XMA details:
// https://devel.nuclex.org/external/svn/directx/trunk/include/xma2defs.h
// https://github.com/gdawg/fsbext/blob/master/src/xma_header.h
//
// XAudio2 uses XMA under the covers, and seems to map with the same
// restrictions of frame/subframe/etc:
// https://msdn.microsoft.com/en-us/library/windows/desktop/microsoft.directx_sdk.xaudio2.xaudio2_buffer(v=vs.85).aspx
//
// XMA contexts are 64b in size and tight bitfields. They are in physical
// memory not usually available to games. Games will use MmMapIoSpace to get
// the 64b pointer in user memory so they can party on it. If the game doesn't
// do this, it's likely they are either passing the context to XAudio or
// using the XMA* functions.

namespace gh::audio {

XmaDecoder::XmaDecoder(rex::runtime::FunctionDispatcher* function_dispatcher)
    : memory_(function_dispatcher->memory()), function_dispatcher_(function_dispatcher) {}

XmaDecoder::~XmaDecoder() { Shutdown(); }

void av_log_callback(void* avcl, int level, const char* fmt, va_list va) {
  if (!REXCVAR_GET(ffmpeg_verbose) && level > AV_LOG_WARNING) {
    return;
  }

  rex::string::StringBuffer buff;
  buff.AppendVarargs(fmt, va);
  auto msg = buff.to_string_view();

  switch (level) {
    case AV_LOG_ERROR:
      REXAPU_ERROR("ffmpeg: {}", msg);
      break;
    case AV_LOG_WARNING:
      REXAPU_WARN("ffmpeg: {}", msg);
      break;
    case AV_LOG_INFO:
      REXAPU_INFO("ffmpeg: {}", msg);
      break;
    case AV_LOG_VERBOSE:
    case AV_LOG_DEBUG:
    default:
      REXAPU_DEBUG("ffmpeg: {}", msg);
      break;
  }
}

X_STATUS XmaDecoder::Setup(rex::system::KernelState* kernel_state) {
  // Setup ffmpeg logging callback
  av_log_set_callback(av_log_callback);

  // Register APU/XMA MMIO handlers
  // XMA registers are at 0x7FEA0000-0x7FEAFFFF
  memory()->AddVirtualMappedRange(
      0x7FEA0000,  // base address
      0xFFFF0000,  // mask
      0x0000FFFF,  // size (64KB)
      this,        // context (XmaDecoder*)
      reinterpret_cast<rex::runtime::MMIOReadCallback>(MMIOReadRegisterThunk),
      reinterpret_cast<rex::runtime::MMIOWriteCallback>(MMIOWriteRegisterThunk));
  REXAPU_DEBUG("XMA: Registered MMIO handlers at 0x7FEA0000-0x7FEAFFFF");

  // Setup XMA context data.
  // The Xbox 360 kernel allocates the contexts with X_PAGE_NOCACHE |
  // X_PAGE_READWRITE and writes MmGetPhysicalAddress for the address to the
  // register.
  context_data_first_ptr_ = memory()->SystemHeapAlloc(sizeof(XmaContextData) * gh::audio::XmaRuntime::kContextCount, 256,
                                                      rex::memory::kSystemHeapPhysical);
  context_data_last_ptr_ = context_data_first_ptr_ + (sizeof(XmaContextData) * gh::audio::XmaRuntime::kContextCount - 1);
  gh::audio::GuestAudioMemory memory{
      [this](uint32_t address) { return memory_->TranslateVirtual(address); },
      [this](uint32_t address) { return memory_->TranslatePhysical(address); },
      [](std::string_view message) { REXAPU_ERROR("{}", message); }};
  if (!runtime_.Initialize(std::move(memory), context_data_first_ptr_,
                           memory_->GetPhysicalAddress(context_data_first_ptr_),
                           [] { PROFILE_XMA_FRAME_DECODED(); })) {
    memory_->SystemHeapFree(context_data_first_ptr_);
    context_data_first_ptr_ = context_data_last_ptr_ = 0;
    return X_STATUS_UNSUCCESSFUL;
  }
  return X_STATUS_SUCCESS;
}

void XmaDecoder::Shutdown() {
  runtime_.Stop();
  if (context_data_first_ptr_) memory_->SystemHeapFree(context_data_first_ptr_);
  context_data_first_ptr_ = context_data_last_ptr_ = 0;
}
uint32_t XmaDecoder::AllocateContext() { return runtime_.AllocateContext(); }
void XmaDecoder::ReleaseContext(uint32_t pointer) { runtime_.ReleaseContext(pointer); }
bool XmaDecoder::BlockOnContext(uint32_t pointer, bool poll) {
  return runtime_.BlockOnContext(pointer, poll);
}
uint32_t XmaDecoder::ReadRegister(uint32_t address) { return runtime_.ReadRegister(address); }
void XmaDecoder::WriteRegister(uint32_t address, uint32_t value) {
  runtime_.WriteRegister(address, value);
}
void XmaDecoder::Pause() { runtime_.Pause(); }
void XmaDecoder::Resume() { runtime_.Resume(); }
bool XmaDecoder::SaveState(State& state) {
  if (!is_paused() || !context_data_first_ptr_ ||
      context_data_last_ptr_ != context_data_first_ptr_ +
          sizeof(XmaContextData) * XmaRuntime::kContextCount - 1 ||
      runtime_.context_array_ptr() != memory_->GetPhysicalAddress(context_data_first_ptr_))
    return false;
  return runtime_.SaveState(state);
}
bool XmaDecoder::RestoreState(const State& state) {
  if (!is_paused() || !context_data_first_ptr_ ||
      state.context_base != context_data_first_ptr_ ||
      context_data_last_ptr_ != context_data_first_ptr_ +
          sizeof(XmaContextData) * XmaRuntime::kContextCount - 1 ||
      runtime_.context_array_ptr() != memory_->GetPhysicalAddress(context_data_first_ptr_) ||
      state.registers[0x600] != runtime_.context_array_ptr()) return false;
  return runtime_.RestoreState(state);
}
}  // namespace gh::audio
