#include "TestFramework.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <memory>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include "../src/pdf_engine/src/PdfiumLibrary.h"
#include "../src/pdf_engine/src/PdfDocument.h"
#include "../src/pdf_engine/src/PdfPage.h"
#include "../src/core/RecentFilesManager.h"
#include "../src/ui/src/AppMode.h"
#include "../src/ui/src/views/HomeView.h"
#include "../src/ui/src/views/DocumentView.h"
#include "../src/ui/src/components/AppShell.h"
#include "../src/ui/src/components/TabBar.h"
#include "../src/ui/src/components/Toolbar.h"
#include "../src/ui/src/components/ModeRail.h"


// ============================================================================
// STRESS TEST GROUP 1: TabBar Boundary Conditions & Index Out-Of-Bounds
// ============================================================================

TEST(M1_TabBar_EmptyTabs_ActiveIndexOutOfBounds) {
    components::TabBar tabBar;
    tabBar.Layout(D2D1::RectF(0, 0, 800, 40));

    // Empty tabs with various out-of-bounds active indices
    tabBar.SetTabs({}, -1);
    tabBar.SetTabs({}, 0);
    tabBar.SetTabs({}, 5);
    tabBar.SetTabs({}, -100);

    int selectedIdx = -999;
    tabBar.SetOnTabSelected([&selectedIdx](int idx) {
        selectedIdx = idx;
    });

    // Clicking anywhere on an empty tab bar should never invoke onTabSelected
    tabBar.OnMouseDown(50.0f, 20.0f);
    tabBar.OnMouseDown(300.0f, 20.0f);
    tabBar.OnMouseDown(-10.0f, 20.0f);
    tabBar.OnMouseDown(900.0f, 20.0f);

    EXPECT_EQ(selectedIdx, -999);
}

TEST(M1_TabBar_MultipleTabs_ClickExactHitAndGaps) {
    components::TabBar tabBar;
    tabBar.Layout(D2D1::RectF(0, 0, 1000, 40));
    // 3 tabs: Tab 0 [8, 268], Gap [268, 276], Tab 1 [276, 536], Gap [536, 544], Tab 2 [544, 804]
    tabBar.SetTabs({L"Doc A", L"Doc B", L"Doc C"}, 0);

    int selectedIdx = -1;
    tabBar.SetOnTabSelected([&selectedIdx](int idx) {
        selectedIdx = idx;
    });

    // 1. Click before Tab 0 (e.g. x = 4.0f)
    selectedIdx = -1;
    tabBar.OnMouseDown(4.0f, 20.0f);
    EXPECT_EQ(selectedIdx, -1);

    // 2. Click inside Tab 0 (e.g. x = 100.0f)
    selectedIdx = -1;
    tabBar.OnMouseDown(100.0f, 20.0f);
    EXPECT_EQ(selectedIdx, 0);

    // 3. Click exactly on gap between Tab 0 and Tab 1 (e.g. x = 272.0f)
    selectedIdx = -1;
    tabBar.OnMouseDown(272.0f, 20.0f);
    EXPECT_EQ(selectedIdx, -1);

    // 4. Click inside Tab 1 (e.g. x = 300.0f)
    selectedIdx = -1;
    tabBar.OnMouseDown(300.0f, 20.0f);
    EXPECT_EQ(selectedIdx, 1);

    // 5. Click inside Tab 2 (e.g. x = 600.0f)
    selectedIdx = -1;
    tabBar.OnMouseDown(600.0f, 20.0f);
    EXPECT_EQ(selectedIdx, 2);

    // 6. Click past Tab 2 (e.g. x = 850.0f)
    selectedIdx = -1;
    tabBar.OnMouseDown(850.0f, 20.0f);
    EXPECT_EQ(selectedIdx, -1);

    // 7. Click negative coordinates
    selectedIdx = -1;
    tabBar.OnMouseDown(-50.0f, -10.0f);
    EXPECT_EQ(selectedIdx, -1);
}

TEST(M1_TabBar_HoverTrackingAcrossTabs) {
    components::TabBar tabBar;
    tabBar.Layout(D2D1::RectF(0, 0, 1000, 40));
    tabBar.SetTabs({L"Doc 1", L"Doc 2"}, 0);

    // Move over Tab 0
    tabBar.OnMouseMove(50.0f, 20.0f);
    // Move over Gap
    tabBar.OnMouseMove(272.0f, 20.0f);
    // Move over Tab 1
    tabBar.OnMouseMove(300.0f, 20.0f);
    // Move outside
    tabBar.OnMouseMove(900.0f, 20.0f);
    EXPECT_TRUE(true);
}

