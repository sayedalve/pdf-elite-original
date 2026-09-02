#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include "core/interfaces/dom/Navigation.h"

namespace components {

class BookmarkPanel {
public:
    BookmarkPanel();
    ~BookmarkPanel();

    bool Create(HWND parentHwnd);
    void Show();
    void Hide();
    void SetBounds(int x, int y, int width, int height);
    bool IsVisible() const;
    
    void LoadBookmarks(const std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>>& bookmarks);
    
    void SetOnNavigateCallback(std::function<void(const core::interfaces::dom::NavigationTarget&)> cb) { m_onNavigate = cb; }
    void SetOnAddBookmarkCallback(std::function<void()> cb) { m_onAddBookmark = cb; }
    
    HWND GetHwnd() const { return m_hwndTree; }

private:
    void PopulateTree(HTREEITEM hParent, const std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>>& bookmarks);
    
    static LRESULT CALLBACK ContainerWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND m_parentHwnd = nullptr;
    HWND m_hwndContainer = nullptr;
    HWND m_hwndTree = nullptr;
    HWND m_hwndAddButton = nullptr;
    std::function<void(const core::interfaces::dom::NavigationTarget&)> m_onNavigate;
    std::function<void()> m_onAddBookmark;
    
    std::vector<core::interfaces::dom::NavigationTarget> m_destinations;
};

} // namespace components
