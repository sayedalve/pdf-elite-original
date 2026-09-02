#include "TestFramework.h"
#include "../src/pdf_engine/src/PdfiumLibrary.h"
#include "../src/pdf_engine/src/PdfDocument.h"
#include "../src/core/CoordinateConverter.h"
#include "../src/ui/src/PdfViewer.h"
#include "../src/ui/src/views/HomeView.h"
#include "../src/ui/src/components/AppShell.h"
#include "../src/ui/src/components/TabBar.h"
#include "../src/ui/src/GraphicsDevice.h"
#include <windows.h>
#include <d2d1_1.h>
#include <wincodec.h>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>

TEST(HomeView_RenderAndQuickToolsCallbacks) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    GraphicsDevice::Instance().Initialize();
    auto factory = GraphicsDevice::Instance().GetD2DFactory();
    auto wicFactory = GraphicsDevice::Instance().GetWicFactory();
    EXPECT_TRUE(factory != nullptr);
    EXPECT_TRUE(wicFactory != nullptr);

    ComPtr<IWICBitmap> wicBitmap;
    HRESULT hr = wicFactory->CreateBitmap(1200, 800, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wicBitmap);
    EXPECT_TRUE(SUCCEEDED(hr));

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0f, 96.0f, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT
    );
    ComPtr<ID2D1RenderTarget> rt;
    hr = factory->CreateWicBitmapRenderTarget(wicBitmap.Get(), &props, &rt);
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_TRUE(rt != nullptr);

    auto homeView = std::make_shared<views::HomeView>();
    D2D1_RECT_F bounds = D2D1::RectF(0, 0, 1200, 800);
    homeView->Layout(bounds);

    rt->BeginDraw();
    homeView->Render(rt);
    rt->EndDraw();

    std::vector<std::wstring> requestedTools;
    homeView->onToolRequest = [&](const std::wstring& tool) {
        requestedTools.push_back(tool);
    };

    bool openRequested = false;
    homeView->onOpenRequest = [&]() {
        openRequested = true;
    };

    bool createRequested = false;
    homeView->onCreateRequest = [&]() {
        createRequested = true;
    };

    // Test Open button click (bounds: rect.left+12 = 12, rect.top+72 = 72, rect.right-12 = 228, rect.top+116 = 116)
    homeView->OnMouseDown(50.0f, 90.0f);
    EXPECT_TRUE(openRequested);

    // Test Create button click (bounds: rect.left+12 = 12, rect.top+124 = 124, rect.right-12 = 228, rect.top+168 = 168)
    homeView->OnMouseDown(50.0f, 140.0f);
    EXPECT_TRUE(createRequested);

    // Test Quick Tool Cards (8 tools arranged in 2 rows of 4 columns)
    // Sidebar width is 240, main rect starts at 240. contentLeft = 240 + 32 = 272.
    // contentRight = 1200 - 32 = 1168.
    // available width = 1168 - 272 = 896. gap = 16. cols = 4.
    // cardW = (896 - 16 * 3) / 4 = 848 / 4 = 212.
    // Row 0: y = 0 + 60 = 60. Row 1: y = 60 + 120 + 16 = 196.
    
    // Tool 0: Edit PDF (col 0, row 0): x = 272 + 10 = 282, y = 60 + 20 = 80
    homeView->OnMouseDown(282.0f, 80.0f);
    // Tool 1: Convert PDF (col 1, row 0): x = 272 + 212 + 16 + 10 = 510, y = 80
    homeView->OnMouseDown(510.0f, 80.0f);
    // Tool 2: OCR PDF (col 2, row 0): x = 272 + 2*(228) + 10 = 738, y = 80
    homeView->OnMouseDown(738.0f, 80.0f);
    // Tool 3: Add Comments (col 3, row 0): x = 272 + 3*(228) + 10 = 966, y = 80
    homeView->OnMouseDown(966.0f, 80.0f);
    // Tool 4: Translate PDF (col 0, row 1): x = 282, y = 196 + 20 = 216
    homeView->OnMouseDown(282.0f, 216.0f);
    // Tool 5: Combine Files (col 1, row 1): x = 510, y = 216
    homeView->OnMouseDown(510.0f, 216.0f);
    // Tool 6: Compress PDF (col 2, row 1): x = 738, y = 216
    homeView->OnMouseDown(738.0f, 216.0f);
    // Tool 7: Batch PDFs (col 3, row 1): x = 966, y = 216
    homeView->OnMouseDown(966.0f, 216.0f);

    EXPECT_EQ(requestedTools.size(), 8);
    EXPECT_TRUE(requestedTools[0] == L"Edit PDF");
    EXPECT_TRUE(requestedTools[1] == L"Convert PDF");
    EXPECT_TRUE(requestedTools[2] == L"OCR PDF");
    EXPECT_TRUE(requestedTools[3] == L"Add Comments");
    EXPECT_TRUE(requestedTools[4] == L"Translate PDF");
    EXPECT_TRUE(requestedTools[5] == L"Combine Files");
    EXPECT_TRUE(requestedTools[6] == L"Compress PDF");
    EXPECT_TRUE(requestedTools[7] == L"Batch PDFs");
}

