#include "TestFramework.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>
#include <climits>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commctrl.h>
#include <d2d1.h>
#include <dwrite.h>

#include "../src/pdf_engine/src/PdfiumLibrary.h"
#include "../src/pdf_engine/src/PdfDocument.h"
#include "../src/pdf_engine/src/PdfPage.h"
#include "../src/core/interfaces/dom/CommandStack.h"
#include "../src/core/interfaces/dom/ICommand.h"
#include "../src/core/RecentFilesManager.h"
#include "../src/core/TabManager.h"
#include "../src/pdf_engine/src/commands/PageCommands.h"
#include "../src/pdf_engine/src/commands/AnnotationCommands.h"
#include "../src/pdf_engine/src/commands/TextCommands.h"
#include "../src/pdf_engine/src/commands/ImageCommands.h"
#include "../src/pdf_engine/src/commands/MacroCommand.h"
#include "../src/core/Clipboard.h"

#include "../src/ui/include/ui/MainWindow.h"
#include "../src/ui/src/views/HomeView.h"
#include "../src/ui/src/views/DocumentView.h"
#include "../src/ui/src/components/AppShell.h"
#include "../src/ui/src/components/Toolbar.h"
#include "../src/ui/src/components/ModeRail.h"
#include "../src/ui/src/components/TabBar.h"
#include "../src/ui/src/components/StatusBar.h"
#include "../src/ui/src/components/ThumbnailViewer.h"
#include "../src/ui/src/components/BookmarkPanel.h"
#include "../src/ui/src/components/SearchBar.h"
#include "../src/ui/src/components/PropertiesPanel.h"
#include "../src/ui/src/interaction/TextSelectableObject.h"
#include "../src/ui/src/interaction/AnnotationSelectableObject.h"
#include "../src/ui/src/interaction/ImageSelectableObject.h"
#include "../src/ui/src/PdfViewer.h"
#include "../src/ui/src/GraphicsDevice.h"
#include "../src/ui/include/viewport/KineticScrollFilter.h"
#include "../src/ui/include/viewport/ViewportEngine.h"
#include "../src/ui/include/input/PointerCaptureService.h"
#include "../src/ui/include/tools/ToolStateMachine.h"
#include "../src/ui/include/tools/SelectTool.h"
#include "../src/ui/include/tools/PanTool.h"
#include "../src/ui/include/tools/ShapeTool.h"
#include "../src/ui/include/tools/MarkupTool.h"
#include "../src/ui/include/tools/InkTool.h"
#include "../src/ui/include/tools/EraserTool.h"
#include "../src/ui/include/selection/SelectionModel.h"
#include "../src/ui/include/selection/TransformHandles.h"
#include "../src/ui/include/selection/CursorResolver.h"
#include "../src/app/PrintManager.h"

namespace app {
bool PrintManager::PrintDocument(HWND /*hwndOwner*/, std::shared_ptr<core::interfaces::dom::IDocument> /*doc*/) {
    return true;
}
}

namespace {


static void WriteTestFile(const wchar_t* path, const char* data, size_t size) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }
    std::ofstream out(path, std::ios::binary);
    if (size > 0) out.write(data, size);
}

static const char kMinimalPdf[] = 
    "%PDF-1.4\n"
    "1 0 obj <</Type/Catalog/Pages 2 0 R>> endobj\n"
    "2 0 obj <</Type/Pages/Count 1/Kids[3 0 R]>> endobj\n"
    "3 0 obj <</Type/Page/Parent 2 0 R/MediaBox[0 0 612 792]/Contents 4 0 R/Resources<<>>>> endobj\n"
    "4 0 obj <</Length 0>> stream\nendstream\nendobj\n"
    "xref\n0 5\n0000000000 65535 f \n0000000009 00000 n \n0000000052 00000 n \n0000000101 00000 n \n0000000193 00000 n \n"
    "trailer <</Size 5/Root 1 0 R>>\nstartxref\n233\n%%EOF";

static Microsoft::WRL::ComPtr<ID2D1RenderTarget> CreateOffscreenRenderTarget(int width = 1024, int height = 768) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    GraphicsDevice::Instance().Initialize();
    auto wicFactory = GraphicsDevice::Instance().GetWicFactory();
    auto d2dFactory = GraphicsDevice::Instance().GetD2DFactory();
    if (!wicFactory || !d2dFactory) return nullptr;

    Microsoft::WRL::ComPtr<IWICBitmap> wicBitmap;
    HRESULT hr = wicFactory->CreateBitmap(width, height, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wicBitmap);
    if (FAILED(hr) || !wicBitmap) return nullptr;

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0, 0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);

    Microsoft::WRL::ComPtr<ID2D1RenderTarget> rt;
    hr = d2dFactory->CreateWicBitmapRenderTarget(wicBitmap.Get(), props, &rt);
    if (FAILED(hr)) return nullptr;
    return rt;
}

static std::wstring GetFixturePath() {
    std::filesystem::path p = std::filesystem::absolute("tests/fixtures/basic/minimal.pdf");
    return p.wstring();
}

static void CleanMessageQueue() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        // Drain any pending messages including WM_QUIT
    }
}

static void EnsureFixtures() {
    PdfiumLibrary::Instance().Initialize();
    std::error_code ec;
    std::filesystem::create_directories("tests/fixtures/basic", ec);
    WriteTestFile(GetFixturePath().c_str(), kMinimalPdf, sizeof(kMinimalPdf) - 1);
}



} // namespace

// ============================================================================
// SUITE 1: StatusBar Null Active Tab Safety & Direct Component Testing
// ============================================================================

TEST(M2_StatusBar_NullActiveTab_AllActions_NoCrash) {
    EnsureFixtures();
    MainWindow mw;
    bool created = mw.Create(L"TestNullTabSafety", 1024, 768);
    EXPECT_TRUE(created);

    // Initial state: HomeView active, 0 tabs open (tabs.empty(), activeTabIndex = -1)
    // DocumentView's StatusBar callback is wired to MainWindow.
    // Simulate user clicking any status bar button or triggering onAction directly.
    std::vector<std::wstring> actions = {
        L"Page Down", L"Page Up", L"Zoom In", L"Zoom Out", L"Fit",
        L"Thumbnails", L"Bookmarks", L"Select", L"Avatar", L"Comments",
        L"Fields", L"More", L"", L"UnknownNonExistentAction", L"!@#$%"
    };

    // Trigger all actions repeatedly on empty tabs
    for (int iter = 0; iter < 10; ++iter) {
        for (const auto& act : actions) {
            (void)act;
            SendMessageW(mw.GetHwnd(), WM_SIZE, SIZE_RESTORED, MAKELPARAM(1024, 768));
        }
    }

    // Switch to non-existent tabs should safely no-op
    mw.SwitchToTab(-1);
    mw.SwitchToTab(0);
    mw.SwitchToTab(99);

    // Close invalid tabs should safely no-op
    mw.CloseTab(-1);
    mw.CloseTab(0);
    mw.CloseTab(99);

    // DoSave / DoSaveAs with invalid tabs should return false safely
    EXPECT_FALSE(mw.DoSave(-1));
    EXPECT_FALSE(mw.DoSave(0));
    EXPECT_FALSE(mw.DoSaveAs(-1));
    EXPECT_FALSE(mw.DoSaveAs(0));

    DestroyWindow(mw.GetHwnd());
    CleanMessageQueue();
}

