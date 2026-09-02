#include "TestFramework.h"
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <optional>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commctrl.h>
#include <d2d1.h>
#include <dwrite.h>

#include "../src/ui/src/components/BookmarkPanel.h"
#include "../src/ui/src/components/SearchBar.h"
#include "../src/ui/src/components/AppShell.h"
#include "../src/ui/src/components/TabBar.h"
#include "../src/ui/src/views/HomeView.h"
#include "../src/ui/src/views/DocumentView.h"
#include "../src/core/interfaces/dom/Navigation.h"

#pragma comment(lib, "comctl32.lib")

namespace {

HWND CreateDummyWindow() {
    static bool registered = false;
    const wchar_t* className = L"M1Challenger2DummyWnd";
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    if (!registered) {
        WNDCLASS wc = {0};
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = className;
        RegisterClass(&wc);
        registered = true;
    }

    HWND hwnd = CreateWindowEx(
        0, className, L"M1 Test Dummy",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInstance, nullptr
    );
    ShowWindow(hwnd, SW_SHOW);
    return hwnd;
}

} // namespace

// ============================================================================
// SUITE 1: BookmarkPanel Lifecycle, Win32 Container & Tree Operations
// ============================================================================

TEST(Challenger2_BookmarkPanel_Creation_ShowHide_IsVisible) {
    HWND dummy = CreateDummyWindow();
    EXPECT_TRUE(dummy != nullptr);

    components::BookmarkPanel panel;
    EXPECT_FALSE(panel.IsVisible());

    bool created = panel.Create(dummy);
    EXPECT_TRUE(created);
    EXPECT_TRUE(panel.GetHwnd() != nullptr);

    // Initial state after Create is hidden container
    panel.Hide();
    EXPECT_FALSE(panel.IsVisible());

    // Show
    panel.Show();
    EXPECT_TRUE(panel.IsVisible());

    // Hide
    panel.Hide();
    EXPECT_FALSE(panel.IsVisible());

    // SetBounds
    panel.SetBounds(20, 30, 250, 400);
    // Tree control should still exist
    EXPECT_TRUE(IsWindow(panel.GetHwnd()));

    DestroyWindow(dummy);
}

TEST(Challenger2_BookmarkPanel_LoadBookmarks_EmptyList_Safety) {
    HWND dummy = CreateDummyWindow();
    components::BookmarkPanel panel;
    panel.Create(dummy);

    // Load empty vector
    std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> emptyBookmarks;
    panel.LoadBookmarks(emptyBookmarks);

    int count = TreeView_GetCount(panel.GetHwnd());
    EXPECT_EQ(count, 0);

    // Multiple empty loads should be idempotent
    panel.LoadBookmarks(emptyBookmarks);
    panel.LoadBookmarks(emptyBookmarks);
    EXPECT_EQ(TreeView_GetCount(panel.GetHwnd()), 0);

    DestroyWindow(dummy);
}

