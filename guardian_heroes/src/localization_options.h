#pragma once

#include <rex/ppc/context.h>

void GhAppendLanguageOption(GuestRegister& stack, GuestRegister& menu);
void GhHandleLanguageOption(GuestRegister& stack, GuestRegister& menu, GuestRegister& scene);
void GhRefreshTitleOptions(GuestRegister& stack, GuestRegister& scene);
void GhRefreshPauseOptions(GuestRegister& stack, GuestRegister& scene);