// ============================================================================
// STRESS TEST GROUP 2: AppShell Mode Transitions & TitleBar Controls
// ============================================================================

TEST(M1_AppShell_ModeTransitionsAndWorkspaceVisibility) {
    auto shell = std::make_shared<components::AppShell>();
    auto homeView = std::make_shared<views::HomeView>();
    auto docView = std::make_shared<views::DocumentView>();

    shell->SetHomeContent(homeView);
    shell->SetDocumentWorkspace(docView);
    shell->Layout(D2D1::RectF(0, 0, 1280, 720));

    // Initial default mode is Home
    EXPECT_EQ(shell->GetMode(), components::AppShellMode::Home);
    EXPECT_TRUE(homeView->IsVisible());
    EXPECT_FALSE(docView->IsVisible());

    // Switch to Document mode
    shell->SetMode(components::AppShellMode::Document);
    EXPECT_EQ(shell->GetMode(), components::AppShellMode::Document);
    EXPECT_FALSE(homeView->IsVisible());
    EXPECT_TRUE(docView->IsVisible());

    // Switch back to Home mode
    shell->SetMode(components::AppShellMode::Home);
    EXPECT_EQ(shell->GetMode(), components::AppShellMode::Home);
    EXPECT_TRUE(homeView->IsVisible());
    EXPECT_FALSE(docView->IsVisible());
}

TEST(M1_AppShell_TitleBarControls_HitTestingAndDispatch) {
    auto shell = std::make_shared<components::AppShell>();
    shell->Layout(D2D1::RectF(0, 0, 1000, 600));

    bool minCalled = false;
    bool maxCalled = false;
    bool closeCalled = false;

    shell->onMinimize = [&minCalled]() { minCalled = true; };
    shell->onMaximize = [&maxCalled]() { maxCalled = true; };
    shell->onClose = [&closeCalled]() { closeCalled = true; };

    // Width = 1000. Controls: 3 buttons * 46px = 138px.
    // Minimize: x in [862, 908], y in [0, 32]
    // Maximize: x in [908, 954], y in [0, 32]
    // Close:    x in [954, 1000], y in [0, 32]

    // 1. Test Minimize button
    shell->OnMouseMove(880.0f, 15.0f);
    shell->OnMouseDown(880.0f, 15.0f);
    EXPECT_TRUE(minCalled);
    EXPECT_FALSE(maxCalled);
    EXPECT_FALSE(closeCalled);

    // 2. Test Maximize button
    minCalled = false;
    shell->OnMouseMove(930.0f, 15.0f);
    shell->OnMouseDown(930.0f, 15.0f);
    EXPECT_FALSE(minCalled);
    EXPECT_TRUE(maxCalled);
    EXPECT_FALSE(closeCalled);

    // 3. Test Close button
    maxCalled = false;
    shell->OnMouseMove(980.0f, 15.0f);
    shell->OnMouseDown(980.0f, 15.0f);
    EXPECT_FALSE(minCalled);
    EXPECT_FALSE(maxCalled);
    EXPECT_TRUE(closeCalled);

    // 4. Test Click below title bar (e.g. y = 45.0f)
    closeCalled = false;
    shell->OnMouseMove(980.0f, 45.0f);
    shell->OnMouseDown(980.0f, 45.0f);
    EXPECT_FALSE(closeCalled);
}

#include "../src/ui/src/GraphicsDevice.h"

static Microsoft::WRL::ComPtr<ID2D1RenderTarget> CreateOffscreenRenderTarget(int width, int height) {
    GraphicsDevice::Instance().Initialize();
    auto factory = GraphicsDevice::Instance().GetD2DFactory();
    auto wicFactory = GraphicsDevice::Instance().GetWicFactory();
    if (!factory || !wicFactory) return nullptr;

    Microsoft::WRL::ComPtr<IWICBitmap> wicBitmap;
    HRESULT hr = wicFactory->CreateBitmap(width, height, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wicBitmap);
    if (FAILED(hr) || !wicBitmap) return nullptr;

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0, 0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT
    );
    Microsoft::WRL::ComPtr<ID2D1RenderTarget> rt;
    hr = factory->CreateWicBitmapRenderTarget(wicBitmap.Get(), props, &rt);
    if (FAILED(hr)) return nullptr;
    return rt;
}

// ============================================================================
// STRESS TEST GROUP 3: HomeView Action Dispatch & Quick Tools
// ============================================================================

