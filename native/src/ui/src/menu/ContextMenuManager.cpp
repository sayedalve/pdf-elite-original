#include "menu/ContextMenuManager.h"
#include <algorithm>

namespace ui {
namespace menu {

enum PREFERRED_APP_MODE {
    AppModeDefault,
    AllowDark,
    ForceDark,
    ForceLight,
    Max
};

using fnSetPreferredAppMode = PREFERRED_APP_MODE(WINAPI*)(PREFERRED_APP_MODE);
using fnFlushMenuThemes = void(WINAPI*)();

void InitDarkMenus() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    HMODULE hUxTheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (hUxTheme) {
        auto SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxTheme, MAKEINTRESOURCEA(135));
        if (SetPreferredAppMode) {
            SetPreferredAppMode(ForceDark);
        }
        auto FlushMenuThemes = (fnFlushMenuThemes)GetProcAddress(hUxTheme, MAKEINTRESOURCEA(136));
        if (FlushMenuThemes) {
            FlushMenuThemes();
        }
    }
}

ContextMenuManager& ContextMenuManager::Instance() {
    static ContextMenuManager instance;
    InitDarkMenus();
    return instance;
}

std::vector<MenuItem> ContextMenuManager::BuildMenuItems(const ContextMenuInfo& info) const {
    std::vector<MenuItem> items;

    switch (info.targetType) {
    case TargetType::TextSelection: {
        items.push_back(MenuItem::Action(CommandIds::TextCopy, L"Copy"));
        items.push_back(MenuItem::Separator());
        items.push_back(MenuItem::Action(CommandIds::TextHighlight, L"Highlight"));
        items.push_back(MenuItem::Action(CommandIds::TextUnderline, L"Underline"));
        items.push_back(MenuItem::Action(CommandIds::TextStrikeout, L"Strikeout"));

        if (!info.selectedText.empty()) {
            items.push_back(MenuItem::Separator());
            std::wstring searchLabel = L"Search for \"" + 
                (info.selectedText.length() > 20 ? info.selectedText.substr(0, 17) + L"..." : info.selectedText) + L"\"";
            items.push_back(MenuItem::Action(CommandIds::TextSearch, searchLabel));
        }
        break;
    }
    case TargetType::TextObject: {
        items.push_back(MenuItem::Action(CommandIds::TextCopy, L"Copy"));
        items.push_back(MenuItem::Action(CommandIds::TextEdit, L"Edit"));
        items.push_back(MenuItem::Action(CommandIds::TextDelete, L"Delete"));
        items.push_back(MenuItem::Separator());
        items.push_back(MenuItem::Action(CommandIds::TextAddLink, L"Add Link (Not implemented)", false));
        break;
    }
    case TargetType::ImageObject: {
        items.push_back(MenuItem::Action(CommandIds::ImageReplace, L"Replace Image"));
        items.push_back(MenuItem::Action(CommandIds::ImageExtract, L"Extract Image"));
        items.push_back(MenuItem::Action(CommandIds::ImageCrop, L"Crop (Not implemented)", false));
        items.push_back(MenuItem::Separator());
        items.push_back(MenuItem::Action(CommandIds::ImageDelete, L"Delete"));
        break;
    }
    case TargetType::Annotation: {
        items.push_back(MenuItem::Action(CommandIds::AnnotProperties, L"Properties..."));
        items.push_back(MenuItem::Action(CommandIds::AnnotDuplicate, L"Duplicate"));
        items.push_back(MenuItem::Separator());
        items.push_back(MenuItem::Action(CommandIds::AnnotDelete, L"Delete"));
        items.push_back(MenuItem::Action(CommandIds::AnnotFlatten, L"Flatten"));
        break;
    }
    case TargetType::PageCanvas:
    case TargetType::None:
    case TargetType::Custom:
    default: {
        items.push_back(MenuItem::Action(CommandIds::PageSelectAll, L"Select All"));
        items.push_back(MenuItem::Separator());
        items.push_back(MenuItem::Action(CommandIds::PageZoomIn, L"Zoom In\tCtrl++"));
        items.push_back(MenuItem::Action(CommandIds::PageZoomOut, L"Zoom Out\tCtrl+-"));
        items.push_back(MenuItem::Separator());
        items.push_back(MenuItem::Action(CommandIds::PageRotateCw, L"Rotate Clockwise\tCtrl+R"));
        items.push_back(MenuItem::Action(CommandIds::PageRotateCcw, L"Rotate Counter-Clockwise\tCtrl+Shift+R"));
        items.push_back(MenuItem::Separator());
        items.push_back(MenuItem::Action(CommandIds::PageInsertBlank, L"Insert Blank Page..."));
        items.push_back(MenuItem::Action(CommandIds::PageDelete, L"Delete Page..."));
        break;
    }
    }

    // Apply any customizer
    auto it = m_customizers.find(info.targetType);
    if (it != m_customizers.end() && it->second) {
        it->second(items, info);
    }

    return items;
}

void ContextMenuManager::AppendItemsToHMenu(HMENU hMenu, const std::vector<MenuItem>& items, std::map<UINT, std::function<void()>>* outTriggerMap) const {
    if (!hMenu) return;

    for (const auto& item : items) {
        if (item.isSeparator) {
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        } else if (!item.children.empty()) {
            HMENU hSub = CreatePopupMenu();
            AppendItemsToHMenu(hSub, item.children, outTriggerMap);
            UINT flags = MF_POPUP;
            if (!item.enabled) flags |= (MF_DISABLED | MF_GRAYED);
            AppendMenuW(hMenu, flags, reinterpret_cast<UINT_PTR>(hSub), item.text.c_str());
        } else {
            UINT flags = MF_STRING;
            if (!item.enabled) flags |= (MF_DISABLED | MF_GRAYED);
            if (item.checked) flags |= MF_CHECKED;

            AppendMenuW(hMenu, flags, item.id, item.text.c_str());

            if (outTriggerMap && item.onTrigger) {
                (*outTriggerMap)[item.id] = item.onTrigger;
            }
        }
    }
}

ScopedHMenu ContextMenuManager::CreateNativeMenu(const std::vector<MenuItem>& items, std::map<UINT, std::function<void()>>* outTriggerMap) const {
    HMENU hMenu = CreatePopupMenu();
    if (hMenu) {
        AppendItemsToHMenu(hMenu, items, outTriggerMap);
    }
    return ScopedHMenu(hMenu);
}

int ContextMenuManager::ShowContextMenu(HWND hwnd, const POINT& screenPt, const ContextMenuInfo& info) {
    if (!hwnd) return 0;

    std::map<UINT, std::function<void()>> triggerMap;
    auto items = BuildMenuItems(info);
    ScopedHMenu menu = CreateNativeMenu(items, &triggerMap);

    if (!menu) return 0;

    int choice = TrackPopupMenu(
        menu.Get(),
        TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RETURNCMD,
        screenPt.x,
        screenPt.y,
        0,
        hwnd,
        NULL
    );

    if (choice > 0) {
        auto it = triggerMap.find(static_cast<UINT>(choice));
        if (it != triggerMap.end() && it->second) {
            it->second();
        } else {
            SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(static_cast<WORD>(choice), 0), 0);
        }
    }

    return choice;
}

void ContextMenuManager::SetCustomizer(TargetType target, Customizer customizer) {
    m_customizers[target] = std::move(customizer);
}

void ContextMenuManager::ClearCustomizer(TargetType target) {
    m_customizers.erase(target);
}

} // namespace menu
} // namespace ui
