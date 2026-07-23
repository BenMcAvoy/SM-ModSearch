#include "hook_creativemenu.h"
#include "searchbar.h"
#include "sm_console.h"

#include <cstdio>

CreativeMenuRefreshFn g_trampoline_CreativeMenuRefresh = nullptr;

void __fastcall Detour_CreativeMenuRefresh(uint64_t self, int mode) {
    // Let the game repopulate the ItemBox normally first.
    g_trampoline_CreativeMenuRefresh(self, mode);

    // this+72 is CreativeModeMenu's view-mode field (logged for diagnosis
    // only -- actual targeting is name-based via SearchBar, see searchbar.cpp).
    const int viewMode = *reinterpret_cast<int*>(self + 72);

    char buf[128];
    snprintf(buf, sizeof(buf), "refresh hit: self=0x%llX mode=%d viewMode=%d",
             (unsigned long long)self, mode, viewMode);
    SMConsole::Log(buf);

    SearchBar::EnsureCreated(self);
    SearchBar::ApplyFilter(self);
}