TEST(M1_HomeView_ButtonsAndAllQuickToolsDispatch) {
    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));

    auto rt = CreateOffscreenRenderTarget(1024, 768);
    EXPECT_TRUE(rt != nullptr);
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    bool openClicked = false;
    bool createClicked = false;
    std::wstring toolRequested;

    homeView.onOpenRequest = [&openClicked]() { openClicked = true; };
    homeView.onCreateRequest = [&createClicked]() { createClicked = true; };
    homeView.onToolRequest = [&toolRequested](const std::wstring& t) { toolRequested = t; };

    // 1. Open PDF button in sidebar (left+12, top+72, right-12, top+116)
    homeView.OnMouseDown(50.0f, 90.0f);
    EXPECT_TRUE(openClicked);

    // 2. Create PDF button in sidebar (left+12, top+124, right-12, top+168)
    homeView.OnMouseDown(50.0f, 140.0f);
    EXPECT_TRUE(createClicked);

    // 3. Quick tools
    const wchar_t* expectedTools[8] = {
        L"Edit PDF", L"Convert PDF", L"OCR PDF", L"Add Comments",
        L"Translate PDF", L"Combine Files", L"Compress PDF", L"Batch PDFs"
    };

    float cols = 4, gap = 16;
    float contentLeft = 240 + 32, contentRight = 1024 - 32;
    float cardW = (contentRight - contentLeft - gap * (cols - 1)) / cols;
    float y = 60;

    for (int i = 0; i < 8; ++i) {
        toolRequested.clear();
        int col = i % 4;
        int row = i / 4;
        float x = contentLeft + col * (cardW + gap) + 30.0f;
        float cy = y + row * (120 + gap) + 30.0f;
        homeView.OnMouseDown(x, cy);
        EXPECT_TRUE(toolRequested == expectedTools[i]);
    }
}


// ============================================================================
// STRESS TEST GROUP 4: Simulated Document Tab Lifecycle & Transitions
// ============================================================================

struct SimulatedTab {
    std::wstring path;
    std::wstring title;
    bool dirty = false;
};

class SimulatedMainWindowManager {
public:
    std::vector<SimulatedTab> tabs;
    int activeTabIndex = -1;
    components::AppShellMode currentMode = components::AppShellMode::Home;
    app::AppMode activeAppMode = app::AppMode::View;

    void OpenDirect(const std::wstring& path) {
        SimulatedTab tab;
        tab.path = path;
        size_t slashPos = path.find_last_of(L"/\\");
        tab.title = (slashPos != std::wstring::npos) ? path.substr(slashPos + 1) : path;
        tab.dirty = false;
        tabs.push_back(tab);
        activeTabIndex = static_cast<int>(tabs.size()) - 1;
        currentMode = components::AppShellMode::Document;
    }

    void HandleToolRequest(const std::wstring& toolName, bool userSelectsFile, const std::wstring& selectedPath) {
        auto openAndSetMode = [this, userSelectsFile, selectedPath](app::AppMode mode) {
            size_t prevCount = tabs.size();
            if (userSelectsFile && !selectedPath.empty()) {
                OpenDirect(selectedPath);
            }
            if (tabs.size() > prevCount && activeTabIndex >= 0) {
                activeAppMode = mode;
            }
        };

        if (toolName == L"Edit PDF") openAndSetMode(app::AppMode::Edit);
        else if (toolName == L"Add Comments") openAndSetMode(app::AppMode::Comment);
        else if (toolName == L"Convert PDF") openAndSetMode(app::AppMode::Convert);
        else if (toolName == L"OCR PDF") openAndSetMode(app::AppMode::Tools);
        else if (toolName == L"Translate PDF") openAndSetMode(app::AppMode::Tools);
        else if (toolName == L"Compress PDF") openAndSetMode(app::AppMode::Tools);
    }

    void SwitchToTab(int index) {
        if (index < 0 || index >= static_cast<int>(tabs.size())) return;
        activeTabIndex = index;
    }

    void CloseTab(int tabIndex) {
        if (tabIndex < 0 || tabIndex >= static_cast<int>(tabs.size())) return;

        tabs.erase(tabs.begin() + tabIndex);

        if (tabs.empty()) {
            activeTabIndex = -1;
            currentMode = components::AppShellMode::Home;
        } else {
            if (activeTabIndex >= static_cast<int>(tabs.size())) {
                activeTabIndex = static_cast<int>(tabs.size() - 1);
            }
        }
    }

