#pragma once
#include "gh_guest_continuation.h"

namespace gh::guest::continuation {
std::unique_ptr<PendingHost> BeginKeDelay(GuestContext&, uint8_t*, std::span<const uint8_t>);
std::unique_ptr<PendingHost> BeginKeWaitSingle(GuestContext&, uint8_t*, std::span<const uint8_t>);
std::unique_ptr<PendingHost> BeginNtWaitSingle(GuestContext&, uint8_t*, std::span<const uint8_t>);
std::unique_ptr<PendingHost> BeginKeWaitMultiple(GuestContext&, uint8_t*, std::span<const uint8_t>);
std::unique_ptr<PendingHost> BeginNtWaitMultiple(GuestContext&, uint8_t*, std::span<const uint8_t>);
std::unique_ptr<PendingHost> BeginSignalAndWait(GuestContext&, uint8_t*, std::span<const uint8_t>);
}  // namespace gh::guest::continuation
