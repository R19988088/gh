#include "generated/default/guardian_heroes_init.h"
#include "guardian_heroes_app.h"
#include "gh_guest_continuation_title.h"

namespace {
std::unique_ptr<rex::ui::WindowedApp> CreateContinuationApp(
    rex::ui::WindowedAppContext& context) {
  static const auto program = gh::guest::continuation::CreateTitleContinuationProgram();
  gh::guest::continuation::InstallTitleProgram(&program);
  return GuardianHeroesApp::Create(context);
}
}  // namespace

REX_DEFINE_APP(guardian_heroes, CreateContinuationApp)