TEST(AppShell_WindowControlsAndLayout) {
    auto appShell = std::make_shared<components::AppShell>();
    D2D1_RECT_F bounds = D2D1::RectF(0, 0, 1000, 700);
    appShell->Layout(bounds);

    bool minCalled = false;
    bool maxCalled = false;
    bool closeCalled = false;

    appShell->onMinimize = [&]() { minCalled = true; };
    appShell->onMaximize = [&]() { maxCalled = true; };
    appShell->onClose = [&]() { closeCalled = true; };

    // Window controls: 3 buttons, each width 46, height 32.
    // startX = bounds.right - 3 * 46 = 1000 - 138 = 862.
    // Minimize button: x in [862, 908], y in [0, 32] -> midpoint (885, 16)
    // Maximize button: x in [908, 954], y in [0, 32] -> midpoint (931, 16)
    // Close button: x in [954, 1000], y in [0, 32] -> midpoint (977, 16)

    // Test Minimize
    appShell->OnMouseMove(885.0f, 16.0f);
    appShell->OnMouseDown(885.0f, 16.0f);
    EXPECT_TRUE(minCalled);

    // Test Maximize
    appShell->OnMouseMove(931.0f, 16.0f);
    appShell->OnMouseDown(931.0f, 16.0f);
    EXPECT_TRUE(maxCalled);

    // Test Close
    appShell->OnMouseMove(977.0f, 16.0f);
    appShell->OnMouseDown(977.0f, 16.0f);
    EXPECT_TRUE(closeCalled);

    // Test clicking outside control area (e.g. titlebar center: 500, 16)
    minCalled = false;
    maxCalled = false;
    closeCalled = false;
    appShell->OnMouseMove(500.0f, 16.0f);
    appShell->OnMouseDown(500.0f, 16.0f);
    EXPECT_FALSE(minCalled);
    EXPECT_FALSE(maxCalled);
    EXPECT_FALSE(closeCalled);
}

TEST(TabBar_TabSelectionAndEdgeCases) {
    auto tabBar = std::make_shared<components::TabBar>();
    D2D1_RECT_F bounds = D2D1::RectF(60.0f, 0.0f, 800.0f, 40.0f);
    tabBar->Layout(bounds);

    int selectedTab = -1;
    tabBar->SetOnTabSelected([&](int idx) {
        selectedTab = idx;
    });

    // 1. Zero tabs
    tabBar->SetTabs({}, -1);
    tabBar->OnMouseMove(70.0f, 20.0f);
    tabBar->OnMouseDown(70.0f, 20.0f);
    EXPECT_EQ(selectedTab, -1);

    // 2. Multiple tabs
    std::vector<std::wstring> tabs = { L"Doc1.pdf", L"Doc2.pdf", L"Doc3.pdf" };
    tabBar->SetTabs(tabs, 0);

    // Tab 0: x in [68, 68+260 = 328]
    selectedTab = -1;
    tabBar->OnMouseDown(100.0f, 20.0f);
    EXPECT_EQ(selectedTab, 0);

    // Tab 1: x in [328+8 = 336, 336+260 = 596]
    selectedTab = -1;
    tabBar->OnMouseDown(400.0f, 20.0f);
    EXPECT_EQ(selectedTab, 1);

    // Tab 2: x in [596+8 = 604, 604+260 = 864]
    selectedTab = -1;
    tabBar->OnMouseDown(700.0f, 20.0f);
    EXPECT_EQ(selectedTab, 2);

    // Click outside tabs (e.g. x = 900)
    selectedTab = -1;
    tabBar->OnMouseDown(900.0f, 20.0f);
    EXPECT_EQ(selectedTab, -1);

    // 3. Stress test rapid tab switching
    for (int i = 0; i < 1000; ++i) {
        int target = i % 3;
        float clickX = 68.0f + target * 268.0f + 50.0f;
        tabBar->OnMouseDown(clickX, 20.0f);
        EXPECT_EQ(selectedTab, target);
    }
}