TEST(Challenger2_BookmarkPanel_LoadBookmarks_DeepHierarchy_PopulatedCorrectly) {
    HWND dummy = CreateDummyWindow();
    components::BookmarkPanel panel;
    panel.Create(dummy);

    // Build hierarchy:
    // - Chapter 1 (page 0)
    //   - Section 1.1 (page 1)
    //     - Subsection 1.1.1 (page 2)
    //   - Section 1.2 (page 3)
    // - Chapter 2 (no destination)
    //   - Section 2.1 (page 4)
    std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> bookmarks;

    auto ch1 = std::make_unique<core::interfaces::dom::PdfBookmark>();
    ch1->title = L"Chapter 1";
    ch1->destination = core::interfaces::dom::NavigationTarget{0, 10.0f, 20.0f, 1.0f};
    ch1->expanded = true;

    auto s11 = std::make_unique<core::interfaces::dom::PdfBookmark>();
    s11->title = L"Section 1.1";
    s11->destination = core::interfaces::dom::NavigationTarget{1, 0.0f, 0.0f, 1.0f};
    s11->expanded = true;

    auto sub111 = std::make_unique<core::interfaces::dom::PdfBookmark>();
    sub111->title = L"Subsection 1.1.1";
    sub111->destination = core::interfaces::dom::NavigationTarget{2, 0.0f, 0.0f, 1.0f};
    s11->children.push_back(std::move(sub111));

    auto s12 = std::make_unique<core::interfaces::dom::PdfBookmark>();
    s12->title = L"Section 1.2";
    s12->destination = core::interfaces::dom::NavigationTarget{3, 0.0f, 0.0f, 1.0f};

    ch1->children.push_back(std::move(s11));
    ch1->children.push_back(std::move(s12));

    auto ch2 = std::make_unique<core::interfaces::dom::PdfBookmark>();
    ch2->title = L"Chapter 2";
    ch2->destination = std::nullopt; // No destination

    auto s21 = std::make_unique<core::interfaces::dom::PdfBookmark>();
    s21->title = L"Section 2.1";
    s21->destination = core::interfaces::dom::NavigationTarget{4, 0.0f, 0.0f, 1.0f};
    ch2->children.push_back(std::move(s21));

    bookmarks.push_back(std::move(ch1));
    bookmarks.push_back(std::move(ch2));

    panel.LoadBookmarks(bookmarks);

    int totalItems = TreeView_GetCount(panel.GetHwnd());
    EXPECT_EQ(totalItems, 6); // 2 chapters + 2 sections + 1 subsection + 1 section

    DestroyWindow(dummy);
}

TEST(Challenger2_BookmarkPanel_NavigationCallback_Dispatch) {
    HWND dummy = CreateDummyWindow();
    components::BookmarkPanel panel;
    panel.Create(dummy);

    std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> bookmarks;
    auto bm1 = std::make_unique<core::interfaces::dom::PdfBookmark>();
    bm1->title = L"Page 5 Bookmark";
    bm1->destination = core::interfaces::dom::NavigationTarget{5, 100.0f, 200.0f, 1.5f};
    bookmarks.push_back(std::move(bm1));

    auto bm2 = std::make_unique<core::interfaces::dom::PdfBookmark>();
    bm2->title = L"No Dest Bookmark";
    bm2->destination = std::nullopt;
    bookmarks.push_back(std::move(bm2));

    panel.LoadBookmarks(bookmarks);

    int navigatedPage = -1;
    panel.SetOnNavigateCallback([&navigatedPage](const core::interfaces::dom::NavigationTarget& tgt) {
        navigatedPage = tgt.pageIndex;
    });

    // Find root item in tree view
    HWND hwndTree = panel.GetHwnd();
    HTREEITEM hRoot1 = TreeView_GetRoot(hwndTree);
    EXPECT_TRUE(hRoot1 != nullptr);

    // Select the first bookmark
    TreeView_SelectItem(hwndTree, hRoot1);

    // Trigger TVN_SELCHANGED message directly to verify container dispatch
    NMTREEVIEWW nmtv = {0};
    nmtv.hdr.hwndFrom = hwndTree;
    nmtv.hdr.code = TVN_SELCHANGEDW;
    nmtv.itemNew.lParam = 0; // index 0 in m_destinations
    HWND hwndContainer = GetParent(hwndTree);
    SendMessage(hwndContainer, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&nmtv));

    EXPECT_EQ(navigatedPage, 5);

    // Trigger for node with (size_t)-1 (no destination)
    navigatedPage = -1;
    nmtv.itemNew.lParam = static_cast<LPARAM>(-1);
    SendMessage(hwndContainer, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&nmtv));
    EXPECT_EQ(navigatedPage, -1); // Unchanged

    // Trigger with out of bounds index (e.g. 99999)
    navigatedPage = -1;
    nmtv.itemNew.lParam = 99999;
    SendMessage(hwndContainer, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&nmtv));
    EXPECT_EQ(navigatedPage, -1); // Unchanged, safe boundary guard

    DestroyWindow(dummy);
}

