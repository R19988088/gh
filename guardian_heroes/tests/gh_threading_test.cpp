#include "gh_threading.h"

#include <cassert>
#include <string_view>

int main(int argc, char** argv) {
  if (argc > 1 && std::string_view(argv[1]) == "unmatched-leave") {
    gh::thread::GlobalCriticalRegion::Leave();
    return 0;
  }

  gh::thread::YieldCurrentThread();

  std::recursive_mutex runtime_mutex;
  gh::thread::GlobalCriticalRegion::BindRuntimeMutex(&runtime_mutex);
  assert(&gh::thread::GlobalCriticalRegion::mutex() == &runtime_mutex);
  assert(gh::thread::GlobalCriticalRegion::depth() == 0);
  assert(gh::thread::GlobalCriticalRegion::Check() == 0x8000);

  gh::thread::GlobalCriticalRegion::Enter();
  assert(gh::thread::GlobalCriticalRegion::depth() == 1);
  assert(gh::thread::GlobalCriticalRegion::Check() == 0);
  gh::thread::GlobalCriticalRegion::Enter();
  assert(gh::thread::GlobalCriticalRegion::depth() == 2);
  gh::thread::GlobalCriticalRegion::Leave();
  assert(gh::thread::GlobalCriticalRegion::depth() == 1);
  gh::thread::GlobalCriticalRegion::Leave();
  assert(gh::thread::GlobalCriticalRegion::depth() == 0);
  assert(gh::thread::GlobalCriticalRegion::Check() == 0x8000);

  // Runtime and generated-code users share the exact same recursive mutex.
  runtime_mutex.lock();
  {
    auto nested = gh::thread::GlobalCriticalRegion::AcquireDirect();
    assert(nested.owns_lock());
  }
  runtime_mutex.unlock();
}
