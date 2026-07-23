#pragma once
#include <cstdint>

// CreativeModeMenu::sub_1402F51A0(this, mode) -- rebuilds the mods list
// ItemBoxes on every tab-switch/refresh. RVA 0x2F51A0 in ScrapMechanic.exe.
using CreativeMenuRefreshFn = void(__fastcall*)(uint64_t self, int mode);

extern CreativeMenuRefreshFn g_trampoline_CreativeMenuRefresh;

void __fastcall Detour_CreativeMenuRefresh(uint64_t self, int mode);