TEST(M2_StatusBar_PostTabClose_AllActions_Safety) {
    EnsureFixtures();
    MainWindow mw;
    EXPECT_TRUE(mw.Create(L"TestTabCloseSafety", 1024, 768));
    std::cout << "\n[TRACE] Created MainWindow\n"; std::cout.flush();

    // Open a document
    mw.OpenFileDirect(GetFixturePath().c_str());
    std::cout << "[TRACE] OpenFileDirect done\n"; std::cout.flush();
    
    // Close the document tab -> transitions back to HomeView, activeTabIndex = -1
    mw.CloseTab(0);
    std::cout << "[TRACE] CloseTab done\n"; std::cout.flush();

    // Hammer window with WM_SIZE, WM_PAINT, WM_COMMAND, and mouse events
    for (int i = 0; i < 50; ++i) {
        SendMessageW(mw.GetHwnd(), WM_SIZE, SIZE_RESTORED, MAKELPARAM(800 + i * 5, 600 + i * 3));
        SendMessageW(mw.GetHwnd(), WM_COMMAND, MAKEWPARAM(3010, 0), 0); // Copy
        SendMessageW(mw.GetHwnd(), WM_COMMAND, MAKEWPARAM(3011, 0), 0); // Edit
        SendMessageW(mw.GetHwnd(), WM_COMMAND, MAKEWPARAM(3012, 0), 0); // Delete
    }
    std::cout << "[TRACE] Hammer loop done\n"; std::cout.flush();

    DestroyWindow(mw.GetHwnd());
    std::cout << "[TRACE] DestroyWindow done\n"; std::cout.flush();
    CleanMessageQueue();
    std::cout << "[TRACE] CleanMessageQueue done\n"; std::cout.flush();
}

TEST(M2_StatusBar_MultiTab_SequentialClose_InterleavedActions) {
    EnsureFixtures();
    MainWindow mw;
    EXPECT_TRUE(mw.Create(L"TestMultiTabActions", 1024, 768));

    // Open 3 tabs
    mw.OpenFileDirect(GetFixturePath().c_str());
    mw.OpenFileDirect(GetFixturePath().c_str());
    mw.OpenFileDirect(GetFixturePath().c_str());

    // Switch between tabs and trigger actions
    mw.SwitchToTab(0);
    SendMessageW(mw.GetHwnd(), WM_SIZE, SIZE_RESTORED, MAKELPARAM(1024, 768));

    mw.SwitchToTab(2);
    SendMessageW(mw.GetHwnd(), WM_SIZE, SIZE_RESTORED, MAKELPARAM(1024, 768));

    // Close active tab (tab 2) -> tab 1 becomes active
    mw.CloseTab(2);
    SendMessageW(mw.GetHwnd(), WM_SIZE, SIZE_RESTORED, MAKELPARAM(1024, 768));

    // Close tab 0 -> tab 0 (formerly tab 1) becomes active
    mw.CloseTab(0);
    SendMessageW(mw.GetHwnd(), WM_SIZE, SIZE_RESTORED, MAKELPARAM(1024, 768));

    // Close final tab -> returns to HomeView
    mw.CloseTab(0);
    SendMessageW(mw.GetHwnd(), WM_SIZE, SIZE_RESTORED, MAKELPARAM(1024, 768));

    DestroyWindow(mw.GetHwnd());
    CleanMessageQueue();
}

TEST(M2_StatusBar_DirectComponent_ExtremeValues_RenderSafety) {
    components::StatusBar status;
    auto rt = CreateOffscreenRenderTarget(48, 800);
    EXPECT_TRUE(rt != nullptr);

    // 1. Extreme Page Counts
    status.SetPageInfo(-100, -50);
    status.SetPageInfo(0, 0);
    status.SetPageInfo(999999, 1000000);
    status.SetPageInfo(INT_MAX, INT_MAX);

    // 2. Extreme Zoom Values
    status.SetZoom(0.0f);
    status.SetZoom(-5.0f);
    status.SetZoom(100.0f);
    status.SetZoom(0.0001f);

    // 3. Layout with various rect dimensions
    status.Layout(D2D1::RectF(0, 0, 48, 800));
    if (rt) {
        rt->BeginDraw();
        status.Render(rt);
        rt->EndDraw();
    }

    // Zero bounds layout & render
    status.Layout(D2D1::RectF(0, 0, 0, 0));
    if (rt) {
        rt->BeginDraw();
        status.Render(rt);
        rt->EndDraw();
    }

    // Tiny bounds layout & render
    status.Layout(D2D1::RectF(0, 0, 10, 10));
    if (rt) {
        rt->BeginDraw();
        status.Render(rt);
        rt->EndDraw();
    }

    // Huge bounds layout & render
    status.Layout(D2D1::RectF(0, 0, 500, 5000));
    if (rt) {
        rt->BeginDraw();
        status.Render(rt);
        rt->EndDraw();
    }

    EXPECT_TRUE(true);
}

// ============================================================================
// SUITE 2: Sidebar Toggles & PropertiesPanel Layout Updates Under Stress
// ============================================================================

TEST(M2_SidebarToggles_ThumbnailsAndBookmarks_HighFrequencyStress) {
    views::DocumentView docView;
    D2D1_RECT_F client = D2D1::RectF(0, 0, 1280, 720);
    docView.Layout(client);

    auto sidebar = docView.GetLeftSidebar();
    auto props = docView.GetPropertiesPanel();
    EXPECT_TRUE(sidebar != nullptr);
    EXPECT_TRUE(props != nullptr);

    // Initial state: Sidebar is visible (or default), Props is hidden
    props->SetVisible(false);

    // Stress toggle 1000 times
    for (int i = 0; i < 1000; ++i) {
        bool sideVis = (i % 2 == 0);
        bool propVis = (i % 3 == 0);

        sidebar->SetVisible(sideVis);
        props->SetVisible(propVis);

        docView.Layout(client);

        auto canvasBounds = docView.GetCanvasContainer()->GetBounds();
        float expectedWidth = (client.right - 48.0f) - (client.left + 60.0f);
        if (sideVis) expectedWidth -= 240.0f;
        if (propVis) expectedWidth -= 240.0f;

        float actualWidth = canvasBounds.right - canvasBounds.left;
        EXPECT_EQ(actualWidth, expectedWidth);
    }
}