    void RequestHome() {
        bool canGoHome = true;
        for (const auto& tab : tabs) {
            if (tab.dirty) { canGoHome = false; break; }
        }
        if (!canGoHome) return;

        tabs.clear();
        activeTabIndex = -1;
        currentMode = components::AppShellMode::Home;
    }
};

TEST(M1_Lifecycle_ToolCancellation_PreservesHomeState) {
    SimulatedMainWindowManager mgr;
    EXPECT_EQ(mgr.currentMode, components::AppShellMode::Home);
    EXPECT_EQ(mgr.activeTabIndex, -1);
    EXPECT_EQ(mgr.tabs.size(), 0);

    // User clicks "Edit PDF" but cancels file open dialog
    mgr.HandleToolRequest(L"Edit PDF", false, L"");
    EXPECT_EQ(mgr.currentMode, components::AppShellMode::Home);
    EXPECT_EQ(mgr.activeTabIndex, -1);
    EXPECT_EQ(mgr.tabs.size(), 0);
    EXPECT_EQ(mgr.activeAppMode, app::AppMode::View);

    // User clicks "Add Comments" but cancels file open dialog
    mgr.HandleToolRequest(L"Add Comments", false, L"");
    EXPECT_EQ(mgr.currentMode, components::AppShellMode::Home);
    EXPECT_EQ(mgr.activeTabIndex, -1);
    EXPECT_EQ(mgr.tabs.size(), 0);
}

TEST(M1_Lifecycle_ToolSuccess_TransitionsToDocumentAndToolMode) {
    SimulatedMainWindowManager mgr;

    // User clicks "Edit PDF" and selects a valid file
    mgr.HandleToolRequest(L"Edit PDF", true, L"C:\\Documents\\Report.pdf");
    EXPECT_EQ(mgr.currentMode, components::AppShellMode::Document);
    EXPECT_EQ(mgr.activeTabIndex, 0);
    EXPECT_EQ(mgr.tabs.size(), 1);
    EXPECT_EQ(mgr.tabs[0].title, L"Report.pdf");
    EXPECT_EQ(mgr.activeAppMode, app::AppMode::Edit);
}

TEST(M1_Lifecycle_MultiTab_SwitchingAndOutOfBoundsSafety) {
    SimulatedMainWindowManager mgr;
    mgr.OpenDirect(L"DocA.pdf");
    mgr.OpenDirect(L"DocB.pdf");
    mgr.OpenDirect(L"DocC.pdf");

    EXPECT_EQ(mgr.tabs.size(), 3);
    EXPECT_EQ(mgr.activeTabIndex, 2); // Active is DocC

    // Switch to DocA (index 0)
    mgr.SwitchToTab(0);
    EXPECT_EQ(mgr.activeTabIndex, 0);

    // Switch to DocB (index 1)
    mgr.SwitchToTab(1);
    EXPECT_EQ(mgr.activeTabIndex, 1);

    // Out-of-bounds switches should be ignored safely
    mgr.SwitchToTab(-1);
    EXPECT_EQ(mgr.activeTabIndex, 1);

    mgr.SwitchToTab(3);
    EXPECT_EQ(mgr.activeTabIndex, 1);

    mgr.SwitchToTab(99);
    EXPECT_EQ(mgr.activeTabIndex, 1);
}

TEST(M1_Lifecycle_CloseTabs_SequentialToHome_AndReopen) {
    SimulatedMainWindowManager mgr;
    mgr.OpenDirect(L"Doc1.pdf");
    mgr.OpenDirect(L"Doc2.pdf");

    EXPECT_EQ(mgr.tabs.size(), 2);
    EXPECT_EQ(mgr.activeTabIndex, 1);

    // Close invalid index
    mgr.CloseTab(-1);
    mgr.CloseTab(5);
    EXPECT_EQ(mgr.tabs.size(), 2);

    // Close active tab (Doc2 at index 1) -> active becomes index 0 (Doc1)
    mgr.CloseTab(1);
    EXPECT_EQ(mgr.tabs.size(), 1);
    EXPECT_EQ(mgr.activeTabIndex, 0);
    EXPECT_EQ(mgr.currentMode, components::AppShellMode::Document);

    // Close remaining tab (Doc1 at index 0) -> tabs empty -> transitions to Home
    mgr.CloseTab(0);
    EXPECT_EQ(mgr.tabs.size(), 0);
    EXPECT_EQ(mgr.activeTabIndex, -1);
    EXPECT_EQ(mgr.currentMode, components::AppShellMode::Home);

    // Reopen new document after returning to Home
    mgr.OpenDirect(L"NewDoc.pdf");
    EXPECT_EQ(mgr.tabs.size(), 1);
    EXPECT_EQ(mgr.activeTabIndex, 0);
    EXPECT_EQ(mgr.currentMode, components::AppShellMode::Document);
    EXPECT_EQ(mgr.tabs[0].title, L"NewDoc.pdf");
}

