#include "gh_threading.h"

#include <cassert>
#include <cstdlib>
#include <thread>

namespace gh::thread {
namespace {

std::recursive_mutex& fallback_mutex() {
  static std::recursive_mutex mutex;
  return mutex;
}

std::recursive_mutex*& bound_mutex() {
  static std::recursive_mutex* mutex = nullptr;
  return mutex;
}

std::atomic<std::int32_t>& lock_depth() {
  static std::atomic<std::int32_t> depth{0};
  return depth;
}

}  // namespace

void YieldCurrentThread() noexcept {
  std::this_thread::yield();
  std::atomic_thread_fence(std::memory_order_seq_cst);
}

std::recursive_mutex& GlobalCriticalRegion::mutex() {
  if (auto* bound = bound_mutex()) return *bound;
  return fallback_mutex();
}

std::unique_lock<std::recursive_mutex> GlobalCriticalRegion::AcquireDirect() {
  return std::unique_lock<std::recursive_mutex>(mutex());
}

void GlobalCriticalRegion::BindRuntimeMutex(std::recursive_mutex* runtime_mutex) {
  assert(lock_depth().load(std::memory_order_relaxed) == 0 &&
         "cannot rebind global critical region while guest lock is held");
  bound_mutex() = runtime_mutex;
}

void GlobalCriticalRegion::Enter() {
  mutex().lock();
  lock_depth().fetch_add(1, std::memory_order_relaxed);
}

void GlobalCriticalRegion::Leave() {
  const auto old_depth = lock_depth().load(std::memory_order_relaxed);
  assert(old_depth >= 1 && "LeaveGlobalLock called without matching EnterGlobalLock");
  if (old_depth < 1) std::abort();
  lock_depth().fetch_sub(1, std::memory_order_relaxed);
  mutex().unlock();
}

std::uint64_t GlobalCriticalRegion::Check() {
  auto lock = AcquireDirect();
  return lock_depth().load(std::memory_order_relaxed) ? 0 : 0x8000;
}

std::int32_t GlobalCriticalRegion::depth() {
  return lock_depth().load(std::memory_order_relaxed);
}

}  // namespace gh::thread