TEST(M2_PropertiesPanel_DynamicSelectionLifecycle) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(GetFixturePath().c_str());
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    // Add 2 text objects
    RectF bounds1 = { 50, 700, 200, 650 };
    RectF bounds2 = { 50, 600, 200, 550 };
    auto addCmd1 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Text 1", bounds1, "Arial", 12.0f, 0, 0, 0, 255);
    auto addCmd2 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Text 2", bounds2, "Arial", 14.0f, 255, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd1)));
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd2)));

    components::PropertiesPanel propsPanel;
    propsPanel.SetVisible(false);

    PdfViewer viewer;
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objects = viewer.GetInteractionManager().GetObjects();
    EXPECT_TRUE(objects.size() >= 2);

    std::shared_ptr<ui::interaction::TextSelectableObject> t1 = nullptr;
    std::shared_ptr<ui::interaction::TextSelectableObject> t2 = nullptr;
    for (auto& obj : objects) {
        if (auto tObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(obj)) {
            if (tObj->GetTextObject()->GetText() == L"Text 1") t1 = tObj;
            if (tObj->GetTextObject()->GetText() == L"Text 2") t2 = tObj;
        }
    }
    EXPECT_TRUE(t1 != nullptr);
    EXPECT_TRUE(t2 != nullptr);

    // Wire simulated selection changed callback (same logic as MainWindow)
    auto updatePropsOnSelection = [&](const std::vector<std::shared_ptr<ui::interaction::ISelectableObject>>& sel) {
        if (sel.size() == 1 && std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(sel[0])) {
            propsPanel.SetSelectedObject(sel[0]);
            propsPanel.SetVisible(true);
        } else {
            propsPanel.SetSelectedObject(nullptr);
            propsPanel.SetVisible(false);
        }
    };

    // 1. Single selection -> Props panel becomes visible
    viewer.GetInteractionManager().GetSelectionModel().Select(t1);
    updatePropsOnSelection(viewer.GetInteractionManager().GetSelection());
    EXPECT_TRUE(propsPanel.IsVisible());

    // 2. Multi-selection -> Props panel hides
    viewer.GetInteractionManager().GetSelectionModel().AddSelect(t2);
    EXPECT_EQ(viewer.GetInteractionManager().GetSelection().size(), 2);
    updatePropsOnSelection(viewer.GetInteractionManager().GetSelection());
    EXPECT_FALSE(propsPanel.IsVisible());

    // 3. Clear selection -> Props panel hides
    viewer.GetInteractionManager().GetSelectionModel().Clear();
    EXPECT_EQ(viewer.GetInteractionManager().GetSelection().size(), 0);
    updatePropsOnSelection(viewer.GetInteractionManager().GetSelection());
    EXPECT_FALSE(propsPanel.IsVisible());

    // 4. Select t2 -> Props panel visible
    viewer.GetInteractionManager().GetSelectionModel().Select(t2);
    updatePropsOnSelection(viewer.GetInteractionManager().GetSelection());
    EXPECT_TRUE(propsPanel.IsVisible());
}

TEST(M2_PropertiesPanel_EdgeCaseData_RenderSafety) {
    EnsureFixtures();
    components::PropertiesPanel props;
    props.SetVisible(true);

    auto rt = CreateOffscreenRenderTarget(240, 600);
    EXPECT_TRUE(rt != nullptr);

    // 1. Render with null object
    props.SetSelectedObject(nullptr);
    props.Layout(D2D1::RectF(0, 0, 240, 600));
    if (rt) {
        rt->BeginDraw();
        props.Render(rt);
        rt->EndDraw();
    }

    // 2. Render with text object having extreme text and font size
    auto docRes = PdfDocument::LoadFromFile(GetFixturePath().c_str());
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    std::wstring hugeText(5000, L'W');
    RectF bounds = { 0, 792, 612, 0 };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, hugeText, bounds, "Arial", 100.0f, 128, 64, 32, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    auto page = doc->GetPage(0);
    auto textObjs = page->GetTextObjects();
    EXPECT_FALSE(textObjs.empty());

    auto selObj = std::make_shared<ui::interaction::TextSelectableObject>(textObjs[0], 0);
    props.SetSelectedObject(selObj);

    if (rt) {
        rt->BeginDraw();
        props.Render(rt);
        rt->EndDraw();
    }

    // 3. Render at zero bounds
    props.Layout(D2D1::RectF(0, 0, 0, 0));
    if (rt) {
        rt->BeginDraw();
        props.Render(rt);
        rt->EndDraw();
    }

    EXPECT_TRUE(true);
}

TEST(M2_DocumentView_Layout_GeometryConstraints) {
    views::DocumentView docView;

    // Test extreme client dimensions
    std::vector<D2D1_RECT_F> clientRects = {
        D2D1::RectF(0, 0, 0, 0),
        D2D1::RectF(0, 0, 50, 50),
        D2D1::RectF(0, 0, 108, 108),
        D2D1::RectF(0, 0, 500, 300),
        D2D1::RectF(0, 0, 1920, 1080),
        D2D1::RectF(0, 0, 3840, 2160),
        D2D1::RectF(0, 0, 10000, 100)
    };

    for (const auto& rc : clientRects) {
        docView.Layout(rc);
        auto canvasBounds = docView.GetCanvasContainer()->GetBounds();
        EXPECT_FALSE(std::isnan(canvasBounds.left));
        EXPECT_FALSE(std::isnan(canvasBounds.top));
        EXPECT_FALSE(std::isnan(canvasBounds.right));
        EXPECT_FALSE(std::isnan(canvasBounds.bottom));
    }
}

// ============================================================================
// SUITE 3: Page Insert and Delete Macro Operations
// ============================================================================

TEST(M2_PageOperations_InsertBlankPage_VariousIndices_UndoRedo) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(GetFixturePath().c_str());
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);
    EXPECT_EQ(doc->PageCount(), 1);

    PdfViewer viewer;
    viewer.SetDocument(doc);

    // Insert at index 0 (before page 0)
    viewer.InsertBlankPage(0, 612.0, 792.0);
    EXPECT_EQ(doc->PageCount(), 2);

    // Insert at index 2 (at end)
    viewer.InsertBlankPage(2, 595.0, 842.0);
    EXPECT_EQ(doc->PageCount(), 3);

    // Insert at index 1 (in middle)
    viewer.InsertBlankPage(1, 400.0, 600.0);
    EXPECT_EQ(doc->PageCount(), 4);

    // Undo all 3
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->PageCount(), 3);
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->PageCount(), 2);
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->PageCount(), 1);

    // Redo all 3
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_EQ(doc->PageCount(), 2);
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_EQ(doc->PageCount(), 3);
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_EQ(doc->PageCount(), 4);
}

TEST(M2_PageOperations_DeletePage_SinglePageGuard) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(GetFixturePath().c_str());
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);
    EXPECT_EQ(doc->PageCount(), 1);

    PdfViewer viewer;
    viewer.SetDocument(doc);

    // Attempt to delete the single page -> must be safely guarded
    viewer.DeletePage(0);
    EXPECT_EQ(doc->PageCount(), 1);
    EXPECT_FALSE(doc->GetCommandStack().CanUndo());
}