TEST(M1_Lifecycle_HomeRequest_CleanVsDirty) {
    SimulatedMainWindowManager mgr;
    mgr.OpenDirect(L"CleanDoc.pdf");
    EXPECT_EQ(mgr.currentMode, components::AppShellMode::Document);

    // Request Home with clean document -> succeeds and transitions to Home
    mgr.RequestHome();
    EXPECT_EQ(mgr.currentMode, components::AppShellMode::Home);
    EXPECT_EQ(mgr.tabs.size(), 0);
    EXPECT_EQ(mgr.activeTabIndex, -1);

    // Open another document and mark dirty
    mgr.OpenDirect(L"DirtyDoc.pdf");
    mgr.tabs[0].dirty = true;

    // Request Home with dirty document -> should NOT clear tabs or change mode
    mgr.RequestHome();
    EXPECT_EQ(mgr.currentMode, components::AppShellMode::Document);
    EXPECT_EQ(mgr.tabs.size(), 1);
    EXPECT_EQ(mgr.activeTabIndex, 0);
}

#include "../src/ui/src/components/SearchBar.h"
#include "../src/ui/src/components/BookmarkPanel.h"

// ============================================================================
// ADVERSARIAL CHALLENGE GROUP 5: TabBar Close Button & Boundary Precision
// ============================================================================

TEST(M1_Adversarial_TabBar_CloseButton_ExactBoundaries) {
    components::TabBar tabBar;
    // Layout bounds: [100, 0, 1000, 40]
    tabBar.Layout(D2D1::RectF(100.0f, 0.0f, 1000.0f, 40.0f));
    // 3 tabs:
    // Tab 0: [108, 368], Select area: [108, 340), Close area: [340, 368]
    // Gap 0-1: (368, 376)
    // Tab 1: [376, 636], Select area: [376, 608), Close area: [608, 636]
    // Gap 1-2: (636, 644)
    // Tab 2: [644, 904], Select area: [644, 876), Close area: [876, 904]
    tabBar.SetTabs({L"Doc Alpha", L"Doc Beta", L"Doc Gamma"}, 0);

    int selectedIdx = -1;
    int closedIdx = -1;

    tabBar.SetOnTabSelected([&selectedIdx](int idx) { selectedIdx = idx; });
    tabBar.SetOnTabClosed([&closedIdx](int idx) { closedIdx = idx; });

    auto reset = [&]() { selectedIdx = -1; closedIdx = -1; };

    // 1. Outside left of Tab 0
    reset();
    tabBar.OnMouseDown(107.99f, 20.0f);
    EXPECT_EQ(selectedIdx, -1);
    EXPECT_EQ(closedIdx, -1);

    // 2. Exact left edge of Tab 0 -> Select Tab 0
    reset();
    tabBar.OnMouseDown(108.0f, 20.0f);
    EXPECT_EQ(selectedIdx, 0);
    EXPECT_EQ(closedIdx, -1);

    // 3. Sub-pixel just before close button (339.99f) -> Select Tab 0
    reset();
    tabBar.OnMouseDown(339.99f, 20.0f);
    EXPECT_EQ(selectedIdx, 0);
    EXPECT_EQ(closedIdx, -1);

    // 4. Exact boundary of close button (340.0f) -> Close Tab 0
    reset();
    tabBar.OnMouseDown(340.0f, 20.0f);
    EXPECT_EQ(selectedIdx, -1);
    EXPECT_EQ(closedIdx, 0);

    // 5. Middle of close button (354.0f) -> Close Tab 0
    reset();
    tabBar.OnMouseDown(354.0f, 20.0f);
    EXPECT_EQ(selectedIdx, -1);
    EXPECT_EQ(closedIdx, 0);

    // 6. Exact right edge of Tab 0 (368.0f) -> Close Tab 0
    reset();
    tabBar.OnMouseDown(368.0f, 20.0f);
    EXPECT_EQ(selectedIdx, -1);
    EXPECT_EQ(closedIdx, 0);

    // 7. Sub-pixel into gap (368.01f) -> No action
    reset();
    tabBar.OnMouseDown(368.01f, 20.0f);
    EXPECT_EQ(selectedIdx, -1);
    EXPECT_EQ(closedIdx, -1);

    // 8. Middle of gap (372.0f) -> No action
    reset();
    tabBar.OnMouseDown(372.0f, 20.0f);
    EXPECT_EQ(selectedIdx, -1);
    EXPECT_EQ(closedIdx, -1);

    // 9. End of gap (375.99f) -> No action
    reset();
    tabBar.OnMouseDown(375.99f, 20.0f);
    EXPECT_EQ(selectedIdx, -1);
    EXPECT_EQ(closedIdx, -1);

    // 10. Start of Tab 1 (376.0f) -> Select Tab 1
    reset();
    tabBar.OnMouseDown(376.0f, 20.0f);
    EXPECT_EQ(selectedIdx, 1);
    EXPECT_EQ(closedIdx, -1);

    // 11. Tab 1 close button boundary (608.0f) -> Close Tab 1
    reset();
    tabBar.OnMouseDown(608.0f, 20.0f);
    EXPECT_EQ(selectedIdx, -1);
    EXPECT_EQ(closedIdx, 1);

    // 12. Tab 2 close button boundary (876.0f) -> Close Tab 2
    reset();
    tabBar.OnMouseDown(876.0f, 20.0f);
    EXPECT_EQ(selectedIdx, -1);
    EXPECT_EQ(closedIdx, 2);

    // 13. Past last tab (904.01f) -> No action
    reset();
    tabBar.OnMouseDown(904.01f, 20.0f);
    EXPECT_EQ(selectedIdx, -1);
    EXPECT_EQ(closedIdx, -1);
}