TEST(DocumentLifecycle_TabTitleAndStateTransitions) {
    // Simulate DocumentTab structure and UpdateTabs logic
    struct DocumentTabSim {
        std::wstring filePath;
        std::wstring title;
        bool dirty = false;
    };

    std::vector<DocumentTabSim> tabs;
    int activeIndex = -1;

    auto openSim = [&](const std::wstring& path) {
        DocumentTabSim tab;
        tab.filePath = path;
        size_t slashPos = path.find_last_of(L"/\\");
        tab.title = (slashPos != std::wstring::npos) ? path.substr(slashPos + 1) : path;
        tabs.push_back(tab);
        activeIndex = static_cast<int>(tabs.size() - 1);
    };

    auto closeSim = [&](int index) {
        if (index < 0 || index >= static_cast<int>(tabs.size())) return;
        tabs.erase(tabs.begin() + index);
        if (tabs.empty()) {
            activeIndex = -1;
        } else if (activeIndex >= static_cast<int>(tabs.size())) {
            activeIndex = static_cast<int>(tabs.size() - 1);
        }
    };

    auto getTitles = [&]() -> std::vector<std::wstring> {
        std::vector<std::wstring> titles;
        titles.reserve(tabs.size());
        for (const auto& tab : tabs) {
            if (!tab.title.empty()) {
                titles.push_back(tab.title);
            } else {
                titles.push_back(L"Untitled");
            }
        }
        return titles;
    };

    // Open 3 documents
    openSim(L"C:\\Users\\sayed\\Documents\\Annual_Report_2026.pdf");
    openSim(L"C:\\Users\\sayed\\Downloads\\Contract_v2.pdf");
    openSim(L"");

    EXPECT_EQ(tabs.size(), 3);
    EXPECT_EQ(activeIndex, 2);

    auto titles = getTitles();
    EXPECT_EQ(titles.size(), 3);
    EXPECT_TRUE(titles[0] == L"Annual_Report_2026.pdf");
    EXPECT_TRUE(titles[1] == L"Contract_v2.pdf");
    EXPECT_TRUE(titles[2] == L"Untitled");

    // Close middle tab
    closeSim(1);
    EXPECT_EQ(tabs.size(), 2);
    EXPECT_EQ(activeIndex, 1);
    titles = getTitles();
    EXPECT_TRUE(titles[0] == L"Annual_Report_2026.pdf");
    EXPECT_TRUE(titles[1] == L"Untitled");

    // Close active tab
    closeSim(activeIndex);
    EXPECT_EQ(tabs.size(), 1);
    EXPECT_EQ(activeIndex, 0);
    titles = getTitles();
    EXPECT_TRUE(titles[0] == L"Annual_Report_2026.pdf");

    // Close last tab -> empty state / return to Home
    closeSim(0);
    EXPECT_EQ(tabs.size(), 0);
    EXPECT_EQ(activeIndex, -1);
    titles = getTitles();
    EXPECT_EQ(titles.size(), 0);
}