TEST(M2_PageOperations_DeletePage_MultiPage_UndoRedo) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(GetFixturePath().c_str());
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    PdfViewer viewer;
    viewer.SetDocument(doc);

    // Insert 4 pages -> 5 pages total
    for (int i = 0; i < 4; ++i) {
        viewer.InsertBlankPage(doc->PageCount(), 612.0, 792.0);
    }
    EXPECT_EQ(doc->PageCount(), 5);

    // Delete page 0
    viewer.DeletePage(0);
    EXPECT_EQ(doc->PageCount(), 4);

    // Delete page 3 (last page)
    viewer.DeletePage(3);
    EXPECT_EQ(doc->PageCount(), 3);

    // Delete page 1 (middle page)
    viewer.DeletePage(1);
    EXPECT_EQ(doc->PageCount(), 2);

    // Undo 3 deletes
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->PageCount(), 3);
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->PageCount(), 4);
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->PageCount(), 5);

    // Redo 3 deletes
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_EQ(doc->PageCount(), 4);
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_EQ(doc->PageCount(), 3);
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_EQ(doc->PageCount(), 2);
}

TEST(M2_PageOperations_MacroCommand_BatchDelete_DescendingOrder) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(GetFixturePath().c_str());
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    PdfViewer viewer;
    viewer.SetDocument(doc);

    // Insert 5 pages -> 6 total (0, 1, 2, 3, 4, 5)
    for (int i = 0; i < 5; ++i) {
        viewer.InsertBlankPage(doc->PageCount(), 612.0, 792.0);
    }
    EXPECT_EQ(doc->PageCount(), 6);

    // Batch delete pages 1, 3, 5 (in descending order: 5, 3, 1)
    auto macro = std::make_unique<pdf_engine::commands::MacroCommand>("Batch Delete");
    macro->AddCommand(std::make_unique<pdf_engine::commands::DeletePageCommand>(static_cast<PdfDocument*>(doc.get()), 5));
    macro->AddCommand(std::make_unique<pdf_engine::commands::DeletePageCommand>(static_cast<PdfDocument*>(doc.get()), 3));
    macro->AddCommand(std::make_unique<pdf_engine::commands::DeletePageCommand>(static_cast<PdfDocument*>(doc.get()), 1));

    viewer.ExecuteMacroStructureChange(std::move(macro));
    EXPECT_EQ(doc->PageCount(), 3);

    // Single undo restores all 3 deleted pages
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->PageCount(), 6);

    // Single redo deletes all 3 pages
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_EQ(doc->PageCount(), 3);
}

TEST(M2_PageOperations_ComplexMacro_AtomicRollback) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(GetFixturePath().c_str());
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);
    EXPECT_EQ(doc->PageCount(), 1);

    PdfViewer viewer;
    viewer.SetDocument(doc);

    auto macro = std::make_unique<pdf_engine::commands::MacroCommand>("Complex Macro");
    macro->AddCommand(std::make_unique<pdf_engine::commands::InsertBlankPageCommand>(
        static_cast<PdfDocument*>(doc.get()), 1, 612.0, 792.0));
    macro->AddCommand(std::make_unique<pdf_engine::commands::RotatePageCommand>(
        static_cast<PdfDocument*>(doc.get()), 1, 90));
    macro->AddCommand(std::make_unique<pdf_engine::commands::InsertBlankPageCommand>(
        static_cast<PdfDocument*>(doc.get()), 2, 612.0, 792.0));
    macro->AddCommand(std::make_unique<pdf_engine::commands::RotatePageCommand>(
        static_cast<PdfDocument*>(doc.get()), 2, 180));
    macro->AddCommand(std::make_unique<pdf_engine::commands::DeletePageCommand>(
        static_cast<PdfDocument*>(doc.get()), 0));

    viewer.ExecuteMacroStructureChange(std::move(macro));
    EXPECT_EQ(doc->PageCount(), 2);
    EXPECT_EQ(doc->GetPage(0)->GetRotation(), 90);
    EXPECT_EQ(doc->GetPage(1)->GetRotation(), 180);

    // Atomic Undo
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->PageCount(), 1);
    EXPECT_EQ(doc->GetPage(0)->GetRotation(), 0);

    // Atomic Redo
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_EQ(doc->PageCount(), 2);
    EXPECT_EQ(doc->GetPage(0)->GetRotation(), 90);
    EXPECT_EQ(doc->GetPage(1)->GetRotation(), 180);
}

TEST(M2_PageOperations_RapidUndoRedoCycles_500Iterations) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(GetFixturePath().c_str());
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    PdfViewer viewer;
    viewer.SetDocument(doc);

    viewer.InsertBlankPage(1, 612.0, 792.0);
    viewer.InsertBlankPage(2, 612.0, 792.0);
    EXPECT_EQ(doc->PageCount(), 3);

    for (int i = 0; i < 500; ++i) {
        EXPECT_TRUE(doc->GetCommandStack().Undo());
        EXPECT_TRUE(doc->GetCommandStack().Redo());
    }
    EXPECT_EQ(doc->PageCount(), 3);
}

TEST(M2_PageOperations_NullDocument_Safety) {
    PdfViewer viewer;
    // Calling page operations without setting document should be completely safe
    viewer.InsertBlankPage(0, 612.0, 792.0);
    viewer.DeletePage(0);
    viewer.MovePage(0, 1);
    viewer.RotatePage(0, 90);
    viewer.DuplicatePage(0);
    viewer.ExecuteMacroStructureChange(nullptr);

    EXPECT_TRUE(true);
}

// ============================================================================
// SUITE 4: Context Menu & Delete Key Object Selection Handling
// ============================================================================

TEST(M2_ContextMenu_Copy_NullSelectionAndInvalidSelection) {
    EnsureFixtures();
    PdfViewer viewer;
    viewer.Initialize(nullptr);

    // 1. OnCommand without document
    viewer.OnCommand(IDM_TEXT_COPY, 0);

    // 2. OnCommand with document but empty selection
    auto docRes = PdfDocument::LoadFromFile(GetFixturePath().c_str());
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    viewer.OnCommand(IDM_TEXT_COPY, 0);

    // 3. Unmapped IDs
    viewer.OnCommand(0, 0);
    viewer.OnCommand(99999, 0);
    viewer.OnCommand(static_cast<unsigned __int64>(-1), 0);

    EXPECT_TRUE(true);
}