TEST(M1_Adversarial_TabBar_ActiveVsInactiveCloseSeparation) {
    components::TabBar tabBar;
    tabBar.Layout(D2D1::RectF(0, 0, 1000, 40));
    tabBar.SetTabs({L"Doc 0", L"Doc 1", L"Doc 2"}, 0); // Active is 0

    int selectedIdx = -1;
    int closedIdx = -1;
    tabBar.SetOnTabSelected([&selectedIdx](int idx) { selectedIdx = idx; });
    tabBar.SetOnTabClosed([&closedIdx](int idx) { closedIdx = idx; });

    // Click close on inactive Tab 1 -> Tab 1 is closed, Tab 1 is NOT selected
    tabBar.OnMouseDown(8.0f + 268.0f + 240.0f, 20.0f);
    EXPECT_EQ(closedIdx, 1);
    EXPECT_EQ(selectedIdx, -1);

    // Click close on active Tab 0 -> Tab 0 is closed, no selection event
    closedIdx = -1;
    selectedIdx = -1;
    tabBar.OnMouseDown(8.0f + 240.0f, 20.0f);
    EXPECT_EQ(closedIdx, 0);
    EXPECT_EQ(selectedIdx, -1);
}

TEST(M1_Adversarial_TabBar_HighVolumeStress_1000Cycles) {
    components::TabBar tabBar;
    tabBar.Layout(D2D1::RectF(0, 0, 3000, 40));

    std::vector<std::wstring> titles;
    for (int i = 0; i < 10; ++i) {
        titles.push_back(L"Tab " + std::to_wstring(i));
    }
    tabBar.SetTabs(titles, 0);

    int selectCount = 0;
    int closeCount = 0;
    int lastSelected = -1;
    int lastClosed = -1;

    tabBar.SetOnTabSelected([&](int idx) { selectCount++; lastSelected = idx; });
    tabBar.SetOnTabClosed([&](int idx) { closeCount++; lastClosed = idx; });

    for (int cycle = 0; cycle < 1000; ++cycle) {
        int tabIdx = cycle % 10;
        float currentX = 8.0f + tabIdx * 268.0f;
        if (cycle % 2 == 0) {
            // Click body
            tabBar.OnMouseDown(currentX + 50.0f, 20.0f);
            EXPECT_EQ(lastSelected, tabIdx);
        } else {
            // Click close
            tabBar.OnMouseDown(currentX + 245.0f, 20.0f);
            EXPECT_EQ(lastClosed, tabIdx);
        }
    }

    EXPECT_EQ(selectCount, 500);
    EXPECT_EQ(closeCount, 500);
}

// ============================================================================
// ADVERSARIAL CHALLENGE GROUP 6: HomeView Sidebar Navigation & Precision
// ============================================================================

