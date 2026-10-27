#include <rex/exception_handler.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
void* page;
size_t page_size;
volatile sig_atomic_t recovered = 0;

bool Recover(rex::arch::Exception* exception, void*) {
  if (exception->code() != rex::arch::Exception::Code::kAccessViolation ||
      exception->fault_address() != reinterpret_cast<uintptr_t>(page)) return false;
  if (mprotect(page, page_size, PROT_READ | PROT_WRITE)) _exit(90);
  recovered = 1;
  return true;
}
bool Decline(rex::arch::Exception*, void*) { return false; }
void PreviousSimple(int signal) { _exit(signal == SIGSEGV || signal == SIGBUS ? 42 : 91); }
void PreviousInfo(int signal, siginfo_t* info, void* context) {
  _exit((signal == SIGSEGV || signal == SIGBUS) && info && info->si_addr == page && context
            ? 43 : 92);
}

enum class Case { Recover, Default, Simple, Info };
void Child(Case test) {
  const rlimit no_core{0, 0};
  setrlimit(RLIMIT_CORE, &no_core);
  alarm(3);
  page_size = static_cast<size_t>(sysconf(_SC_PAGESIZE));
  page = mmap(nullptr, page_size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
  if (page == MAP_FAILED) _exit(93);
  struct sigaction previous{};
  sigemptyset(&previous.sa_mask);
  previous.sa_handler = SIG_DFL;
  if (test == Case::Simple) previous.sa_handler = PreviousSimple;
  if (test == Case::Info) {
    previous.sa_flags = SA_SIGINFO;
    previous.sa_sigaction = PreviousInfo;
  }
  sigaction(SIGSEGV, &previous, nullptr);
#if defined(__APPLE__)
  sigaction(SIGBUS, &previous, nullptr);
#endif
  auto handler = test == Case::Recover ? Recover : Decline;
  rex::arch::ExceptionHandler::Install(handler, nullptr);
  *static_cast<volatile uint32_t*>(page) = 0x12345678;
  if (test != Case::Recover || !recovered ||
      *static_cast<volatile uint32_t*>(page) != 0x12345678) _exit(94);
  rex::arch::ExceptionHandler::Uninstall(handler, nullptr);
  _exit(0);
}
void Check(Case test, int expected_exit) {
  const auto child = fork();
  if (child < 0) std::abort();
  if (!child) Child(test);
  int status = 0;
  if (waitpid(child, &status, 0) != child) std::abort();
  const bool passed = test == Case::Default
      ? WIFSIGNALED(status) && (WTERMSIG(status) == SIGSEGV || WTERMSIG(status) == SIGBUS)
      : WIFEXITED(status) && WEXITSTATUS(status) == expected_exit;
  if (!passed) {
    std::fprintf(stderr, "POSIX exception case %d failed: child status %d\n", int(test), status);
    std::abort();
  }
}
}  // namespace

int main() {
  Check(Case::Recover, 0);
  Check(Case::Default, 0);
  Check(Case::Simple, 42);
  Check(Case::Info, 43);
  std::puts("POSIX exceptions: recoverable protection fault, default disposition and both handler forms passed");
}