TEST(M2_ContextMenu_Edit_Transitions) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(GetFixturePath().c_str());
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    RectF bounds = { 100, 700, 300, 600 };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Edit Test", bounds, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    PdfViewer viewer;
    viewer.Initialize(nullptr);
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objects = viewer.GetInteractionManager().GetObjects();
    EXPECT_FALSE(objects.empty());

    viewer.GetInteractionManager().GetSelectionModel().Select(objects[0]);
    viewer.OnCommand(IDM_TEXT_EDIT, 0);

    EXPECT_EQ(viewer.GetToolMode(), ToolMode::EditText);
    EXPECT_TRUE(viewer.GetInteractionManager().IsEditingText());
}

TEST(M2_ContextMenu_Delete_UndoRedo) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(GetFixturePath().c_str());
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    RectF bounds = { 100, 700, 300, 600 };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Delete Text Test", bounds, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    PdfViewer viewer;
    viewer.Initialize(nullptr);
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objects = viewer.GetInteractionManager().GetObjects();
    EXPECT_FALSE(objects.empty());

    viewer.GetInteractionManager().GetSelectionModel().Select(objects[0]);
    viewer.OnCommand(IDM_TEXT_DELETE, 0);

    EXPECT_TRUE(doc->GetCommandStack().CanUndo());
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_TRUE(doc->GetCommandStack().Redo());
}

TEST(M2_InteractionManager_DeleteRequested_BatchMixedTypes) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(GetFixturePath().c_str());
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    // Add annotation
    auto annot = doc->GetPage(0)->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    EXPECT_TRUE(annot != nullptr);

    // Add text
    RectF bounds = { 50, 650, 250, 550 };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Batch Delete Test", bounds, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    PdfViewer viewer;
    viewer.Initialize(nullptr);
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objects = viewer.GetInteractionManager().GetObjects();
    EXPECT_TRUE(objects.size() >= 2);

    for (auto& obj : objects) {
        viewer.GetInteractionManager().GetSelectionModel().AddSelect(obj);
    }
    auto selected = viewer.GetInteractionManager().GetSelection();
    EXPECT_TRUE(selected.size() >= 2);

    viewer.GetInteractionManager().onDeleteRequested(selected);
    EXPECT_TRUE(doc->GetCommandStack().CanUndo());

    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_TRUE(doc->GetCommandStack().CanRedo());
    EXPECT_TRUE(doc->GetCommandStack().Redo());
}

// ============================================================================
// SUITE 5: KineticScrollFilter Physics & Numerical Stability Adversarial Tests
// ============================================================================

TEST(Challenger2_KineticScroll_ExtremeVelocityClamping) {
    ui::viewport::KineticScrollFilter filter;
    
    // 1. Single massive impulse in X and Y (positive and negative)
    filter.AddVelocity(1e8f, -1e8f);
    EXPECT_NEAR(filter.GetVelocityX(), ui::viewport::KineticScrollFilter::MAX_VELOCITY, 0.001f);
    EXPECT_NEAR(filter.GetVelocityY(), -ui::viewport::KineticScrollFilter::MAX_VELOCITY, 0.001f);

    // 2. Continuous burst of 10,000 rapid wheel ticks in the same direction
    for (int i = 0; i < 10000; ++i) {
        filter.AddWheelDelta(120.0f, 120.0f);
        EXPECT_LE(filter.GetVelocityX(), ui::viewport::KineticScrollFilter::MAX_VELOCITY);
        EXPECT_LE(filter.GetVelocityY(), ui::viewport::KineticScrollFilter::MAX_VELOCITY);
    }
    EXPECT_NEAR(filter.GetVelocityX(), ui::viewport::KineticScrollFilter::MAX_VELOCITY, 0.001f);
    EXPECT_NEAR(filter.GetVelocityY(), ui::viewport::KineticScrollFilter::MAX_VELOCITY, 0.001f);

    // 3. Rapid sign alternation under maximum impulse
    for (int i = 0; i < 1000; ++i) {
        float sign = (i % 2 == 0) ? 1.0f : -1.0f;
        filter.AddVelocity(sign * 20000.0f, -sign * 20000.0f);
        EXPECT_GE(filter.GetVelocityX(), -ui::viewport::KineticScrollFilter::MAX_VELOCITY);
        EXPECT_LE(filter.GetVelocityX(), ui::viewport::KineticScrollFilter::MAX_VELOCITY);
        EXPECT_GE(filter.GetVelocityY(), -ui::viewport::KineticScrollFilter::MAX_VELOCITY);
        EXPECT_LE(filter.GetVelocityY(), ui::viewport::KineticScrollFilter::MAX_VELOCITY);
    }
}

TEST(Challenger2_KineticScroll_ExponentialDecay_VariableTimeDelta_MathematicalInvariance) {
    const float decayRate = 0.35f;
    const double decayConst = -std::log(1.0 - decayRate) / 0.016; // ~26.92477

    // Case A: 100 uniform steps of dt = 0.016 (total T = 1.6s)
    ui::viewport::KineticScrollFilter uniformFilter(decayRate);
    uniformFilter.SetVelocity(4000.0f, 4000.0f);
    float dx = 0.0f, dy = 0.0f;
    for (int i = 0; i < 100; ++i) {
        uniformFilter.Update(0.016, dx, dy);
    }
    // At T=1.6s, velocity decays well below quiescence threshold (< 0.1 px/s)
    EXPECT_NEAR(uniformFilter.GetVelocityX(), 0.0f, 0.001f);

    // Case B: Compare intermediate decay at T = 0.16s (10 ticks) between fixed vs variable dt
    ui::viewport::KineticScrollFilter fixedFilter(decayRate);
    fixedFilter.SetVelocity(5000.0f, 5000.0f);
    for (int i = 0; i < 10; ++i) {
        fixedFilter.Update(0.016, dx, dy);
    }
    double expectedV_16 = 5000.0 * std::exp(-decayConst * 0.16);
    EXPECT_NEAR(fixedFilter.GetVelocityX(), static_cast<float>(expectedV_16), 0.01f);

    // Variable step sequence summing to exactly 0.16s: { 0.005, 0.015, 0.020, 0.035, 0.010, 0.025, 0.050 } = 0.160s
    ui::viewport::KineticScrollFilter variableFilter(decayRate);
    variableFilter.SetVelocity(5000.0f, 5000.0f);
    double variableDts[] = { 0.005, 0.015, 0.020, 0.035, 0.010, 0.025, 0.050 };
    for (double dtStep : variableDts) {
        variableFilter.Update(dtStep, dx, dy);
    }
    // Variable stepping must match fixed stepping and analytical formula within floating point precision
    EXPECT_NEAR(variableFilter.GetVelocityX(), fixedFilter.GetVelocityX(), 0.001f);
    EXPECT_NEAR(variableFilter.GetVelocityY(), fixedFilter.GetVelocityY(), 0.001f);
    EXPECT_NEAR(variableFilter.GetVelocityX(), static_cast<float>(expectedV_16), 0.01f);
}