TEST(M1_Adversarial_HomeView_SidebarNav_SelectionAndCallbacks) {
    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));

    auto rt = CreateOffscreenRenderTarget(1024, 768);
    EXPECT_TRUE(rt != nullptr);
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    int requestedNav = -1;
    homeView.onNavRequest = [&requestedNav](int navIdx) {
        requestedNav = navIdx;
    };

    // Initial state
    EXPECT_EQ(homeView.GetSelectedNav(), 0);

    // Sidebar width is 240. Nav items: left = 8, right = 232.
    // Nav 0 (Recent Files): y in [200, 236]
    // Nav 1 (Starred Files): y in [240, 276]
    // Nav 2 (Recent Folders): y in [280, 316]

    // 1. Click Nav 1 (Starred Files) -> (100, 250)
    requestedNav = -1;
    homeView.OnMouseDown(100.0f, 250.0f);
    EXPECT_EQ(requestedNav, 1);
    EXPECT_EQ(homeView.GetSelectedNav(), 1);

    // 2. Click Nav 2 (Recent Folders) -> (100, 290)
    requestedNav = -1;
    homeView.OnMouseDown(100.0f, 290.0f);
    EXPECT_EQ(requestedNav, 2);
    EXPECT_EQ(homeView.GetSelectedNav(), 2);

    // 3. Click Nav 0 (Recent Files) -> (100, 210)
    requestedNav = -1;
    homeView.OnMouseDown(100.0f, 210.0f);
    EXPECT_EQ(requestedNav, 0);
    EXPECT_EQ(homeView.GetSelectedNav(), 0);

    // 4. Click gap between Nav 0 and Nav 1 (y = 238) -> Selection unchanged, no callback
    requestedNav = -1;
    homeView.OnMouseDown(100.0f, 238.0f);
    EXPECT_EQ(requestedNav, -1);
    EXPECT_EQ(homeView.GetSelectedNav(), 0);

    // 5. Click gap between Nav 1 and Nav 2 (y = 278) -> Selection unchanged, no callback
    requestedNav = -1;
    homeView.OnMouseDown(100.0f, 278.0f);
    EXPECT_EQ(requestedNav, -1);
    EXPECT_EQ(homeView.GetSelectedNav(), 0);

    // 6. Click above nav items (y = 180) -> Selection unchanged
    requestedNav = -1;
    homeView.OnMouseDown(100.0f, 180.0f);
    EXPECT_EQ(requestedNav, -1);
    EXPECT_EQ(homeView.GetSelectedNav(), 0);

    // 7. Click below nav items (y = 330) -> Selection unchanged
    requestedNav = -1;
    homeView.OnMouseDown(100.0f, 330.0f);
    EXPECT_EQ(requestedNav, -1);
    EXPECT_EQ(homeView.GetSelectedNav(), 0);

    // 8. Click outside sidebar in main area (x = 350, y = 250) -> Selection unchanged
    requestedNav = -1;
    homeView.OnMouseDown(350.0f, 250.0f);
    EXPECT_EQ(requestedNav, -1);
    EXPECT_EQ(homeView.GetSelectedNav(), 0);
}

TEST(M1_Adversarial_HomeView_SidebarNav_HoverTrackingAndPersistence) {
    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));

    auto rt = CreateOffscreenRenderTarget(1024, 768);
    EXPECT_TRUE(rt != nullptr);
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    // Set selection explicitly
    homeView.SetSelectedNav(2);
    EXPECT_EQ(homeView.GetSelectedNav(), 2);

    // Re-render
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }
    EXPECT_EQ(homeView.GetSelectedNav(), 2);

    // Hover Nav 0
    homeView.OnMouseMove(100.0f, 210.0f);
    // Hover outside
    homeView.OnMouseMove(500.0f, 500.0f);
    // Selection state is still preserved
    EXPECT_EQ(homeView.GetSelectedNav(), 2);
}

// ============================================================================
// ADVERSARIAL CHALLENGE GROUP 7: SearchBar Win32 Close & Key Handlers
// ============================================================================

