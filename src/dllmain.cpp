#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <MinHook.h>
#include <cstdint>

#include "hook_creativemenu.h"
#include "sm_console.h"

#include <cstdio>

namespace {

// CreativeModeMenu::sub_1402F51A0 in ScrapMechanic.exe (confirmed via IDA
// against ScrapMechanic.exe.i64). Relative to the exe's own module base, so
// it survives normal ASLR relocation.
constexpr uintptr_t kCreativeMenuRefreshRVA = 0x2F51A0;

void InstallHooks() {
    MH_STATUS st = MH_Initialize();
    if (st != MH_OK && st != MH_ERROR_ALREADY_INITIALIZED) {
        SMConsole::LogWarn(std::string("MH_Initialize failed: ") + MH_StatusToString(st));
        return;
    }

    auto* base = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    void* target = base + kCreativeMenuRefreshRVA;

    char buf[128];
    snprintf(buf, sizeof(buf), "exe base=0x%p target=0x%p (rva 0x%zX)", (void*)base, target, kCreativeMenuRefreshRVA);
    SMConsole::Log(buf);

    st = MH_CreateHook(target, reinterpret_cast<void*>(&Detour_CreativeMenuRefresh),
                        reinterpret_cast<void**>(&g_trampoline_CreativeMenuRefresh));
    if (st != MH_OK) {
        SMConsole::LogWarn(std::string("MH_CreateHook failed: ") + MH_StatusToString(st));
        return;
    }

    st = MH_EnableHook(target);
    if (st != MH_OK) {
        SMConsole::LogWarn(std::string("MH_EnableHook failed: ") + MH_StatusToString(st));
        return;
    }

    SMConsole::Log("hook installed OK");
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        SMConsole::Log("DllMain attach");
        InstallHooks();
    }
    return TRUE;
}