TEST(Challenger2_BookmarkPanel_MultipleReloads_Integrity) {
    HWND dummy = CreateDummyWindow();
    components::BookmarkPanel panel;
    panel.Create(dummy);

    // First load: 3 bookmarks
    {
        std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> bms;
        for (int i = 0; i < 3; ++i) {
            auto b = std::make_unique<core::interfaces::dom::PdfBookmark>();
            b->title = L"BM" + std::to_wstring(i);
            b->destination = core::interfaces::dom::NavigationTarget{i, 0, 0, 1};
            bms.push_back(std::move(b));
        }
        panel.LoadBookmarks(bms);
        EXPECT_EQ(TreeView_GetCount(panel.GetHwnd()), 3);
    }

    // Second load: 1 bookmark
    {
        std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> bms;
        auto b = std::make_unique<core::interfaces::dom::PdfBookmark>();
        b->title = L"OnlyBM";
        b->destination = core::interfaces::dom::NavigationTarget{42, 0, 0, 1};
        bms.push_back(std::move(b));
        panel.LoadBookmarks(bms);
        EXPECT_EQ(TreeView_GetCount(panel.GetHwnd()), 1);
    }

    int navigatedPage = -1;
    panel.SetOnNavigateCallback([&navigatedPage](const core::interfaces::dom::NavigationTarget& tgt) {
        navigatedPage = tgt.pageIndex;
    });

    HWND hwndContainer = GetParent(panel.GetHwnd());
    NMTREEVIEWW nmtv = {0};
    nmtv.hdr.hwndFrom = panel.GetHwnd();
    nmtv.hdr.code = TVN_SELCHANGEDW;
    nmtv.itemNew.lParam = 0;
    SendMessage(hwndContainer, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&nmtv));

    EXPECT_EQ(navigatedPage, 42);

    DestroyWindow(dummy);
}

TEST(Challenger2_BookmarkPanel_UncreatedInstance_NullSafety) {
    components::BookmarkPanel panel;
    // Calling methods on uncreated BookmarkPanel should be completely safe
    panel.Show();
    panel.Hide();
    panel.SetBounds(0, 0, 100, 100);
    EXPECT_FALSE(panel.IsVisible());
    std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> bms;
    panel.LoadBookmarks(bms);
}

// ============================================================================
// SUITE 2: SearchBar Lifecycle, Control Interactions, Layout & Callback Wiring
// ============================================================================

TEST(Challenger2_SearchBar_Creation_ShowHide_IsVisible) {
    HWND dummy = CreateDummyWindow();
    SearchBar searchBar;
    EXPECT_FALSE(searchBar.IsVisible());

    bool ok = searchBar.Create(dummy);
    EXPECT_TRUE(ok);

    searchBar.Hide();
    EXPECT_FALSE(searchBar.IsVisible());

    searchBar.Show();
    EXPECT_TRUE(searchBar.IsVisible());

    searchBar.Hide();
    EXPECT_FALSE(searchBar.IsVisible());

    searchBar.SetBounds(100, 50, 320, 36);
    searchBar.SetFocus();

    DestroyWindow(dummy);
}

TEST(Challenger2_SearchBar_SetResultCount_Formatting) {
    HWND dummy = CreateDummyWindow();
    SearchBar searchBar;
    searchBar.Create(dummy);

    searchBar.SetResultCount(0, 0);
    searchBar.SetResultCount(10, 3);
    searchBar.SetResultCount(1, 1);
    searchBar.SetResultCount(500, 42);

    DestroyWindow(dummy);
}