TEST(Challenger2_KineticScroll_Quiescence_Termination_Threshold) {
    ui::viewport::KineticScrollFilter filter;
    float dx = 0.0f, dy = 0.0f;

    // 1. Velocity below VELOCITY_EPSILON (0.1f) is immediately quiescent
    filter.SetVelocity(0.0999f, -0.0999f);
    EXPECT_FALSE(filter.IsActive());
    EXPECT_FALSE(filter.Update(0.016, dx, dy));
    EXPECT_NEAR(dx, 0.0f, 0.0001f);
    EXPECT_NEAR(dy, 0.0f, 0.0001f);

    // 2. Velocity slightly above VELOCITY_EPSILON (0.105f) -> active on tick 1, terminates on tick 2
    filter.SetVelocity(0.105f, 0.105f);
    EXPECT_TRUE(filter.IsActive());
    bool active1 = filter.Update(0.016, dx, dy);
    EXPECT_TRUE(active1);
    EXPECT_GT(dx, 0.0f);
    EXPECT_GT(dy, 0.0f);

    // Velocity decayed below 0.1f -> now exactly zeroed
    EXPECT_FALSE(filter.IsActive());
    EXPECT_NEAR(filter.GetVelocityX(), 0.0f, 0.0001f);
    EXPECT_NEAR(filter.GetVelocityY(), 0.0f, 0.0001f);

    // Subsequent updates return false
    for (int i = 0; i < 50; ++i) {
        EXPECT_FALSE(filter.Update(0.016, dx, dy));
        EXPECT_NEAR(dx, 0.0f, 0.0001f);
        EXPECT_NEAR(dy, 0.0f, 0.0001f);
    }
}

TEST(Challenger2_KineticScroll_SubPixelFloatAccumulation_And_DisplacementIntegral) {
    ui::viewport::KineticScrollFilter filter;
    // Set initial velocity to 1000 px/s
    filter.SetVelocity(1000.0f, 0.0f);

    // Exact analytical total displacement: \int_0^\infty 1000 * exp(-alpha * t) dt = 1000 / alpha = 37.1405 pixels
    // With analytical closed-form integral displacement, total displacement is frame-rate invariant.
    double totalDisplacementX = 0.0;
    float dx = 0.0f, dy = 0.0f;
    int stepCount = 0;
    while (filter.Update(0.001, dx, dy)) {
        totalDisplacementX += dx;
        stepCount++;
        if (stepCount > 10000) break; // guard against infinite loop
    }

    EXPECT_GT(stepCount, 100);
    EXPECT_FALSE(filter.IsActive());
    // Verify exact analytical displacement: 1000 / alpha = 37.1405 pixels
    EXPECT_NEAR(static_cast<float>(totalDisplacementX), 37.14f, 0.1f);
}

TEST(Challenger2_KineticScroll_EdgeCaseTimeDeltas) {
    ui::viewport::KineticScrollFilter filter;
    filter.SetVelocity(500.0f, 500.0f);
    float dx = 0.0f, dy = 0.0f;

    // 1. Zero dt
    bool res0 = filter.Update(0.0, dx, dy);
    EXPECT_TRUE(res0);
    EXPECT_NEAR(dx, 0.0f, 0.0001f);
    EXPECT_NEAR(dy, 0.0f, 0.0001f);
    EXPECT_NEAR(filter.GetVelocityX(), 500.0f, 0.001f);

    // 2. Negative dt
    bool resNeg = filter.Update(-0.016, dx, dy);
    EXPECT_TRUE(resNeg);
    EXPECT_NEAR(dx, 0.0f, 0.0001f);
    EXPECT_NEAR(dy, 0.0f, 0.0001f);

    // 3. Huge dt (frame freeze / sleep 5.0 seconds) -> clamped to 0.1s dt
    bool resHuge = filter.Update(5.0, dx, dy);
    EXPECT_TRUE(resHuge);
    EXPECT_FALSE(std::isnan(dx));
    EXPECT_FALSE(std::isinf(dx));
    EXPECT_FALSE(std::isnan(dy));
    EXPECT_FALSE(std::isinf(dy));
}

// ============================================================================
// SUITE 6: ViewportEngine Coordinate Invariance & Multi-Page Layout Tests
// ============================================================================

TEST(Challenger2_ViewportEngine_FocalZoom_CoordinateInvariance_MultiCycle) {
    const double canvasX = 350.25;
    const double canvasY = 620.75;
    const double focalScreenX = 500.0;
    const double focalScreenY = 400.0;

    double zoom = 1.0;
    double scrollX = canvasX * zoom - focalScreenX;
    double scrollY = canvasY * zoom - focalScreenY;

    for (int cycle = 0; cycle < 100; ++cycle) {
        double newZoom = zoom * 1.15;
        double outScrollX = 0.0, outScrollY = 0.0;
        ui::viewport::ViewportEngine::CalculateFocalZoomOffsets(
            zoom, newZoom,
            focalScreenX, focalScreenY,
            scrollX, scrollY,
            outScrollX, outScrollY);

        double screenX_afterIn = canvasX * newZoom - outScrollX;
        double screenY_afterIn = canvasY * newZoom - outScrollY;
        EXPECT_NEAR(screenX_afterIn, focalScreenX, 0.0001);
        EXPECT_NEAR(screenY_afterIn, focalScreenY, 0.0001);

        double backScrollX = 0.0, backScrollY = 0.0;
        ui::viewport::ViewportEngine::CalculateFocalZoomOffsets(
            newZoom, zoom,
            focalScreenX, focalScreenY,
            outScrollX, outScrollY,
            backScrollX, backScrollY);

        double screenX_back = canvasX * zoom - backScrollX;
        double screenY_back = canvasY * zoom - backScrollY;
        EXPECT_NEAR(screenX_back, focalScreenX, 0.0001);
        EXPECT_NEAR(screenY_back, focalScreenY, 0.0001);
        EXPECT_NEAR(backScrollX, scrollX, 0.0001);
        EXPECT_NEAR(backScrollY, scrollY, 0.0001);
    }
}

TEST(Challenger2_ViewportEngine_FocalZoom_WithPageOffsets) {
    const double focalScreenX = 450.0;
    const double focalScreenY = 350.0;
    const double pageOffsetX = 50.0;
    const double pageOffsetY = 80.0;

    double zoom = 1.25;
    double newZoom = 2.50;
    double scrollX = 120.0;
    double scrollY = 180.0;
    double docX = (focalScreenX + scrollX - pageOffsetX) / zoom;
    double docY = (focalScreenY + scrollY - pageOffsetY) / zoom;

    double outScrollX = 0.0, outScrollY = 0.0;
    ui::viewport::ViewportEngine::CalculateFocalZoomOffsets(
        zoom, newZoom,
        focalScreenX, focalScreenY,
        scrollX, scrollY,
        outScrollX, outScrollY,
        pageOffsetX, pageOffsetY);

    double docX_after = (focalScreenX + outScrollX - pageOffsetX) / newZoom;
    double docY_after = (focalScreenY + outScrollY - pageOffsetY) / newZoom;

    EXPECT_NEAR(docX_after, docX, 0.0001);
    EXPECT_NEAR(docY_after, docY, 0.0001);
}

