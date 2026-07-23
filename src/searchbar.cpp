#include "searchbar.h"
#include "hook_creativemenu.h"
#include "sm_console.h"

#include <MyGUI_Gui.h>
#include <MyGUI_Widget.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_ItemBox.h>
#include <MyGUI_Delegate.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <unordered_map>

namespace {

struct MenuState {
    MyGUI::ItemBox* trackedList = nullptr;
    MyGUI::EditBox* searchBox = nullptr;
    std::string lastNeedle; // lowercased text as of the last filter pass
};

std::unordered_map<uint64_t, MenuState> g_menus;

std::string ToLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

// Looked up by name instead of a guessed CreativeModeMenu field offset --
// "ModList"/"ModBoxPanel" are the widgets' actual names from
// CreativeModeMenu.layout.
MyGUI::Widget* FindByName(const char* name) {
    auto* gui = MyGUI::Gui::getInstancePtr();
    if (!gui) return nullptr;
    return gui->findWidgetT(name, false);
}

// Filters the ItemBox's *currently shown* items down to those matching
// needle. Only ever removes -- never restores -- so it's only correct to call
// when every currently-shown item is still a valid candidate (see the
// "narrowing" check below). removeItemAt/redrawAllItems are ItemBox's own
// blessed mutation API, called on the list the game itself just populated --
// unlike a raw removeAll+re-add from a detached cache (tried twice now,
// both times broke ItemBox's internal scroll/viewport state even with no
// filtering applied), this keeps things consistent because it's the same
// incremental-removal pattern the game's own UI relies on elsewhere.
void FilterCurrentItems(MyGUI::ItemBox* list, const std::string& needle) {
    for (size_t i = list->getItemCount(); i-- > 0;) {
        MyGUI::Widget* item = list->getWidgetByIndex(i);
        if (!item) continue;

        MyGUI::Widget* nameWidget = item->findWidget("Name");
        if (!nameWidget) continue;

        auto* nameText = static_cast<MyGUI::TextBox*>(nameWidget);
        const std::string caption = ToLower(nameText->getCaption().asUTF8());

        if (caption.find(needle) == std::string::npos) {
            list->removeItemAt(i, false);
        }
    }
    list->redrawAllItems();
}

// removeItemAt() is destructive -- once a keystroke filters an item out,
// nothing brings it back except rebuilding the list from the game's own
// data. So a full rebuild (via the MinHook trampoline, bypassing our hook
// to avoid recursion) only runs when the new text *could* match something
// already filtered out, i.e. when it's not a refinement of the text last
// filtered on (backspacing, pasting, clearing). Typing forward just narrows
// the already-reduced set further, which is cheap: if lastNeedle is a
// substring of needle, every caption that matches needle also matched
// lastNeedle, so nothing hidden could possibly need restoring.
void RefilterOnTextChanged(uint64_t self) {
    auto it = g_menus.find(self);
    if (it == g_menus.end()) return;

    MenuState& state = it->second;
    if (!state.searchBox || !state.trackedList) return;

    const std::string needle = ToLower(state.searchBox->getOnlyText().asUTF8());
    const bool isNarrowing = needle.find(state.lastNeedle) != std::string::npos;

    if (!isNarrowing) {
        const int mode = *reinterpret_cast<int*>(self + 72);
        g_trampoline_CreativeMenuRefresh(self, mode);
    }

    state.lastNeedle = needle;
    if (!needle.empty()) {
        FilterCurrentItems(state.trackedList, needle);
    }
}

void OnSearchTextChanged(MyGUI::EditBox* sender) {
    for (auto& [menu, state] : g_menus) {
        if (state.searchBox == sender) {
            RefilterOnTextChanged(menu);
            return;
        }
    }
}

} // namespace