TEST(Challenger2_SearchBar_WM_COMMAND_ButtonEvents) {
    HWND dummy = CreateDummyWindow();
    SearchBar searchBar;
    searchBar.Create(dummy);

    bool nextTriggered = false;
    bool prevTriggered = false;
    bool closeTriggered = false;

    searchBar.SetOnNextCallback([&nextTriggered]() { nextTriggered = true; });
    searchBar.SetOnPrevCallback([&prevTriggered]() { prevTriggered = true; });
    searchBar.SetOnCloseCallback([&closeTriggered]() { closeTriggered = true; });

    // Find SearchBar HWND by getting child of dummy
    HWND hwndSearchBar = FindWindowEx(dummy, nullptr, L"PdfEliteSearchBar", nullptr);
    EXPECT_TRUE(hwndSearchBar != nullptr);

    // Send ID_NEXT (102)
    SendMessage(hwndSearchBar, WM_COMMAND, MAKEWPARAM(102, BN_CLICKED), 0);
    EXPECT_TRUE(nextTriggered);

    // Send ID_PREV (103)
    SendMessage(hwndSearchBar, WM_COMMAND, MAKEWPARAM(103, BN_CLICKED), 0);
    EXPECT_TRUE(prevTriggered);

    // Send ID_CLOSE (104)
    SendMessage(hwndSearchBar, WM_COMMAND, MAKEWPARAM(104, BN_CLICKED), 0);
    EXPECT_TRUE(closeTriggered);

    DestroyWindow(dummy);
}

TEST(Challenger2_SearchBar_OnSize_ZeroAndExtremeDimensions) {
    HWND dummy = CreateDummyWindow();
    SearchBar searchBar;
    searchBar.Create(dummy);

    HWND hwndSearchBar = FindWindowEx(dummy, nullptr, L"PdfEliteSearchBar", nullptr);
    EXPECT_TRUE(hwndSearchBar != nullptr);

    // Normal size
    SendMessage(hwndSearchBar, WM_SIZE, 0, MAKELPARAM(350, 40));

    // Zero size
    SendMessage(hwndSearchBar, WM_SIZE, 0, MAKELPARAM(0, 0));

    // Very narrow width (less than buttons combined)
    SendMessage(hwndSearchBar, WM_SIZE, 0, MAKELPARAM(50, 40));

    // Very large width
    SendMessage(hwndSearchBar, WM_SIZE, 0, MAKELPARAM(4000, 100));

    DestroyWindow(dummy);
}

TEST(Challenger2_SearchBar_UncreatedInstance_NullSafety) {
    SearchBar searchBar;
    searchBar.Show();
    searchBar.Hide();
    searchBar.SetBounds(0, 0, 100, 100);
    searchBar.SetFocus();
    searchBar.SetResultCount(5, 1);
    EXPECT_FALSE(searchBar.IsVisible());
}

// ============================================================================
// SUITE 3: Multi-Tab State Transitions, Panel Cleanup, and View Modes
// ============================================================================

struct MockTabState {
    std::wstring path;
    std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> bookmarks;
    bool dirty = false;
};

class MockAppLifecycleManager {
public:
    std::vector<MockTabState> tabs;
    int activeTabIndex = -1;
    components::AppShellMode currentMode = components::AppShellMode::Home;
    bool searchBarVisible = false;
    bool bookmarkPanelVisible = false;
    std::wstring currentLoadedBookmarkDoc;

    void OpenTab(const std::wstring& path, std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> bms) {
        MockTabState tab;
        tab.path = path;
        tab.bookmarks = std::move(bms);
        tabs.push_back(std::move(tab));
        activeTabIndex = static_cast<int>(tabs.size()) - 1;
        currentMode = components::AppShellMode::Document;
        currentLoadedBookmarkDoc = path;
    }

    void SwitchToTab(int index) {
        if (index < 0 || index >= static_cast<int>(tabs.size())) return;
        activeTabIndex = index;
        currentLoadedBookmarkDoc = tabs[activeTabIndex].path;
    }

