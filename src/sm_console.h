#pragma once
// Minimal mirror of the in-game dev console API, trimmed from
// C:\Users\Ben\scrapcheese\src\sdk.hpp (SM::Contraption / SM::UTILS::Console).
// Contraption has no vtable (plain singleton, offset access); Console is
// vtable-based (game-implemented), so this class's layout must match the
// real one exactly for virtual dispatch to land on the right slots.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <string>

namespace SMConsole {

enum class LogAttributes : uint32_t {
    FgBlue = 0x0001,
    FgGreen = 0x0002,
    FgRed = 0x0004,
    FgIntensity = 0x0008,
};

enum class LogType : uint32_t {
    Default = 1u << 0,
    Warning = 1u << 30,
    System = 0x80000000u,
};

class Console {
private:
    virtual ~Console() {}

public:
    virtual void Print(std::string msg, LogAttributes attribs, LogType type) {}
    virtual void PrintNoRepeat(std::string msg, LogAttributes attribs, LogType type) {}
};

class Contraption {
public:
    static Contraption* GetInstance() {
        return *reinterpret_cast<Contraption**>(
            reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)) + 0x1267498);
    }

    Console* GetConsole() {
        return *reinterpret_cast<Console**>(reinterpret_cast<uintptr_t>(this) + 0x58);
    }
};

inline void Log(const std::string& msg, LogAttributes attribs = LogAttributes::FgGreen) {
    auto* contraption = Contraption::GetInstance();
    if (!contraption) return;

    auto* console = contraption->GetConsole();
    if (!console) return;

    console->Print("[SM-ModSearch] " + msg, attribs, LogType::Default);
}

inline void LogWarn(const std::string& msg) {
    Log(msg, LogAttributes::FgRed);
}

} // namespace SMConsole