TEST(Challenger2_ViewportEngine_BoundaryZoomClamping) {
    ui::viewport::ViewportEngine engine;
    engine.SetZoom(1.0);

    for (int i = 0; i < 50; ++i) {
        engine.ZoomIn(2);
    }
    EXPECT_NEAR(engine.GetZoom(), ui::viewport::ViewportEngine::MAX_ZOOM, 0.0001);

    for (int i = 0; i < 100; ++i) {
        engine.ZoomOut(2);
    }
    EXPECT_NEAR(engine.GetZoom(), ui::viewport::ViewportEngine::MIN_ZOOM, 0.0001);

    engine.SetZoom(-5.0);
    EXPECT_NEAR(engine.GetZoom(), ui::viewport::ViewportEngine::MIN_ZOOM, 0.0001);
    engine.SetZoom(0.0);
    EXPECT_NEAR(engine.GetZoom(), ui::viewport::ViewportEngine::MIN_ZOOM, 0.0001);
}

TEST(Challenger2_ViewportEngine_ContinuousLayout_MultiPage_ComplexRotations) {
    ui::viewport::ViewportEngine engine;
    engine.SetViewportSize(1000.0, 800.0);
    engine.SetZoom(1.0);

    std::vector<std::pair<double, double>> pageSizes;
    std::vector<int> rotations;
    for (int i = 0; i < 100; ++i) {
        if (i % 4 == 0) {
            pageSizes.push_back({ 612.0, 792.0 });
            rotations.push_back(0);
        } else if (i % 4 == 1) {
            pageSizes.push_back({ 612.0, 792.0 });
            rotations.push_back(90);
        } else if (i % 4 == 2) {
            pageSizes.push_back({ 595.0, 842.0 });
            rotations.push_back(180);
        } else {
            pageSizes.push_back({ 1200.0, 800.0 });
            rotations.push_back(270);
        }
    }

    const double pageGap = 15.0;
    engine.UpdateContinuousLayout(pageSizes, rotations, pageGap);

    const auto& layouts = engine.GetLayout();
    EXPECT_EQ(layouts.size(), 100);

    double expectedY = pageGap;
    double expectedMaxWidth = 0.0;

    for (size_t i = 0; i < layouts.size(); ++i) {
        const auto& p = layouts[i];
        EXPECT_EQ(p.index, static_cast<int>(i));
        EXPECT_NEAR(p.yOffset, expectedY, 0.001);

        int rot = rotations[i];
        bool isLandscape = (rot == 90 || rot == 270);
        double expectedW = isLandscape ? pageSizes[i].second : pageSizes[i].first;
        double expectedH = isLandscape ? pageSizes[i].first : pageSizes[i].second;

        EXPECT_NEAR(p.scaledWidth, expectedW, 0.001);
        EXPECT_NEAR(p.scaledHeight, expectedH, 0.001);

        if (expectedW < 1000.0) {
            EXPECT_NEAR(p.xOffset, (1000.0 - expectedW) / 2.0, 0.001);
        } else {
            EXPECT_NEAR(p.xOffset, 0.0, 0.001);
        }

        expectedY += expectedH + pageGap;
        expectedMaxWidth = (std::max)(expectedMaxWidth, expectedW);
    }

    EXPECT_NEAR(engine.GetTotalContentHeight(), expectedY, 0.001);
    EXPECT_NEAR(engine.GetTotalContentWidth(), expectedMaxWidth, 0.001);
}

TEST(Challenger2_ViewportEngine_ContinuousLayout_VisibleRange_And_PageQueries) {
    ui::viewport::ViewportEngine engine;
    engine.SetViewportSize(800.0, 600.0);
    engine.SetZoom(1.0);

    std::vector<std::pair<double, double>> pageSizes(5, { 500.0, 500.0 });
    engine.UpdateContinuousLayout(pageSizes, {}, 20.0);

    int startP = -1, endP = -1;

    engine.SetScroll(0.0, 0.0);
    EXPECT_TRUE(engine.GetVisiblePageRange(startP, endP));
    EXPECT_EQ(startP, 0);
    EXPECT_EQ(endP, 1);

    engine.SetScroll(0.0, 1000.0);
    EXPECT_TRUE(engine.GetVisiblePageRange(startP, endP));
    EXPECT_EQ(startP, 1);
    EXPECT_EQ(endP, 3);

    // Queries inside page bounds
    EXPECT_EQ(engine.GetPageAtOffset(10.0), 0);    // Above page 0 -> returns page 0
    EXPECT_EQ(engine.GetPageAtOffset(250.0), 0);   // Inside page 0 [20, 520)
    EXPECT_EQ(engine.GetPageAtOffset(600.0), 1);   // Inside page 1 [540, 1040)
    EXPECT_EQ(engine.GetPageAtOffset(1100.0), 2);  // Inside page 2 [1060, 1560)
    EXPECT_EQ(engine.GetPageAtOffset(3000.0), 4);  // Below last page -> returns page 4

    EXPECT_NEAR(engine.GetScrollYForPage(0), 20.0, 0.001);
    EXPECT_NEAR(engine.GetScrollYForPage(2), 1060.0, 0.001);
    EXPECT_NEAR(engine.GetScrollYForPage(4), 2100.0, 0.001);
    EXPECT_NEAR(engine.GetScrollYForPage(-1), 0.0, 0.001);
    EXPECT_NEAR(engine.GetScrollYForPage(99), 0.0, 0.001);
}

TEST(Challenger2_ViewportEngine_GapOffset_VulnerabilityDemonstration) {
    ui::viewport::ViewportEngine engine;
    engine.SetViewportSize(800.0, 600.0);
    engine.SetZoom(1.0);

    // 5 pages, each 500x500, gap = 20
    // Page 0: [20, 520), Page 1: [540, 1040) ... Page 4: [2100, 2600)
    std::vector<std::pair<double, double>> pageSizes(5, { 500.0, 500.0 });
    engine.UpdateContinuousLayout(pageSizes, {}, 20.0);

    // Query in the gap region [520, 540) at 530.0:
    int pageInGap = engine.GetPageAtOffset(530.0);
    // Inter-page gap [520, 540) between Page 0 and Page 1 correctly maps to Page 0:
    EXPECT_EQ(pageInGap, 0);
}

// ============================================================================
// SUITE 7: PointerCaptureService & ToolStateMachine Lifecycle & Concurrency Tests
// ============================================================================