    void CloseTab(int index) {
        if (index < 0 || index >= static_cast<int>(tabs.size())) return;
        tabs.erase(tabs.begin() + index);
        if (tabs.empty()) {
            activeTabIndex = -1;
            currentMode = components::AppShellMode::Home;
            searchBarVisible = false;
            bookmarkPanelVisible = false;
            currentLoadedBookmarkDoc.clear();
        } else {
            if (activeTabIndex >= static_cast<int>(tabs.size())) {
                activeTabIndex = static_cast<int>(tabs.size() - 1);
            }
            currentLoadedBookmarkDoc = tabs[activeTabIndex].path;
        }
    }

    void RequestHome() {
        bool canGoHome = true;
        for (const auto& t : tabs) {
            if (t.dirty) { canGoHome = false; break; }
        }
        if (!canGoHome) return;

        searchBarVisible = false;
        bookmarkPanelVisible = false;
        currentLoadedBookmarkDoc.clear();
        tabs.clear();
        activeTabIndex = -1;
        currentMode = components::AppShellMode::Home;
    }
};

TEST(Challenger2_Lifecycle_CloseAllTabs_CleanReturnToHome) {
    MockAppLifecycleManager mgr;
    mgr.OpenTab(L"Doc1.pdf", {});
    mgr.OpenTab(L"Doc2.pdf", {});
    mgr.searchBarVisible = true;
    mgr.bookmarkPanelVisible = true;

    EXPECT_EQ(mgr.tabs.size(), 2);
    EXPECT_EQ(mgr.activeTabIndex, 1);
    EXPECT_EQ(mgr.currentMode, components::AppShellMode::Document);

    // Close Tab 1 -> Tab 0 remains active, panels stay open
    mgr.CloseTab(1);
    EXPECT_EQ(mgr.tabs.size(), 1);
    EXPECT_EQ(mgr.activeTabIndex, 0);
    EXPECT_EQ(mgr.currentMode, components::AppShellMode::Document);
    EXPECT_TRUE(mgr.searchBarVisible);
    EXPECT_TRUE(mgr.bookmarkPanelVisible);

    // Close Tab 0 -> All tabs closed -> return to Home, panels cleaned up
    mgr.CloseTab(0);
    EXPECT_EQ(mgr.tabs.size(), 0);
    EXPECT_EQ(mgr.activeTabIndex, -1);
    EXPECT_EQ(mgr.currentMode, components::AppShellMode::Home);
    EXPECT_FALSE(mgr.searchBarVisible);
    EXPECT_FALSE(mgr.bookmarkPanelVisible);
}

TEST(Challenger2_Lifecycle_HomeRequest_StateAndPanelCleanup) {
    MockAppLifecycleManager mgr;
    mgr.OpenTab(L"DocA.pdf", {});
    mgr.searchBarVisible = true;
    mgr.bookmarkPanelVisible = true;

    mgr.RequestHome();
    EXPECT_EQ(mgr.tabs.size(), 0);
    EXPECT_EQ(mgr.activeTabIndex, -1);
    EXPECT_EQ(mgr.currentMode, components::AppShellMode::Home);
    EXPECT_FALSE(mgr.searchBarVisible);
    EXPECT_FALSE(mgr.bookmarkPanelVisible);
}

TEST(Challenger2_Lifecycle_TabSwitching_BookmarkSync) {
    MockAppLifecycleManager mgr;
    mgr.OpenTab(L"DocA.pdf", {});
    mgr.OpenTab(L"DocB.pdf", {});

    EXPECT_EQ(mgr.currentLoadedBookmarkDoc, L"DocB.pdf");

    // Switch back to DocA
    mgr.SwitchToTab(0);
    EXPECT_EQ(mgr.activeTabIndex, 0);
    EXPECT_EQ(mgr.currentLoadedBookmarkDoc, L"DocA.pdf");

    // Switch to DocB
    mgr.SwitchToTab(1);
    EXPECT_EQ(mgr.activeTabIndex, 1);
    EXPECT_EQ(mgr.currentLoadedBookmarkDoc, L"DocB.pdf");
}

int main() {
    std::cout << "Starting Milestone 1 Challenger 2 Empirical Test Suite...\n";
    return TestRunner::Instance().RunAll();
}
