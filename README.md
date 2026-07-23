# SM-ModSearch

A small native mod for Scrap Mechanic that adds a search bar to the mod list in the Creative Mode menu. If you've got a big modpack installed, scrolling through every entry to find the one you want gets old fast - this just lets you type a name and filter the list down.

It works by hooking into the game's creative menu refresh function and injecting a search bar into the ModBoxPanel header, then re-filtering the ItemBox list whenever the search text changes.

## Screenshot

![search bar in the mods menu](docs/screenshot.png)

## Building

This project uses xmake. It targets Windows x64 and builds a shared library (SM-ModSearch.dll) meant to be loaded alongside Scrap Mechanic.

```
xmake
```

It depends on minhook for the function hook, and links against MyGUIEngine.dll (the game ships this without an import lib, so one gets generated at build time from mygui_engine.def).

## How it works

- `hook_creativemenu` detours the game's `CreativeModeMenu::sub_1402F51A0` function, which rebuilds the mod list ItemBoxes on every tab switch or refresh.
- `searchbar` lazily creates the search bar the first time it sees a given menu instance, and applies the current search text as a name filter over the already-populated mod list.

## Status

Early / work in progress. Built and tested against a specific Scrap Mechanic version - the RVA used for the hook may break on game updates.