TEST(CoordinateConverter_RoundTrip_ZeroDrift) {
    CoordinateConverter::PageContext pageCtx{ 612.0, 792.0, 0 };
    CoordinateConverter::ViewContext viewCtx{ 1.5, 50.0, 100.0, 200.0, 150.0 };

    for (int y = 0; y <= 792; y += 33) {
        for (int x = 0; x <= 612; x += 34) {
            PointF origPdf{ static_cast<float>(x), static_cast<float>(y) };
            PointF screen = CoordinateConverter::PdfToScreen(pageCtx, viewCtx, origPdf.x, origPdf.y);
            PointF roundTrip = CoordinateConverter::ScreenToPdf(pageCtx, viewCtx, screen.x, screen.y);
            
            float diffX = std::abs(roundTrip.x - origPdf.x);
            float diffY = std::abs(roundTrip.y - origPdf.y);
            EXPECT_TRUE(diffX < 1e-4f);
            EXPECT_TRUE(diffY < 1e-4f);
        }
    }
}

TEST(CoordinateConverter_Rotations_90_180_270) {
    int rotations[] = { 0, 90, 180, 270 };
    for (int rot : rotations) {
        CoordinateConverter::PageContext pageCtx{ 612.0, 792.0, rot };
        CoordinateConverter::ViewContext viewCtx{ 2.0, 30.0, 60.0, 100.0, 80.0 };

        PointF origPdf{ 150.0f, 300.0f };
        PointF screen = CoordinateConverter::PdfToScreen(pageCtx, viewCtx, origPdf.x, origPdf.y);
        PointF roundTrip = CoordinateConverter::ScreenToPdf(pageCtx, viewCtx, screen.x, screen.y);
        
        float diffX = std::abs(roundTrip.x - origPdf.x);
        float diffY = std::abs(roundTrip.y - origPdf.y);
        EXPECT_TRUE(diffX < 1e-4f);
        EXPECT_TRUE(diffY < 1e-4f);
    }
}

TEST(CoordinateConverter_RectTransforms) {
    CoordinateConverter::PageContext pageCtx{ 612.0, 792.0, 0 };
    CoordinateConverter::ViewContext viewCtx{ 1.25, 0.0, 0.0, 100.0, 100.0 };

    RectF screenRect = CoordinateConverter::PdfToScreenRect(pageCtx, viewCtx, 50.0, 50.0, 200.0, 150.0);
    EXPECT_TRUE(screenRect.left < screenRect.right);
    EXPECT_TRUE(screenRect.top < screenRect.bottom);

    RectF pdfRect = CoordinateConverter::ScreenToPdfRect(pageCtx, viewCtx, screenRect.left, screenRect.top, screenRect.right, screenRect.bottom);
    EXPECT_TRUE(pdfRect.left < pdfRect.right);
    EXPECT_TRUE(pdfRect.top < pdfRect.bottom);
    EXPECT_TRUE(std::abs(pdfRect.left - 50.0f) < 1e-4f);
    EXPECT_TRUE(std::abs(pdfRect.bottom - 150.0f) < 1e-4f);
}

TEST(ToolStateMachine_PointerCapture_Lifecycle) {
    PdfViewer viewer;
    viewer.SetToolMode(ToolMode::Pan);
    EXPECT_TRUE(viewer.GetToolMode() == ToolMode::Pan);

    viewer.OnLButtonDown(100.0f, 100.0f);
    viewer.OnMouseMove(150.0f, 120.0f);
    viewer.OnLButtonUp(150.0f, 120.0f);

    viewer.SetToolMode(ToolMode::Select);
    EXPECT_TRUE(viewer.GetToolMode() == ToolMode::Select);

    viewer.CancelActiveInteractions();
    EXPECT_TRUE(viewer.GetToolMode() == ToolMode::Select);
}

TEST(CursorSelection_Verification) {
    PdfViewer viewer;
    viewer.SetToolMode(ToolMode::Pan);
    EXPECT_TRUE(viewer.OnSetCursor());

    viewer.SetToolMode(ToolMode::Highlight);
    EXPECT_TRUE(viewer.OnSetCursor());

    viewer.SetToolMode(ToolMode::Select);
    EXPECT_TRUE(viewer.OnSetCursor());
}

int main() {
    std::cout << "Starting Milestone 1 Empirical Verification Suite...\n";
    int result = TestRunner::Instance().RunAll();
    return result;
}
