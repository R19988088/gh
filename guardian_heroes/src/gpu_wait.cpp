#include "gh_threading.h"

void GhYieldGpuFence() {
  // Guest cctpl/db16cyc hints vanish during recompilation; let the GPU worker run.
  gh::thread::YieldCurrentThread();
}
