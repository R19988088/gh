#include "gh_exceptions.h"

#include "gh_logging.h"

#include <exception>
#include <cstdlib>

namespace gh::exceptions {
namespace {
void Terminate() noexcept {
  try {
    if (const auto error = std::current_exception()) std::rethrow_exception(error);
  } catch (const std::exception& error) {
    logging::Error(error.what());
  } catch (...) {
    logging::Error("Unhandled non-standard exception");
  }
  std::_Exit(134);
}
}  // namespace

void InstallTerminateHandler() noexcept { std::set_terminate(Terminate); }

}  // namespace gh::exceptions
