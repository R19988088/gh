#include <rex/platform.h>
#include <rex/thread.h>

#if REX_PLATFORM_SWITCH
#include <switch.h>

namespace rex::thread {

void Sleep(std::chrono::microseconds duration) {
  if (duration.count() <= 0) {
    svcSleepThread(0);
    return;
  }

  const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  svcSleepThread(nanoseconds);
}

}  // namespace rex::thread
#endif