void SearchBar::EnsureCreated(uint64_t self) {
    auto* modList = static_cast<MyGUI::ItemBox*>(FindByName("ModList"));
    if (!modList) {
        SMConsole::Log("EnsureCreated: 'ModList' widget not found");
        return;
    }

    MenuState& state = g_menus[self];
    state.trackedList = modList;

    if (state.searchBox) return; // header UI already built for this menu instance

    auto* panel = FindByName("ModBoxPanel");
    if (!panel) {
        SMConsole::LogWarn("EnsureCreated: 'ModBoxPanel' widget not found");
        return;
    }

    const MyGUI::IntCoord panelCoord = panel->getCoord();

    char buf[128];
    snprintf(buf, sizeof(buf), "EnsureCreated: ModBoxPanel coord=(%d,%d,%d,%d)",
             panelCoord.left, panelCoord.top, panelCoord.width, panelCoord.height);
    SMConsole::Log(buf);

    // Float the bar in the header's top-right corner instead of stealing
    // space from the list -- ModBoxPanel's header row is the top ~12.8% of
    // the panel (see CreativeModeMenu.layout's NonBlurryBackgroundDarkRoundedUpperRight).
    constexpr float kHeaderRatio = 0.128205f;
    constexpr int kBarWidth = 200;
    constexpr int kBarHeight = 28;
    constexpr int kRightMargin = 20;

    constexpr int kIconSize = 16;
    constexpr int kIconRightMargin = 8;
    constexpr int kTextPadX = 10;
    constexpr int kTextPadY = 6;

    const int headerHeight = static_cast<int>(panelCoord.height * kHeaderRatio);
    const MyGUI::IntCoord barCoord(
        panelCoord.width - kBarWidth - kRightMargin, (headerHeight - kBarHeight) / 2,
        kBarWidth, kBarHeight);

    // EditBoxEmpty is a bare text-only skin with no visible chrome -- the
    // game's own search boxes (e.g. Inventory.layout's "SearchBox") always
    // wrap it in a SearchBarBackground widget plus a magnifying-glass icon.
    auto* background = panel->createWidget<MyGUI::Widget>(
        "SearchBarBackground", barCoord, MyGUI::Align::Right | MyGUI::Align::Top, "SM_ModSearchBoxBg");
    if (!background) {
        SMConsole::LogWarn("EnsureCreated: createWidget<Widget>(SearchBarBackground) returned null");
        return;
    }

    const MyGUI::IntCoord iconCoord(
        kBarWidth - kIconSize - kIconRightMargin, (kBarHeight - kIconSize) / 2, kIconSize, kIconSize);
    auto* icon = background->createWidget<MyGUI::ImageBox>(
        "ImageBox", iconCoord, MyGUI::Align::Default, "SM_ModSearchIcon");
    if (icon) {
        icon->setImageTexture("gui_inventory_icon_search.png");
    }

    const int editBoxWidth = iconCoord.left - kTextPadX - 4;
    const MyGUI::IntCoord innerCoord(kTextPadX, kTextPadY, editBoxWidth, kBarHeight - 2 * kTextPadY);

    auto* editBox = background->createWidget<MyGUI::EditBox>(
        "EditBoxEmpty", innerCoord, MyGUI::Align::Default, "SM_ModSearchBox");
    if (!editBox) {
        SMConsole::LogWarn("EnsureCreated: createWidget<EditBox> returned null");
        return;
    }

    editBox->setCaption(MyGUI::UString(""));
    editBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
    editBox->setTextColour(MyGUI::Colour(0.99f, 0.99f, 0.99f));
    editBox->setFontName("SM_TextLabel");
    editBox->eventEditTextChange += MyGUI::newDelegate(&OnSearchTextChanged);

    state.searchBox = editBox;
    SMConsole::Log("EnsureCreated: search box created OK");
}

void SearchBar::ApplyFilter(uint64_t self) {
    auto it = g_menus.find(self);
    if (it == g_menus.end()) return;

    MenuState& state = it->second;
    if (!state.searchBox || !state.trackedList) return;

    const std::string needle = ToLower(state.searchBox->getOnlyText().asUTF8());
    state.lastNeedle = needle;

    char buf[128];
    snprintf(buf, sizeof(buf), "ApplyFilter: needle='%s' items=%zu",
             needle.c_str(), state.trackedList->getItemCount());
    SMConsole::Log(buf);

    if (needle.empty()) return; // full list as the game just populated it is correct
    FilterCurrentItems(state.trackedList, needle);
}