TEST(M1_Adversarial_SearchBar_CloseButtonAndEscapeKey) {
    // Create a test parent window for testing child Win32 control
    WNDCLASS wc = {0};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"M1TestSearchBarParent";
    RegisterClass(&wc);

    HWND parentHwnd = CreateWindowEx(
        0, wc.lpszClassName, L"TestParent",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 600, 400,
        nullptr, nullptr, wc.hInstance, nullptr
    );
    EXPECT_TRUE(parentHwnd != nullptr);
    ShowWindow(parentHwnd, SW_SHOW);

    SearchBar searchBar;
    bool created = searchBar.Create(parentHwnd);
    EXPECT_TRUE(created);

    bool closeTriggered = false;
    bool nextTriggered = false;
    bool prevTriggered = false;
    std::wstring searchQuery;

    searchBar.SetOnCloseCallback([&]() {
        closeTriggered = true;
        searchBar.Hide();
    });
    searchBar.SetOnNextCallback([&]() { nextTriggered = true; });
    searchBar.SetOnPrevCallback([&]() { prevTriggered = true; });
    searchBar.SetOnSearchCallback([&](const std::wstring& q) { searchQuery = q; });

    // 1. Show SearchBar
    searchBar.Show();
    EXPECT_TRUE(searchBar.IsVisible());

    // 2. Simulate clicking 'X' button (sends WM_COMMAND with ID_CLOSE = 104)
    closeTriggered = false;
    HWND searchBarHwnd = FindWindowEx(parentHwnd, nullptr, L"PdfEliteSearchBar", nullptr);
    EXPECT_TRUE(searchBarHwnd != nullptr);

    SendMessage(searchBarHwnd, WM_COMMAND, MAKEWPARAM(104, BN_CLICKED), 0);
    EXPECT_TRUE(closeTriggered);
    EXPECT_FALSE(searchBar.IsVisible());

    // 3. Show again and test Escape key via EditSubclassProc
    searchBar.Show();
    EXPECT_TRUE(searchBar.IsVisible());
    closeTriggered = false;

    HWND editHwnd = FindWindowEx(searchBarHwnd, nullptr, L"EDIT", nullptr);
    EXPECT_TRUE(editHwnd != nullptr);

    // Send VK_ESCAPE to edit control
    SendMessage(editHwnd, WM_KEYDOWN, VK_ESCAPE, 0);
    EXPECT_TRUE(closeTriggered);
    EXPECT_FALSE(searchBar.IsVisible());

    // 4. Test Enter key navigation (VK_RETURN -> onNext)
    nextTriggered = false;
    SendMessage(editHwnd, WM_KEYDOWN, VK_RETURN, 0);
    EXPECT_TRUE(nextTriggered);

    // 5. Test typing (WM_KEYUP -> onSearch)
    SetWindowText(editHwnd, L"AdversarialTest");
    searchQuery.clear();
    SendMessage(editHwnd, WM_KEYUP, 'T', 0);
    EXPECT_TRUE(searchQuery == L"AdversarialTest");

    // 6. Test Result Count formatting
    searchBar.SetResultCount(42, 5);

    if (parentHwnd) DestroyWindow(parentHwnd);
}

// ============================================================================
// ADVERSARIAL CHALLENGE GROUP 8: BookmarkPanel Win32 HWND Creation & State
// ============================================================================

TEST(M1_Adversarial_BookmarkPanel_CreationAndTreeLifecycle) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"M1TestBookmarkParent";
    RegisterClass(&wc);

    HWND parentHwnd = CreateWindowEx(
        0, wc.lpszClassName, L"TestParent",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 600, 400,
        nullptr, nullptr, wc.hInstance, nullptr
    );
    EXPECT_TRUE(parentHwnd != nullptr);
    ShowWindow(parentHwnd, SW_SHOW);

    components::BookmarkPanel bookmarkPanel;
    bool created = bookmarkPanel.Create(parentHwnd);
    EXPECT_TRUE(created);
    EXPECT_TRUE(bookmarkPanel.GetHwnd() != nullptr);

    // Initially hidden
    bookmarkPanel.Hide();
    EXPECT_FALSE(bookmarkPanel.IsVisible());

    bookmarkPanel.Show();
    EXPECT_TRUE(bookmarkPanel.IsVisible());

    bookmarkPanel.SetBounds(0, 0, 200, 400);

    // Create bookmark hierarchy
    std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> bookmarks;
    auto bm1 = std::make_unique<core::interfaces::dom::PdfBookmark>();
    bm1->title = L"Chapter 1: Overview";
    bm1->destination = core::interfaces::dom::NavigationTarget{0, 0.0f, 0.0f, 1.0f};

    auto child1 = std::make_unique<core::interfaces::dom::PdfBookmark>();
    child1->title = L"1.1 Introduction";
    child1->destination = core::interfaces::dom::NavigationTarget{1, 0.0f, 0.0f, 1.0f};
    bm1->children.push_back(std::move(child1));

    bookmarks.push_back(std::move(bm1));

    bookmarkPanel.LoadBookmarks(bookmarks);

    // Clear bookmarks
    std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> empty;
    bookmarkPanel.LoadBookmarks(empty);

    bookmarkPanel.Hide();
    EXPECT_FALSE(bookmarkPanel.IsVisible());

    if (parentHwnd) DestroyWindow(parentHwnd);
}

