#if defined(WIN32) || defined(_WIN32)
#if defined(__aarch64__) || defined(_M_ARM64)
#include "config_windows_aarch64.h"
#else
#include "config_windows_x86_64.h"
#endif
#elif defined(__APPLE__) && defined(__MACH__)
#if defined(__aarch64__) || defined(__arm64__)
#include "config_macos_aarch64.h"
#else
#include "config_macos_x86_64.h"
#endif
#elif defined(__gnu_linux__)
#if defined(__aarch64__)
#include "config_linux_aarch64.h"
#else
#include "config_linux_x86_64.h"
#endif
#elif defined(__ANDROID__)
#if defined(__aarch64__)
#include "config_android_aarch64.h"
#elif defined(__amd64__) || defined(__x86_64__)
#include "config_android_x86_64.h"
#else
#error "android arch unsupported"
#endif
#elif defined(__SWITCH__) && defined(__aarch64__)
// The devkitA64 toolchain identifies the target as Generic rather than Linux.
// This FFmpeg subset only uses the AArch64 codec/utility configuration, so keep
// the generated target settings while selecting it explicitly for Switch.
#include "config_linux_aarch64.h"
#undef HAVE_MMAP
#define HAVE_MMAP 0
#undef HAVE_MAPVIEWOFFILE
#define HAVE_MAPVIEWOFFILE 0
#else
#error "no config"
#endif
