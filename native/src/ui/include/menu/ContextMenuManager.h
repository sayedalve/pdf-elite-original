#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <map>

namespace ui {
namespace menu {

// Command IDs for context menu actions
namespace CommandIds {
    constexpr UINT ImageReplace   = 3001;
    constexpr UINT ImageExtract   = 3002;
    constexpr UINT ImageCrop      = 3003;
    constexpr UINT ImageDelete    = 3004;

    constexpr UINT TextCopy       = 3010;
    constexpr UINT TextEdit       = 3011;
    constexpr UINT TextDelete     = 3012;
    constexpr UINT TextAddLink    = 3013;
    constexpr UINT TextHighlight  = 3014;
    constexpr UINT TextUnderline  = 3015;
    constexpr UINT TextStrikeout  = 3016;
    constexpr UINT TextSearch     = 3017;

    constexpr UINT AnnotProperties= 3020;
    constexpr UINT AnnotDuplicate = 3021;
    constexpr UINT AnnotDelete    = 3022;
    constexpr UINT AnnotFlatten   = 3023;

    constexpr UINT PageInsertBlank= 3030;
    constexpr UINT PageDelete     = 3031;
    constexpr UINT PageRotateCw   = 3032;
    constexpr UINT PageRotateCcw  = 3033;
    constexpr UINT PageZoomIn     = 3034;
    constexpr UINT PageZoomOut    = 3035;
    constexpr UINT PageSelectAll  = 3036;
}

enum class TargetType {
    None,
    TextSelection,
    TextObject,
    ImageObject,
    Annotation,
    PageCanvas,
    Custom
};

struct MenuItem {
    UINT id = 0;
    std::wstring text;
    bool enabled = true;
    bool checked = false;
    bool isSeparator = false;
    std::vector<MenuItem> children;
    std::function<void()> onTrigger;

    static MenuItem Action(UINT id, std::wstring text, bool enabled = true, bool checked = false, std::function<void()> trigger = nullptr) {
        MenuItem item;
        item.id = id;
        item.text = std::move(text);
        item.enabled = enabled;
        item.checked = checked;
        item.isSeparator = false;
        item.onTrigger = std::move(trigger);
        return item;
    }

    static MenuItem Separator() {
        MenuItem item;
        item.isSeparator = true;
        return item;
    }

    static MenuItem Submenu(std::wstring text, std::vector<MenuItem> children, bool enabled = true) {
        MenuItem item;
        item.text = std::move(text);
        item.children = std::move(children);
        item.enabled = enabled;
        item.isSeparator = false;
        return item;
    }
};

struct ContextMenuInfo {
    TargetType targetType = TargetType::None;
    std::wstring selectedText;
    bool hasSelection = false;
    bool canUndo = false;
    bool canRedo = false;
    bool canPaste = false;
    int pageIndex = -1;
    float pageX = 0.0f;
    float pageY = 0.0f;
    float viewX = 0.0f;
    float viewY = 0.0f;
    std::shared_ptr<void> targetObject = nullptr;
};

// RAII wrapper for Win32 HMENU
class ScopedHMenu {
public:
    explicit ScopedHMenu(HMENU hMenu = nullptr) : m_hMenu(hMenu) {}
    ~ScopedHMenu() {
        if (m_hMenu) {
            DestroyMenu(m_hMenu);
        }
    }

    ScopedHMenu(const ScopedHMenu&) = delete;
    ScopedHMenu& operator=(const ScopedHMenu&) = delete;

    ScopedHMenu(ScopedHMenu&& other) noexcept : m_hMenu(other.m_hMenu) {
        other.m_hMenu = nullptr;
    }

    ScopedHMenu& operator=(ScopedHMenu&& other) noexcept {
        if (this != &other) {
            if (m_hMenu) DestroyMenu(m_hMenu);
            m_hMenu = other.m_hMenu;
            other.m_hMenu = nullptr;
        }
        return *this;
    }

    HMENU Get() const { return m_hMenu; }
    HMENU Release() {
        HMENU h = m_hMenu;
        m_hMenu = nullptr;
        return h;
    }

    explicit operator bool() const { return m_hMenu != nullptr; }

private:
    HMENU m_hMenu = nullptr;
};

class ContextMenuManager {
public:
    static ContextMenuManager& Instance();

    ContextMenuManager() = default;
    ~ContextMenuManager() = default;

    // Builds a list of MenuItem descriptors according to target context
    std::vector<MenuItem> BuildMenuItems(const ContextMenuInfo& info) const;

    // Converts a list of MenuItem descriptors into a native Win32 HMENU
    ScopedHMenu CreateNativeMenu(const std::vector<MenuItem>& items, std::map<UINT, std::function<void()>>* outTriggerMap = nullptr) const;

    // Displays the popup menu and returns the selected command ID (or 0 if cancelled)
    int ShowContextMenu(HWND hwnd, const POINT& screenPt, const ContextMenuInfo& info);

    // Custom menu modifier callback
    using Customizer = std::function<void(std::vector<MenuItem>&, const ContextMenuInfo&)>;
    void SetCustomizer(TargetType target, Customizer customizer);
    void ClearCustomizer(TargetType target);

private:
    void AppendItemsToHMenu(HMENU hMenu, const std::vector<MenuItem>& items, std::map<UINT, std::function<void()>>* outTriggerMap) const;

    std::map<TargetType, Customizer> m_customizers;
};

} // namespace menu
} // namespace ui
