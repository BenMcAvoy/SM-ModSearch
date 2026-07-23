#pragma once
#include <cstdint>

namespace SearchBar {

// Lazily creates the search bar in ModBoxPanel's header the first time it
// sees a given menu instance.
void EnsureCreated(uint64_t creativeModeMenu);

// Re-applies the current search text as a name filter over the
// already-populated ModList ItemBox.
void ApplyFilter(uint64_t creativeModeMenu);

} // namespace SearchBar