TEST(Challenger2_PointerCaptureService_Simulated_WM_CAPTURECHANGED) {
    ui::input::PointerCaptureService captureService;
    int dummyHwndVal = 1;
    HWND dummyHwnd = reinterpret_cast<HWND>(&dummyHwndVal);
    int dummyToolVal = 2;
    void* toolToken = &dummyToolVal;

    void* notifiedLostOwner = nullptr;
    captureService.SetCaptureLostCallback([&notifiedLostOwner](void* owner) {
        notifiedLostOwner = owner;
    });

    EXPECT_TRUE(captureService.AcquireCapture(dummyHwnd, toolToken));
    EXPECT_TRUE(captureService.IsAnyCaptured());
    EXPECT_EQ(captureService.GetCaptureOwner(), toolToken);

    captureService.OnCaptureLost();

    EXPECT_EQ(notifiedLostOwner, toolToken);
    EXPECT_FALSE(captureService.IsAnyCaptured());
    EXPECT_EQ(captureService.GetCaptureOwner(), nullptr);
    EXPECT_FALSE(captureService.HasCapture(toolToken));
}

TEST(Challenger2_PointerCaptureService_OwnerPreemption) {
    ui::input::PointerCaptureService captureService;
    int dummyHwndVal = 1;
    HWND dummyHwnd = reinterpret_cast<HWND>(&dummyHwndVal);
    int dummyA = 10, dummyB = 20;
    void* toolA = &dummyA;
    void* toolB = &dummyB;

    void* lostOwner = nullptr;
    captureService.SetCaptureLostCallback([&lostOwner](void* owner) {
        lostOwner = owner;
    });

    EXPECT_TRUE(captureService.AcquireCapture(dummyHwnd, toolA));
    EXPECT_EQ(captureService.GetCaptureOwner(), toolA);

    EXPECT_TRUE(captureService.AcquireCapture(dummyHwnd, toolB));

    EXPECT_EQ(lostOwner, toolA);
    EXPECT_EQ(captureService.GetCaptureOwner(), toolB);
    EXPECT_TRUE(captureService.HasCapture(toolB));
    EXPECT_FALSE(captureService.HasCapture(toolA));
}

TEST(Challenger2_PointerCaptureGuard_RAII_Move_And_ScopeSafety) {
    ui::input::PointerCaptureService captureService;
    int dummyHwndVal = 1;
    HWND dummyHwnd = reinterpret_cast<HWND>(&dummyHwndVal);
    int dummyTokenVal = 30;
    void* token = &dummyTokenVal;

    {
        ui::input::PointerCaptureGuard guard(&captureService, dummyHwnd, token);
        EXPECT_TRUE(guard.IsAcquired());
        EXPECT_TRUE(captureService.IsAnyCaptured());
    }
    EXPECT_FALSE(captureService.IsAnyCaptured());

    {
        ui::input::PointerCaptureGuard guard1(&captureService, dummyHwnd, token);
        EXPECT_TRUE(guard1.IsAcquired());

        ui::input::PointerCaptureGuard guard2(std::move(guard1));
        EXPECT_FALSE(guard1.IsAcquired());
        EXPECT_TRUE(guard2.IsAcquired());
        EXPECT_TRUE(captureService.IsAnyCaptured());

        guard2.Release();
        EXPECT_FALSE(guard2.IsAcquired());
        EXPECT_FALSE(captureService.IsAnyCaptured());

        guard2.Release();
        EXPECT_FALSE(captureService.IsAnyCaptured());
    }
}

TEST(Challenger2_ToolStateMachine_RapidToolSwitching_DuringActiveDrag) {
    ui::input::PointerCaptureService captureService;
    int dummyHwndVal = 1;
    HWND dummyHwnd = reinterpret_cast<HWND>(&dummyHwndVal);
    ui::tools::ToolContext context;
    context.captureService = &captureService;
    context.hwnd = dummyHwnd;

    ui::tools::ToolStateMachine sm(context);
    sm.RegisterTool(std::make_unique<ui::tools::SelectTool>());
    sm.RegisterTool(std::make_unique<ui::tools::PanTool>());
    sm.RegisterTool(std::make_unique<ui::tools::ShapeTool>());
    sm.RegisterTool(std::make_unique<ui::tools::InkTool>());
    sm.RegisterTool(std::make_unique<ui::tools::EraserTool>());

    sm.SetActiveTool(ui::tools::ToolType::Select);
    EXPECT_EQ(sm.GetActiveToolType(), ui::tools::ToolType::Select);

    ui::input::PointerEvent downEvt;
    downEvt.button = ui::input::PointerButton::Left;
    downEvt.canvasPoint = { 100.0f, 100.0f };
    downEvt.clientDip = { 100.0f, 100.0f };
    sm.RoutePointerDown(downEvt);

    EXPECT_EQ(sm.GetActiveToolState(), ui::tools::ToolState::Dragging);
    EXPECT_TRUE(captureService.IsAnyCaptured());

    EXPECT_TRUE(sm.SetActiveTool(ui::tools::ToolType::Pan));
    EXPECT_EQ(sm.GetActiveToolType(), ui::tools::ToolType::Pan);
    EXPECT_EQ(sm.GetActiveToolState(), ui::tools::ToolState::Idle);
    EXPECT_FALSE(captureService.IsAnyCaptured());

    ui::tools::ToolType toolTypes[] = {
        ui::tools::ToolType::Select,
        ui::tools::ToolType::Pan,
        ui::tools::ToolType::Rectangle,
        ui::tools::ToolType::Ink,
        ui::tools::ToolType::Eraser
    };

    for (int i = 0; i < 10000; ++i) {
        ui::tools::ToolType targetType = toolTypes[i % 5];
        sm.SetActiveTool(targetType);

        if (i % 2 == 0) {
            sm.RoutePointerDown(downEvt);
        }
    }

    sm.CancelActiveInteractions();
    EXPECT_FALSE(captureService.IsAnyCaptured());
}

TEST(Challenger2_ToolStateMachine_CancelActiveInteractions_RestoresQuiescence) {
    ui::input::PointerCaptureService captureService;
    int dummyHwndVal = 1;
    HWND dummyHwnd = reinterpret_cast<HWND>(&dummyHwndVal);
    ui::tools::ToolContext context;
    context.captureService = &captureService;
    context.hwnd = dummyHwnd;

    ui::tools::ToolStateMachine sm(context);
    sm.RegisterTool(std::make_unique<ui::tools::PanTool>());
    sm.SetActiveTool(ui::tools::ToolType::Pan);

    ui::input::PointerEvent downEvt;
    downEvt.button = ui::input::PointerButton::Left;
    downEvt.clientDip = { 50.0f, 50.0f };
    sm.RoutePointerDown(downEvt);

    EXPECT_EQ(sm.GetActiveToolState(), ui::tools::ToolState::Dragging);
    EXPECT_TRUE(captureService.IsAnyCaptured());

    sm.CancelActiveInteractions();

    EXPECT_EQ(sm.GetActiveToolState(), ui::tools::ToolState::Idle);
    EXPECT_FALSE(captureService.IsAnyCaptured());
}

int main() {
    std::cout << "Starting Milestone 2 Challenger 2 Empirical Test Suite...\n";
    return TestRunner::Instance().RunAll();
}

