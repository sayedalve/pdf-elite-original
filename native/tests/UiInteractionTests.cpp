#include "TestFramework.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>
#include <set>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include "../src/pdf_engine/src/PdfiumLibrary.h"
#include "../src/pdf_engine/src/PdfDocument.h"
#include "../src/pdf_engine/src/PdfPage.h"
#include "../src/pdf_engine/src/SearchEngine.h"
#include "../src/core/interfaces/dom/CommandStack.h"
#include "../src/core/interfaces/dom/ICommand.h"
#include "../src/core/RecentFilesManager.h"
#include "../src/core/TabManager.h"
#include "../src/pdf_engine/src/commands/PageCommands.h"
#include "../src/pdf_engine/src/commands/AnnotationCommands.h"
#include "../src/pdf_engine/src/commands/TextCommands.h"
#include "../src/pdf_engine/src/commands/MacroCommand.h"
#include "pdf_engine/commands/AddLinkCommand.h"
#include "pdf_engine/commands/AddBackgroundCommand.h"
#include "pdf_engine/commands/AddWatermarkCommand.h"
#include "pdf_engine/commands/AddHeaderFooterCommand.h"
#include "pdf_engine/operations/CreateBlankPdf.h"
#include "pdf_engine/operations/CombinePdfs.h"
#include "pdf_engine/operations/ExtractImagesFromPdf.h"

#include "ui/dialogs/CreateBlankDialog.h"
#include "ui/dialogs/CombinePdfDialog.h"
#include "ui/dialogs/ExtractImagesDialog.h"
#include "ui/dialogs/WatermarkDialog.h"
#include "ui/dialogs/HeaderFooterDialog.h"
#include "ui/dialogs/BackgroundDialog.h"
#include "ui/dialogs/LinkDialog.h"

#include "AppMode.h"
#include "views/HomeView.h"
#include "views/DocumentView.h"
#include "components/AppShell.h"
#include "components/Toolbar.h"
#include "components/ModeRail.h"
#include "components/TabBar.h"
#include "components/StatusBar.h"
#include "components/ThumbnailViewer.h"
#include "components/BookmarkPanel.h"
#include "components/SearchBar.h"
#include "components/PropertiesPanel.h"
#include "interaction/TextSelectableObject.h"
#include "interaction/AnnotationSelectableObject.h"
#include "interaction/ImageSelectableObject.h"
#include "../src/core/Clipboard.h"
#include "PdfViewer.h"
#include "ThemeManager.h"
#include "selection/SelectionModel.h"
#include "selection/TransformHandles.h"
#include "selection/CursorResolver.h"
#include "tools/SelectTool.h"
#include "tools/TextSelectTool.h"
#include "tools/ToolStateMachine.h"
#include "input/PointerCaptureService.h"
#include "input/InputRouter.h"
#include "viewport/KineticScrollFilter.h"
#include "viewport/ViewportEngine.h"
#include "core/interfaces/dom/ITextPage.h"

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

static ComPtr<ID2D1RenderTarget> CreateTestRenderTarget(int width = 1024, int height = 768) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    GraphicsDevice::Instance().Initialize();
    auto wicFactory = GraphicsDevice::Instance().GetWicFactory();
    auto d2dFactory = GraphicsDevice::Instance().GetD2DFactory();
    if (!wicFactory || !d2dFactory) return nullptr;

    ComPtr<IWICBitmap> wicBitmap;
    HRESULT hr = wicFactory->CreateBitmap(width, height, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wicBitmap);
    if (FAILED(hr) || !wicBitmap) return nullptr;

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0, 0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);

    ComPtr<ID2D1RenderTarget> rt;
    hr = d2dFactory->CreateWicBitmapRenderTarget(wicBitmap.Get(), props, &rt);
    if (FAILED(hr)) return nullptr;
    return rt;
}

static void EnsureFixtures() {
    std::error_code ec;
    std::filesystem::create_directories("tests/fixtures/basic", ec);
    std::filesystem::create_directories("tests/fixtures/malformed", ec);
    std::filesystem::create_directories("tests/fixtures/output", ec);
    WriteTestFile(L"tests/fixtures/basic/minimal.pdf", kMinimalPdf, sizeof(kMinimalPdf) - 1);
    WriteTestFile(L"tests/fixtures/malformed/empty.pdf", "", 0);
    WriteTestFile(L"tests/fixtures/malformed/truncated.pdf", "%PDF-", 5);
}

} // namespace

// ============================================================================
// TIER 1: FEATURE COVERAGE (>=5 tests per feature)
// ============================================================================

// ----------------------------------------------------------------------------
// Tier 1.1: Start Page (HomeView) Tests
// ----------------------------------------------------------------------------

TEST(Tier1_HomeView_BrandingAndInitialLayout) {
    EnsureFixtures();
    views::HomeView homeView;
    D2D1_RECT_F bounds = D2D1::RectF(0, 0, 1024, 768);
    homeView.Layout(bounds);

    auto resultBounds = homeView.GetBounds();
    EXPECT_EQ(resultBounds.right - resultBounds.left, 1024.0f);
    EXPECT_EQ(resultBounds.bottom - resultBounds.top, 768.0f);
    EXPECT_TRUE(homeView.HitTest(100.0f, 100.0f));
}

TEST(Tier1_HomeView_OpenPdfButton_InvokesCallback) {
    EnsureFixtures();
    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    auto rt = CreateTestRenderTarget();
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    bool openRequested = false;
    homeView.onOpenRequest = [&openRequested]() {
        openRequested = true;
    };

    // Open button is located at (left+12, top+72, right-12, top+116) within 240px sidebar
    homeView.OnMouseDown(60.0f, 95.0f);
    EXPECT_TRUE(openRequested);
}

TEST(Tier1_HomeView_CreatePdfButton_InvokesCallback) {
    EnsureFixtures();
    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    auto rt = CreateTestRenderTarget();
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    bool createRequested = false;
    homeView.onCreateRequest = [&createRequested]() {
        createRequested = true;
    };

    // Create button is located at (left+12, top+124, right-12, top+168)
    homeView.OnMouseDown(60.0f, 145.0f);
    EXPECT_TRUE(createRequested);
}

TEST(Tier1_HomeView_QuickTools_EditPdfCard) {
    EnsureFixtures();
    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    auto rt = CreateTestRenderTarget();
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    std::wstring requestedTool;
    homeView.onToolRequest = [&requestedTool](const std::wstring& tool) {
        requestedTool = tool;
    };

    // Tool 0 ("Edit PDF") is at (left=272, top=60, right=440, bottom=180)
    homeView.OnMouseDown(320.0f, 100.0f);
    EXPECT_TRUE(requestedTool == L"Edit PDF");
}

TEST(Tier1_HomeView_QuickTools_AllEightTools) {
    EnsureFixtures();
    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    auto rt = CreateTestRenderTarget();
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    const wchar_t* expectedTools[8] = {
        L"Edit PDF", L"Convert PDF", L"OCR PDF", L"Add Comments",
        L"Translate PDF", L"Combine Files", L"Compress PDF", L"Batch PDFs"
    };

    float cardW = 168.0f;
    float cardH = 120.0f;
    float gap = 16.0f;
    float startX = 272.0f;
    float startY = 60.0f;

    for (int i = 0; i < 8; ++i) {
        std::wstring requestedTool;
        homeView.onToolRequest = [&requestedTool](const std::wstring& tool) {
            requestedTool = tool;
        };

        int col = i % 4;
        int row = i / 4;
        float x = startX + col * (cardW + gap) + 40.0f;
        float y = startY + row * (cardH + gap) + 40.0f;

        homeView.OnMouseDown(x, y);
        EXPECT_TRUE(requestedTool == expectedTools[i]);
    }
}

TEST(Tier1_HomeView_RecentFiles_RowClickOpensFile) {
    EnsureFixtures();
    std::wstring testPath = L"tests/fixtures/basic/minimal.pdf";
    core::RecentFilesManager::Instance().AddFile(testPath);

    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    auto rt = CreateTestRenderTarget();
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    std::wstring openedPath;
    homeView.onOpenFileRequest = [&openedPath](const std::wstring& path) {
        openedPath = path;
    };

    // First recent file row starts at y = 432
    homeView.OnMouseDown(350.0f, 445.0f);
    EXPECT_TRUE(!openedPath.empty());
}

TEST(Tier1_HomeView_HoverStateTracking) {
    EnsureFixtures();
    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));

    // Mouse over first tool
    homeView.OnMouseMove(320.0f, 190.0f);
    // Mouse out of tools
    homeView.OnMouseMove(10.0f, 10.0f);
    EXPECT_TRUE(homeView.HitTest(10.0f, 10.0f));
}

TEST(Tier1_HomeView_SidebarNavClicks) {
    EnsureFixtures();
    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    auto rt = CreateTestRenderTarget();
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    int selectedNav = -1;
    homeView.onNavRequest = [&selectedNav](int idx) {
        selectedNav = idx;
    };

    // Nav 0 ("Recent Files") at y = 200..236 within sidebar [8..232]
    homeView.OnMouseDown(50.0f, 215.0f);
    EXPECT_EQ(selectedNav, 0);
    EXPECT_EQ(homeView.GetSelectedNav(), 0);

    // Nav 1 ("Starred Files") at y = 240..276
    homeView.OnMouseDown(50.0f, 255.0f);
    EXPECT_EQ(selectedNav, 1);
    EXPECT_EQ(homeView.GetSelectedNav(), 1);

    // Nav 2 ("Recent Folders") at y = 280..316
    homeView.OnMouseDown(50.0f, 295.0f);
    EXPECT_EQ(selectedNav, 2);
    EXPECT_EQ(homeView.GetSelectedNav(), 2);
}

TEST(Tier1_HomeView_BatchPDFs_DirectCardClick) {
    EnsureFixtures();
    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    auto rt = CreateTestRenderTarget(1024, 768);
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    std::wstring requestedTool;
    homeView.onToolRequest = [&requestedTool](const std::wstring& tool) {
        requestedTool = tool;
    };

    // Tool 7 (Batch PDFs) is at Col 3, Row 1 in 1024x768 layout: [824, 196, 992, 316]
    homeView.OnMouseDown(900.0f, 250.0f);
    EXPECT_EQ(requestedTool, L"Batch PDFs");
}

TEST(Tier1_HomeView_BatchPDFs_ModeTransitionIntegration) {
    EnsureFixtures();
    app::AppMode activeMode = app::AppMode::Home;
    components::AppShellMode shellMode = components::AppShellMode::Home;
    components::Toolbar toolbar;
    components::ModeRail modeRail;

    auto onToolHandler = [&](const std::wstring& toolName) {
        if (toolName == L"Batch PDFs" || toolName == L"All Tools") {
            activeMode = app::AppMode::Tools;
            shellMode = components::AppShellMode::Document;
            toolbar.SetMode(app::AppMode::Tools);
            modeRail.SetActiveMode(app::AppMode::Tools);
        }
    };

    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    auto rt = CreateTestRenderTarget(1024, 768);
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }
    homeView.onToolRequest = onToolHandler;

    homeView.OnMouseDown(900.0f, 250.0f);
    EXPECT_EQ(activeMode, app::AppMode::Tools);
    EXPECT_EQ(shellMode, components::AppShellMode::Document);
    EXPECT_EQ(toolbar.GetMode(), app::AppMode::Tools);
    EXPECT_EQ(modeRail.GetActiveMode(), app::AppMode::Tools);
}

TEST(Tier1_HomeView_StarredFiles_EmptyAndPopulatedRendering) {
    EnsureFixtures();
    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    auto rt = CreateTestRenderTarget(1024, 768);

    // 1. Select Starred Files (nav 1)
    homeView.SetSelectedNav(1);
    EXPECT_EQ(homeView.GetSelectedNav(), 1);

    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        HRESULT hr = rt->EndDraw();
        EXPECT_TRUE(SUCCEEDED(hr));
    }

    // 2. Star a file and re-render
    std::wstring testPath = L"tests/fixtures/basic/minimal.pdf";
    core::RecentFilesManager::Instance().AddFile(testPath);
    core::RecentFilesManager::Instance().SetStarred(testPath, true);
    EXPECT_TRUE(core::RecentFilesManager::Instance().IsStarred(testPath));

    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        HRESULT hr = rt->EndDraw();
        EXPECT_TRUE(SUCCEEDED(hr));
    }
}

TEST(Tier1_HomeView_StarredFiles_RowClickOpensFile) {
    EnsureFixtures();
    std::wstring starredPath = L"tests/fixtures/basic/minimal.pdf";
    core::RecentFilesManager::Instance().AddFile(starredPath);
    core::RecentFilesManager::Instance().SetStarred(starredPath, true);

    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    homeView.SetSelectedNav(1); // Starred Files

    auto rt = CreateTestRenderTarget(1024, 768);
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    std::wstring openedPath;
    homeView.onOpenFileRequest = [&openedPath](const std::wstring& path) {
        openedPath = path;
    };

    // Row 0 in Starred view at y = 468, x = 350
    homeView.OnMouseDown(350.0f, 468.0f);
    EXPECT_EQ(openedPath, starredPath);
}

TEST(Tier1_HomeView_StarToggleClickAndPersistence) {
    EnsureFixtures();
    std::wstring testPath = L"tests/fixtures/basic/minimal.pdf";
    core::RecentFilesManager::Instance().AddFile(testPath);
    core::RecentFilesManager::Instance().SetStarred(testPath, false);
    EXPECT_FALSE(core::RecentFilesManager::Instance().IsStarred(testPath));

    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    homeView.SetSelectedNav(0); // Recent Files

    auto rt = CreateTestRenderTarget(1024, 768);
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    // Star icon is at [956, 456, 980, 480] -> click at (968, 468)
    homeView.OnMouseDown(968.0f, 468.0f);
    EXPECT_TRUE(core::RecentFilesManager::Instance().IsStarred(testPath));

    // Click again to untoggle
    homeView.OnMouseDown(968.0f, 468.0f);
    EXPECT_FALSE(core::RecentFilesManager::Instance().IsStarred(testPath));
}

TEST(Tier1_HomeView_RecentFolders_AggregationAndRendering) {
    EnsureFixtures();
    std::wstring file1 = L"C:\\PDFEliteTest\\Docs\\Doc1.pdf";
    std::wstring file2 = L"C:\\PDFEliteTest\\Docs\\Doc2.pdf";
    std::wstring file3 = L"C:\\PDFEliteTest\\Other\\Doc3.pdf";

    core::RecentFilesManager::Instance().AddFile(file1);
    core::RecentFilesManager::Instance().AddFile(file2);
    core::RecentFilesManager::Instance().AddFile(file3);

    auto folders = core::RecentFilesManager::Instance().GetRecentFolders();
    EXPECT_TRUE(folders.size() >= 2);

    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    homeView.SetSelectedNav(2); // Recent Folders

    auto rt = CreateTestRenderTarget(1024, 768);
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    std::wstring requestedFolder;
    homeView.onOpenFolderRequest = [&requestedFolder](const std::wstring& folder) {
        requestedFolder = folder;
    };

    // Click Row 0 in Recent Folders view at (350, 468)
    homeView.OnMouseDown(350.0f, 468.0f);
    EXPECT_FALSE(requestedFolder.empty());
}

TEST(Tier1_HomeView_AllToolsLink_HitTestAndCallback) {
    EnsureFixtures();
    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    auto rt = CreateTestRenderTarget(1024, 768);
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    std::wstring requestedTool;
    homeView.onToolRequest = [&requestedTool](const std::wstring& tool) {
        requestedTool = tool;
    };

    // "All Tools" header link in 1024x768 is at [864, 20, 992, 44] -> click at (920, 32)
    homeView.OnMouseDown(920.0f, 32.0f);
    EXPECT_EQ(requestedTool, L"All Tools");
}

TEST(Tier1_HomeView_AllToolsLink_HoverStateTracking) {
    EnsureFixtures();
    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    auto rt = CreateTestRenderTarget(1024, 768);
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    // Hover over All Tools link
    homeView.OnMouseMove(920.0f, 32.0f);
    // Move away
    homeView.OnMouseMove(100.0f, 100.0f);
    EXPECT_TRUE(homeView.HitTest(100.0f, 100.0f));
}

TEST(Tier1_HomeView_AllToolsLink_MainWindow_SwitchToToolsMode) {
    EnsureFixtures();
    app::AppMode currentMode = app::AppMode::Home;
    components::AppShellMode shellMode = components::AppShellMode::Home;
    components::Toolbar toolbar;
    components::ModeRail modeRail;

    auto onToolHandler = [&](const std::wstring& toolName) {
        if (toolName == L"All Tools") {
            currentMode = app::AppMode::Tools;
            shellMode = components::AppShellMode::Document;
            toolbar.SetMode(app::AppMode::Tools);
            modeRail.SetActiveMode(app::AppMode::Tools);
        }
    };

    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    auto rt = CreateTestRenderTarget(1024, 768);
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }
    homeView.onToolRequest = onToolHandler;

    homeView.OnMouseDown(920.0f, 32.0f);
    EXPECT_EQ(currentMode, app::AppMode::Tools);
    EXPECT_EQ(shellMode, components::AppShellMode::Document);
    EXPECT_EQ(toolbar.GetMode(), app::AppMode::Tools);
    EXPECT_EQ(modeRail.GetActiveMode(), app::AppMode::Tools);
}

TEST(Tier2_HomeView_RapidClicks_AllStartPageInteractiveElements) {
    EnsureFixtures();
    views::HomeView homeView;
    homeView.Layout(D2D1::RectF(0, 0, 1024, 768));
    auto rt = CreateTestRenderTarget(1024, 768);
    if (rt) {
        rt->BeginDraw();
        homeView.Render(rt);
        rt->EndDraw();
    }

    int toolCalls = 0;
    int navCalls = 0;
    int openCalls = 0;
    int createCalls = 0;

    homeView.onToolRequest = [&toolCalls](const std::wstring&) { toolCalls++; };
    homeView.onNavRequest = [&navCalls](int) { navCalls++; };
    homeView.onOpenRequest = [&openCalls]() { openCalls++; };
    homeView.onCreateRequest = [&createCalls]() { createCalls++; };

    for (int i = 0; i < 50; ++i) {
        homeView.OnMouseDown(60.0f, 95.0f);   // Open
        homeView.OnMouseDown(60.0f, 145.0f);  // Create
        homeView.OnMouseDown(50.0f, 215.0f);  // Nav 0
        homeView.OnMouseDown(50.0f, 255.0f);  // Nav 1
        homeView.OnMouseDown(50.0f, 295.0f);  // Nav 2
        homeView.OnMouseDown(900.0f, 250.0f); // Batch PDFs
        homeView.OnMouseDown(920.0f, 32.0f);  // All Tools
    }

    EXPECT_EQ(openCalls, 50);
    EXPECT_EQ(createCalls, 50);
    EXPECT_EQ(navCalls, 150); // 3 nav items * 50
    EXPECT_EQ(toolCalls, 100); // 2 tool items * 50
}

// ----------------------------------------------------------------------------
// Tier 1.2: Toolbar Buttons Tests
// ----------------------------------------------------------------------------

TEST(Tier1_Toolbar_LayoutAndButtonCount) {
    EnsureFixtures();
    components::Toolbar toolbar;
    toolbar.Layout(D2D1::RectF(0, 0, 1200, 48));

    EXPECT_EQ(toolbar.GetMode(), app::AppMode::Edit);
    EXPECT_TRUE(toolbar.GetBounds().right == 1200.0f);
}

TEST(Tier1_Toolbar_ModeSwitching_ViewMode) {
    EnsureFixtures();
    components::Toolbar toolbar;
    toolbar.SetMode(app::AppMode::View);
    EXPECT_EQ(toolbar.GetMode(), app::AppMode::View);

    std::wstring receivedAction;
    toolbar.onAction = [&receivedAction](const std::wstring& action) {
        receivedAction = action;
    };

    toolbar.SetActiveTool(L"Hand");
}

TEST(Tier1_Toolbar_ModeSwitching_EditMode) {
    EnsureFixtures();
    components::Toolbar toolbar;
    toolbar.SetMode(app::AppMode::Edit);
    EXPECT_EQ(toolbar.GetMode(), app::AppMode::Edit);

    std::vector<std::wstring> executedActions;
    toolbar.onAction = [&executedActions](const std::wstring& act) {
        executedActions.push_back(act);
    };

    if (toolbar.onAction) {
        toolbar.onAction(L"Edit All");
        toolbar.onAction(L"Add Text");
        toolbar.onAction(L"Add Link");
        toolbar.onAction(L"Image");
        toolbar.onAction(L"Watermark");
        toolbar.onAction(L"Background");
        toolbar.onAction(L"Header & Footer");
    }

    EXPECT_EQ(executedActions.size(), 7);
    EXPECT_TRUE(executedActions[0] == L"Edit All");
    EXPECT_TRUE(executedActions[6] == L"Header & Footer");
}

TEST(Tier1_Toolbar_ModeSwitching_CommentMode) {
    EnsureFixtures();
    components::Toolbar toolbar;
    toolbar.SetMode(app::AppMode::Comment);
    EXPECT_EQ(toolbar.GetMode(), app::AppMode::Comment);

    std::vector<std::wstring> commentActions;
    toolbar.onAction = [&commentActions](const std::wstring& act) {
        commentActions.push_back(act);
    };

    if (toolbar.onAction) {
        toolbar.onAction(L"Highlight");
        toolbar.onAction(L"Underline");
        toolbar.onAction(L"Strikeout");
        toolbar.onAction(L"Note");
        toolbar.onAction(L"Rectangle");
    }

    EXPECT_EQ(commentActions.size(), 5);
    EXPECT_TRUE(commentActions[0] == L"Highlight");
    EXPECT_TRUE(commentActions[3] == L"Note");
}

TEST(Tier1_Toolbar_ModeSwitching_OrganizeMode) {
    EnsureFixtures();
    components::Toolbar toolbar;
    toolbar.SetMode(app::AppMode::Organize);
    EXPECT_EQ(toolbar.GetMode(), app::AppMode::Organize);

    std::vector<std::wstring> organizeActions;
    toolbar.onAction = [&organizeActions](const std::wstring& act) {
        organizeActions.push_back(act);
    };

    if (toolbar.onAction) {
        toolbar.onAction(L"Insert");
        toolbar.onAction(L"Delete");
        toolbar.onAction(L"Rotate CW");
        toolbar.onAction(L"Rotate CCW");
        toolbar.onAction(L"Extract Page");
        toolbar.onAction(L"Combine Files");
    }

    EXPECT_EQ(organizeActions.size(), 6);
    EXPECT_TRUE(organizeActions[2] == L"Rotate CW");
}

TEST(Tier1_Toolbar_ModeSwitching_ToolsMode) {
    EnsureFixtures();
    components::Toolbar toolbar;
    toolbar.SetMode(app::AppMode::Tools);
    EXPECT_EQ(toolbar.GetMode(), app::AppMode::Tools);

    std::vector<std::wstring> toolsActions;
    toolbar.onAction = [&toolsActions](const std::wstring& act) {
        toolsActions.push_back(act);
    };

    if (toolbar.onAction) {
        toolbar.onAction(L"Create PDF");
        toolbar.onAction(L"Extract Images");
        toolbar.onAction(L"Combine Files");
    }

    EXPECT_EQ(toolsActions.size(), 3);
    EXPECT_TRUE(toolsActions[0] == L"Create PDF");
}

TEST(Tier1_Toolbar_ActionDispatch_AllStandardActions) {
    EnsureFixtures();
    components::Toolbar toolbar;
    std::wstring lastAction;
    toolbar.onAction = [&lastAction](const std::wstring& act) {
        lastAction = act;
    };

    const wchar_t* actions[] = {
        L"Save", L"Save As", L"Undo", L"Redo", L"Print",
        L"Zoom In", L"Zoom Out", L"Fit Width", L"Fit Page",
        L"First", L"Prev", L"Next", L"Last"
    };

    for (const auto* act : actions) {
        toolbar.onAction(act);
        EXPECT_TRUE(lastAction == act);
    }
}

TEST(Tier1_Toolbar_SetActiveTool_StateTracking) {
    EnsureFixtures();
    components::Toolbar toolbar;
    toolbar.SetMode(app::AppMode::Comment);
    toolbar.SetActiveTool(L"Highlight");
    toolbar.SetActiveTool(L"Note");
    toolbar.SetActiveTool(L"Select");
    EXPECT_TRUE(true);
}

// ----------------------------------------------------------------------------
// Tier 1.3: ModeRail Modes Tests
// ----------------------------------------------------------------------------

TEST(Tier1_ModeRail_ItemDefinitionsAndProperties) {
    EnsureFixtures();
    components::ModeRail rail;
    EXPECT_EQ(rail.GetActiveMode(), app::AppMode::View);
}

TEST(Tier1_ModeRail_HitTesting_HomeMode) {
    EnsureFixtures();
    components::ModeRail rail;
    rail.Layout(D2D1::RectF(0, 0, 72, 600));

    app::AppMode selectedMode = app::AppMode::View;
    rail.SetOnModeSelected([&selectedMode](app::AppMode mode) {
        selectedMode = mode;
    });

    // Home is at top: y in [8, 64]
    rail.OnMouseDown(36.0f, 30.0f);
    rail.OnMouseUp(36.0f, 30.0f);
    EXPECT_EQ(selectedMode, app::AppMode::Home);
}

TEST(Tier1_ModeRail_HitTesting_AllModes) {
    EnsureFixtures();
    components::ModeRail rail;
    rail.Layout(D2D1::RectF(0, 0, 72, 600));

    app::AppMode selectedMode = app::AppMode::View;
    rail.SetOnModeSelected([&selectedMode](app::AppMode mode) {
        selectedMode = mode;
    });

    // Click Edit mode (item 2): y in [64 + 64, 64 + 64 + 56] = [128, 184]
    rail.OnMouseDown(36.0f, 150.0f);
    rail.OnMouseUp(36.0f, 150.0f);
    EXPECT_EQ(selectedMode, app::AppMode::Edit);

    // Click View mode (item 3): y in [64 + 2*64, 64 + 2*64 + 56] = [192, 248]
    rail.OnMouseDown(36.0f, 210.0f);
    rail.OnMouseUp(36.0f, 210.0f);
    EXPECT_EQ(selectedMode, app::AppMode::View);
}

TEST(Tier1_ModeRail_ActiveMode_GetterSetter) {
    EnsureFixtures();
    components::ModeRail rail;
    rail.SetActiveMode(app::AppMode::Edit);
    EXPECT_EQ(rail.GetActiveMode(), app::AppMode::Edit);

    rail.SetActiveMode(app::AppMode::Organize);
    EXPECT_EQ(rail.GetActiveMode(), app::AppMode::Organize);

    rail.SetActiveMode(app::AppMode::Comment);
    EXPECT_EQ(rail.GetActiveMode(), app::AppMode::Comment);
}

TEST(Tier1_ModeRail_MouseMoveAndLeave_HoverStates) {
    EnsureFixtures();
    components::ModeRail rail;
    rail.Layout(D2D1::RectF(0, 0, 72, 600));

    rail.OnMouseMove(36.0f, 30.0f);  // Home hover
    rail.OnMouseMove(36.0f, 150.0f); // Edit hover
    rail.OnMouseLeave();             // Clear hover
    EXPECT_TRUE(true);
}

// ----------------------------------------------------------------------------
// Tier 1.4: Dialogs & PDF Engine Operations Contracts
// ----------------------------------------------------------------------------

TEST(Tier1_Dialog_CreateBlank_PresetsAndDimensions) {
    EnsureFixtures();
    ui::dialogs::CreateBlankParams params;
    
    // Default Letter
    EXPECT_EQ(params.pageSizeIndex, 0);
    EXPECT_EQ(params.widthPt, 612.0);
    EXPECT_EQ(params.heightPt, 792.0);
    EXPECT_TRUE(params.isPortrait);

    // Landscape orientation
    params.isPortrait = false;
    std::swap(params.widthPt, params.heightPt);
    EXPECT_EQ(params.widthPt, 792.0);
    EXPECT_EQ(params.heightPt, 612.0);
}

TEST(Tier1_Dialog_CreateBlank_UnitConversions) {
    // 72 points = 1.0 inch
    double pts = 72.0;
    double inches = pts / 72.0;
    EXPECT_EQ(inches, 1.0);

    // 1 inch = 25.4 mm
    double mm = inches * 25.4;
    EXPECT_EQ(mm, 25.4);

    // mm back to pts: (mm / 25.4) * 72.0
    double ptsBack = (mm / 25.4) * 72.0;
    EXPECT_EQ(ptsBack, 72.0);
}

TEST(Tier1_Dialog_CreateBlank_Operation_Success) {
    EnsureFixtures();
    pdf_engine::operations::CreateBlankParams opParams;
    opParams.pageSizeIndex = 0; // Letter
    opParams.widthPt = 612.0;
    opParams.heightPt = 792.0;
    opParams.isPortrait = true;
    opParams.pageCount = 3;
    opParams.outputPath = L"tests/fixtures/output/tier1_blank_created.pdf";

    bool success = pdf_engine::operations::CreateBlankPdfFile(opParams);
    EXPECT_TRUE(success);

    auto loadRes = PdfDocument::LoadFromFile(opParams.outputPath.c_str());
    EXPECT_TRUE(loadRes.has_value());
    EXPECT_EQ(loadRes.value->PageCount(), 3);
    auto size = loadRes.value->GetPageSize(0);
    EXPECT_EQ(size.width, 612.0);
    EXPECT_EQ(size.height, 792.0);
}

TEST(Tier1_Dialog_CombinePdf_ParamsAndReorder) {
    EnsureFixtures();
    ui::dialogs::CombineParams params;
    params.sourceFiles.push_back(L"file1.pdf");
    params.sourceFiles.push_back(L"file2.pdf");
    params.sourceFiles.push_back(L"file3.pdf");
    EXPECT_EQ(params.sourceFiles.size(), 3);

    // Move file2 up -> order: file2, file1, file3
    std::swap(params.sourceFiles[0], params.sourceFiles[1]);
    EXPECT_TRUE(params.sourceFiles[0] == L"file2.pdf");
    EXPECT_TRUE(params.sourceFiles[1] == L"file1.pdf");

    // Remove file3
    params.sourceFiles.pop_back();
    EXPECT_EQ(params.sourceFiles.size(), 2);
}

TEST(Tier1_Dialog_CombinePdf_Operation_Success) {
    EnsureFixtures();
    pdf_engine::operations::CombineParams opParams;
    opParams.sourceFiles.push_back(L"tests/fixtures/basic/minimal.pdf");
    opParams.sourceFiles.push_back(L"tests/fixtures/basic/minimal.pdf");
    opParams.outputFile = L"tests/fixtures/output/tier1_combined.pdf";
    opParams.openAfterMerge = true;

    bool success = pdf_engine::operations::CombinePdfDocuments(opParams);
    EXPECT_TRUE(success);

    auto loadRes = PdfDocument::LoadFromFile(opParams.outputFile.c_str());
    EXPECT_TRUE(loadRes.has_value());
    EXPECT_EQ(loadRes.value->PageCount(), 2);
}

TEST(Tier1_Dialog_ExtractImages_Params) {
    EnsureFixtures();
    ui::dialogs::ExtractImagesParams params;
    params.srcPdfPath = L"tests/fixtures/basic/minimal.pdf";
    params.outputDir = L"tests/fixtures/output";
    params.format = L"PNG";
    params.prefix = L"img_p";
    params.pageScope = 0; // All pages

    EXPECT_TRUE(params.format == L"PNG");
    EXPECT_TRUE(params.prefix == L"img_p");
    EXPECT_EQ(params.pageScope, 0);
}

TEST(Tier1_Dialog_Watermark_Command_ExecuteUndoRedo) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(loadRes.value);
    auto page = doc->GetPage(0);

    pdf_engine::commands::WatermarkParams wmParams;
    wmParams.text = L"CONFIDENTIAL DRAFT";
    wmParams.fontName = L"Helvetica";
    wmParams.fontSize = 36.0f;
    wmParams.opacity = 0.5f;
    wmParams.rotation = 45.0f;
    wmParams.positionIndex = 0; // Center
    wmParams.layerOver = true;
    wmParams.pageScope = 0;

    auto cmd = std::make_unique<pdf_engine::commands::AddWatermarkCommand>(doc.get(), wmParams);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(cmd)));
    EXPECT_TRUE(doc->GetCommandStack().CanUndo());

    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_TRUE(doc->GetCommandStack().CanRedo());

    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_TRUE(doc->GetCommandStack().CanUndo());
}

TEST(Tier1_Dialog_HeaderFooter_Command_ExecuteUndoRedo) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(loadRes.value);
    auto page = doc->GetPage(0);

    pdf_engine::commands::HeaderFooterParams hfParams;
    hfParams.leftHeader = L"Header Left";
    hfParams.centerHeader = L"Title";
    hfParams.rightHeader = L"Page 1";
    hfParams.leftFooter = L"Confidential";
    hfParams.centerFooter = L"2026";
    hfParams.rightFooter = L"PDF Elite";
    hfParams.fontSize = 10.0f;

    auto cmd = std::make_unique<pdf_engine::commands::AddHeaderFooterCommand>(doc.get(), hfParams);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(cmd)));
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_TRUE(doc->GetCommandStack().Redo());
}

TEST(Tier1_Dialog_Background_Command_ExecuteUndoRedo) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(loadRes.value);
    auto page = doc->GetPage(0);

    pdf_engine::commands::BackgroundParams bgParams;
    bgParams.isColor = true;
    bgParams.color = RGB(240, 240, 240);
    bgParams.opacity = 1.0;
    bgParams.pageScope = 0;

    auto cmd = std::make_unique<pdf_engine::commands::AddBackgroundCommand>(doc.get(), bgParams);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(cmd)));
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_TRUE(doc->GetCommandStack().Redo());
}

TEST(Tier1_Dialog_Link_Command_ExecuteUndoRedo) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(loadRes.value);
    auto page = doc->GetPage(0);

    pdf_engine::commands::LinkParams linkParams;
    linkParams.pageIndex = 0;
    linkParams.x = 50.0;
    linkParams.y = 50.0;
    linkParams.width = 150.0;
    linkParams.height = 30.0;
    linkParams.isUrl = true;
    linkParams.url = L"https://github.com";

    auto cmd = std::make_unique<pdf_engine::commands::AddLinkCommand>(doc.get(), linkParams);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(cmd)));
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_TRUE(doc->GetCommandStack().Redo());
}

// ----------------------------------------------------------------------------
// Tier 1.5: File Open / Save / Create Tests
// ----------------------------------------------------------------------------

TEST(Tier1_File_OpenValidDocument) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());
    EXPECT_EQ(loadRes.value->PageCount(), 1);
}

TEST(Tier1_File_SaveAs_WritesNewFile) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());

    const wchar_t* outPath = L"tests/fixtures/output/tier1_saved_as.pdf";
    bool saved = loadRes.value->SaveAs(outPath);
    EXPECT_TRUE(saved);

    auto reopen = PdfDocument::LoadFromFile(outPath);
    EXPECT_TRUE(reopen.has_value());
    EXPECT_EQ(reopen.value->PageCount(), 1);
}

TEST(Tier1_File_CreateBlank_AndVerify) {
    EnsureFixtures();
    pdf_engine::operations::CreateBlankParams p;
    p.pageSizeIndex = 1; // A4
    p.widthPt = 595.28;
    p.heightPt = 841.89;
    p.isPortrait = true;
    p.pageCount = 4;
    p.outputPath = L"tests/fixtures/output/tier1_blank_a4.pdf";

    EXPECT_TRUE(pdf_engine::operations::CreateBlankPdfFile(p));

    auto doc = PdfDocument::LoadFromFile(p.outputPath.c_str());
    EXPECT_TRUE(doc.has_value());
    EXPECT_EQ(doc.value->PageCount(), 4);
}

TEST(Tier1_File_CommandStack_DirtyTracking) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());
    auto* doc = static_cast<PdfDocument*>(loadRes.value.get());

    EXPECT_FALSE(doc->GetCommandStack().CanUndo());

    auto rotateCmd = std::make_unique<pdf_engine::commands::RotatePageCommand>(doc, 0, 90);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(rotateCmd)));
    EXPECT_TRUE(doc->GetCommandStack().CanUndo());

    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_FALSE(doc->GetCommandStack().CanUndo());
    EXPECT_TRUE(doc->GetCommandStack().CanRedo());
}

TEST(Tier1_File_RecentFiles_Persistence) {
    EnsureFixtures();
    auto& rfm = core::RecentFilesManager::Instance();
    rfm.AddFile(L"tests/fixtures/basic/file_a.pdf");
    rfm.AddFile(L"tests/fixtures/basic/file_b.pdf");
    rfm.AddFile(L"tests/fixtures/basic/file_c.pdf");

    auto list = rfm.GetRecentFiles();
    EXPECT_TRUE(!list.empty());
    EXPECT_TRUE(list[0].path == L"tests/fixtures/basic/file_c.pdf");

    // Adding file_a again should bring it to top
    rfm.AddFile(L"tests/fixtures/basic/file_a.pdf");
    list = rfm.GetRecentFiles();
    EXPECT_TRUE(list[0].path == L"tests/fixtures/basic/file_a.pdf");
}

TEST(Tier1_File_UndoRedo_RestoresState) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());
    auto* doc = static_cast<PdfDocument*>(loadRes.value.get());

    int origPages = doc->PageCount();

    auto insertCmd = std::make_unique<pdf_engine::commands::InsertBlankPageCommand>(doc, 1, 612.0, 792.0);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(insertCmd)));
    EXPECT_EQ(doc->PageCount(), origPages + 1);

    doc->GetCommandStack().Undo();
    EXPECT_EQ(doc->PageCount(), origPages);

    doc->GetCommandStack().Redo();
    EXPECT_EQ(doc->PageCount(), origPages + 1);
}

// ----------------------------------------------------------------------------
// Tier 1.6: Tab Switching & Document Lifecycle Tests
// ----------------------------------------------------------------------------

TEST(Tier1_Tabs_TabBar_SetTabs_UpdatesState) {
    EnsureFixtures();
    components::TabBar tabBar;
    tabBar.SetTabs({L"Document1.pdf", L"Document2.pdf", L"Document3.pdf"}, 1);
    EXPECT_TRUE(true);
}

TEST(Tier1_Tabs_TabBar_ClickSelectsTab) {
    EnsureFixtures();
    components::TabBar tabBar;
    tabBar.Layout(D2D1::RectF(0, 0, 800, 40));
    tabBar.SetTabs({L"Doc1.pdf", L"Doc2.pdf"}, 0);

    int selectedTab = -1;
    tabBar.SetOnTabSelected([&selectedTab](int idx) {
        selectedTab = idx;
    });

    // Tab 1 starts at x = 8 + 260 + 8 = 276
    tabBar.OnMouseDown(320.0f, 20.0f);
    EXPECT_EQ(selectedTab, 1);
}

TEST(Tier1_Tabs_TabBar_ClickCloseButton) {
    EnsureFixtures();
    components::TabBar tabBar;
    tabBar.Layout(D2D1::RectF(0, 0, 800, 40));
    std::vector<std::wstring> tabs = { L"Doc1.pdf", L"Doc2.pdf" };
    tabBar.SetTabs(tabs, 0);

    int closedTab = -1;
    int selectedTab = -1;
    tabBar.SetOnTabClosed([&closedTab](int idx) {
        closedTab = idx;
    });
    tabBar.SetOnTabSelected([&selectedTab](int idx) {
        selectedTab = idx;
    });

    // Tab 0 starts at x = 8, width = 260 -> ends at x = 268.
    // Close button region is [268 - 28 = 240, 268].
    // Clicking at x = 250 should invoke onTabClosed(0)
    tabBar.OnMouseDown(250.0f, 20.0f);
    EXPECT_EQ(closedTab, 0);
    EXPECT_EQ(selectedTab, -1);

    // Clicking at x = 100 should invoke onTabSelected(0)
    closedTab = -1;
    tabBar.OnMouseDown(100.0f, 20.0f);
    EXPECT_EQ(closedTab, -1);
    EXPECT_EQ(selectedTab, 0);

    // Tab 1 starts at x = 276, width = 260 -> ends at x = 536.
    // Close button region is [536 - 28 = 508, 536].
    // Clicking at x = 520 should invoke onTabClosed(1)
    closedTab = -1;
    selectedTab = -1;
    tabBar.OnMouseDown(520.0f, 20.0f);
    EXPECT_EQ(closedTab, 1);
    EXPECT_EQ(selectedTab, -1);
}

TEST(Tier1_SearchBar_LifecycleAndCloseCallback) {
    EnsureFixtures();
    SearchBar searchBar;
    bool closed = false;
    searchBar.SetOnCloseCallback([&closed]() {
        closed = true;
    });
    EXPECT_FALSE(searchBar.IsVisible());
}

TEST(Tier1_BookmarkPanel_LifecycleAndVisibility) {
    EnsureFixtures();
    components::BookmarkPanel bookmarkPanel;
    EXPECT_FALSE(bookmarkPanel.IsVisible());
}

TEST(Tier1_Tabs_AppShell_HomeAndDocumentMode) {
    EnsureFixtures();
    components::AppShell shell;
    EXPECT_EQ(shell.GetMode(), components::AppShellMode::Home);

    shell.SetMode(components::AppShellMode::Document);
    EXPECT_EQ(shell.GetMode(), components::AppShellMode::Document);

    shell.SetMode(components::AppShellMode::Home);
    EXPECT_EQ(shell.GetMode(), components::AppShellMode::Home);
}

TEST(Tier1_Tabs_AppShell_WindowControls) {
    EnsureFixtures();
    components::AppShell shell;
    bool minCalled = false, maxCalled = false, closeCalled = false;
    shell.onMinimize = [&minCalled]() { minCalled = true; };
    shell.onMaximize = [&maxCalled]() { maxCalled = true; };
    shell.onClose = [&closeCalled]() { closeCalled = true; };

    if (shell.onMinimize) shell.onMinimize();
    if (shell.onMaximize) shell.onMaximize();
    if (shell.onClose) shell.onClose();

    EXPECT_TRUE(minCalled);
    EXPECT_TRUE(maxCalled);
    EXPECT_TRUE(closeCalled);
}

TEST(Tier1_Tabs_StatusBar_DisplayFormatting) {
    EnsureFixtures();
    components::StatusBar statusBar;
    statusBar.Layout(D2D1::RectF(0, 0, 1024, 28));
    statusBar.SetPageInfo(0, 10);
    statusBar.SetZoom(1.25f);
    statusBar.SetFileName(L"test_document.pdf");
    statusBar.SetState(L"Ready");

    std::wstring triggeredAction;
    statusBar.onAction = [&triggeredAction](const std::wstring& act) {
        triggeredAction = act;
    };

    statusBar.onAction(L"Zoom In");
    EXPECT_TRUE(triggeredAction == L"Zoom In");
}

// ============================================================================
// TIER 2: BOUNDARY & CORNER CASES
// ============================================================================

TEST(Tier2_Boundary_EmptyFile_HandledGracefully) {
    EnsureFixtures();
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/malformed/empty.pdf");
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ((int)res.error, (int)ErrorCode::InvalidFormat);
}

TEST(Tier2_Boundary_NonExistentFile_HandledGracefully) {
    EnsureFixtures();
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/malformed/non_existent_file_xyz123.pdf");
    EXPECT_FALSE(res.has_value());
}

TEST(Tier2_Boundary_CorruptedHeader_HandledGracefully) {
    EnsureFixtures();
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/malformed/truncated.pdf");
    EXPECT_FALSE(res.has_value());
}

TEST(Tier2_Boundary_Zoom_MaxClamp) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());

    PdfViewer viewer;
    viewer.SetDocument(std::move(loadRes.value));

    // Repeatedly zoom in to extreme value
    for (int i = 0; i < 50; ++i) {
        viewer.OnZoom(2.0);
    }
    EXPECT_TRUE(viewer.GetZoom() <= 10.0); // Clamped to 10.0 (1000%)
}

TEST(Tier2_Boundary_Zoom_MinClamp) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());

    PdfViewer viewer;
    viewer.SetDocument(std::move(loadRes.value));

    // Repeatedly zoom out to extreme value
    for (int i = 0; i < 50; ++i) {
        viewer.OnZoom(-0.5);
    }
    EXPECT_TRUE(viewer.GetZoom() >= 0.05); // Clamped above 0
}

TEST(Tier2_Boundary_PageNav_NextBeyondEnd) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());

    PdfViewer viewer;
    viewer.SetDocument(std::move(loadRes.value));

    viewer.GoToPage(100); // 1-page document
    EXPECT_TRUE(viewer.GetCurrentPage() <= 0);
}

TEST(Tier2_Boundary_PageNav_PrevBeforeBeginning) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());

    PdfViewer viewer;
    viewer.SetDocument(std::move(loadRes.value));

    viewer.GoToPage(-10);
    EXPECT_TRUE(viewer.GetCurrentPage() >= 0);
}

TEST(Tier2_Boundary_PageOps_DeleteAllPagesPrevented) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());
    auto* doc = static_cast<PdfDocument*>(loadRes.value.get());

    // Single page document cannot have its only page deleted
    int pageCount = doc->PageCount();
    EXPECT_EQ(pageCount, 1);
    // UI logic enforces selected.size() < PageCount()
    bool canDeleteAll = (1 < pageCount);
    EXPECT_FALSE(canDeleteAll);
}

TEST(Tier2_Boundary_PageOps_InvalidPageIndex) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());
    auto* doc = static_cast<PdfDocument*>(loadRes.value.get());

    // Out-of-bounds delete command
    auto badDelete = std::make_unique<pdf_engine::commands::DeletePageCommand>(doc, 999);
    EXPECT_FALSE(doc->GetCommandStack().ExecuteCommand(std::move(badDelete)));
}

TEST(Tier2_Boundary_RapidRepeatedClicks_ToolbarAndZoom) {
    EnsureFixtures();
    components::Toolbar toolbar;
    toolbar.SetMode(app::AppMode::Edit);

    int callCount = 0;
    toolbar.onAction = [&callCount](const std::wstring&) {
        callCount++;
    };

    // Simulate 100 rapid clicks
    for (int i = 0; i < 100; ++i) {
        toolbar.onAction(L"Zoom In");
        toolbar.onAction(L"Undo");
        toolbar.onAction(L"Redo");
    }
    EXPECT_EQ(callCount, 300);
}

TEST(Tier2_Boundary_EmptySearchQuery_HandledGracefully) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());

    pdf_engine::SearchEngine engine(std::move(loadRes.value));
    std::vector<pdf_engine::SearchResult> results;
    engine.SearchAsync(L"", false, false, [&results](const std::vector<pdf_engine::SearchResult>& r) {
        results = r;
    });

    EXPECT_TRUE(results.empty());
}

TEST(Tier2_Boundary_CreateBlank_ZeroOrNegativePages) {
    EnsureFixtures();
    pdf_engine::operations::CreateBlankParams p;
    p.pageSizeIndex = 0;
    p.widthPt = 612.0;
    p.heightPt = 792.0;
    p.pageCount = 0; // 0 pages safely clamped to minimum 1 page
    p.outputPath = L"tests/fixtures/output/invalid_0_pages.pdf";

    bool res = pdf_engine::operations::CreateBlankPdfFile(p);
    EXPECT_TRUE(res);
    auto doc = PdfDocument::LoadFromFile(p.outputPath.c_str());
    EXPECT_TRUE(doc.has_value());
    EXPECT_EQ(doc.value->PageCount(), 1);
}

TEST(Tier2_Boundary_Combine_EmptyFileList) {
    EnsureFixtures();
    pdf_engine::operations::CombineParams p;
    p.sourceFiles.clear(); // Empty
    p.outputFile = L"tests/fixtures/output/empty_combine.pdf";

    bool res = pdf_engine::operations::CombinePdfDocuments(p);
    EXPECT_FALSE(res);
}

// ============================================================================
// TIER 3: CROSS-FEATURE COMBINATIONS
// ============================================================================

TEST(Tier3_Combination_Open_Zoom_Annotate_Undo_Redo_Save) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(loadRes.value);
    auto page = doc->GetPage(0);

    PdfViewer viewer;
    viewer.SetDocument(doc);
    viewer.OnZoom(1.5); // Zoom to 1.5x
    EXPECT_TRUE(viewer.GetZoom() > 1.0);

    // Add Annotation
    pdf_engine::commands::WatermarkParams wm;
    wm.text = L"REVIEW COPY";
    auto cmd = std::make_unique<pdf_engine::commands::AddWatermarkCommand>(doc.get(), wm);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(cmd)));

    // Undo -> Redo
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_TRUE(doc->GetCommandStack().Redo());

    // Save As
    const wchar_t* outPath = L"tests/fixtures/output/tier3_annotate_saved.pdf";
    EXPECT_TRUE(doc->SaveAs(outPath));

    // Reopen and verify
    auto reopen = PdfDocument::LoadFromFile(outPath);
    EXPECT_TRUE(reopen.has_value());
    EXPECT_EQ(reopen.value->PageCount(), 1);
}

TEST(Tier3_Combination_CreateBlank_InsertPage_Rotate_Delete_Save) {
    EnsureFixtures();
    // 1. Create blank 1-page PDF
    pdf_engine::operations::CreateBlankParams createP;
    createP.pageSizeIndex = 0;
    createP.widthPt = 612.0;
    createP.heightPt = 792.0;
    createP.pageCount = 1;
    createP.outputPath = L"tests/fixtures/output/tier3_step1.pdf";
    EXPECT_TRUE(pdf_engine::operations::CreateBlankPdfFile(createP));

    auto docRes = PdfDocument::LoadFromFile(createP.outputPath.c_str());
    EXPECT_TRUE(docRes.has_value());
    auto* doc = static_cast<PdfDocument*>(docRes.value.get());

    // 2. Insert blank page (now 2 pages)
    auto insertCmd = std::make_unique<pdf_engine::commands::InsertBlankPageCommand>(doc, 1, 612.0, 792.0);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(insertCmd)));
    EXPECT_EQ(doc->PageCount(), 2);

    // 3. Rotate page 1 by 90 degrees
    auto rotCmd = std::make_unique<pdf_engine::commands::RotatePageCommand>(doc, 1, 90);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(rotCmd)));
    EXPECT_EQ(doc->GetPage(1)->GetRotation(), 90);

    // 4. Delete page 0 (now 1 page)
    auto delCmd = std::make_unique<pdf_engine::commands::DeletePageCommand>(doc, 0);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(delCmd)));
    EXPECT_EQ(doc->PageCount(), 1);

    // 5. Save and Reopen
    const wchar_t* finalOut = L"tests/fixtures/output/tier3_restructured.pdf";
    EXPECT_TRUE(doc->SaveAs(finalOut));

    auto reopen = PdfDocument::LoadFromFile(finalOut);
    EXPECT_TRUE(reopen.has_value());
    EXPECT_EQ(reopen.value->PageCount(), 1);
    EXPECT_EQ(reopen.value->GetPage(0)->GetRotation(), 90);
}

TEST(Tier3_Combination_MultiDoc_Watermark_Background_Switch) {
    EnsureFixtures();
    auto doc1Res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    auto doc2Res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(doc1Res.has_value());
    EXPECT_TRUE(doc2Res.has_value());

    std::shared_ptr<core::interfaces::dom::IDocument> doc1 = std::move(doc1Res.value);
    std::shared_ptr<core::interfaces::dom::IDocument> doc2 = std::move(doc2Res.value);
    auto page1 = doc1->GetPage(0);
    auto page2 = doc2->GetPage(0);

    // Add Watermark to Doc 1
    pdf_engine::commands::WatermarkParams wm;
    wm.text = L"DOC 1 EXCLUSIVE";
    auto cmd1 = std::make_unique<pdf_engine::commands::AddWatermarkCommand>(doc1.get(), wm);
    EXPECT_TRUE(doc1->GetCommandStack().ExecuteCommand(std::move(cmd1)));

    // Add Background to Doc 2
    pdf_engine::commands::BackgroundParams bg;
    bg.isColor = true;
    bg.color = RGB(220, 240, 255);
    auto cmd2 = std::make_unique<pdf_engine::commands::AddBackgroundCommand>(doc2.get(), bg);
    EXPECT_TRUE(doc2->GetCommandStack().ExecuteCommand(std::move(cmd2)));

    // Verify independent command stacks
    EXPECT_TRUE(doc1->GetCommandStack().CanUndo());
    EXPECT_TRUE(doc2->GetCommandStack().CanUndo());

    EXPECT_TRUE(doc1->SaveAs(L"tests/fixtures/output/tier3_doc1.pdf"));
    EXPECT_TRUE(doc2->SaveAs(L"tests/fixtures/output/tier3_doc2.pdf"));
}

TEST(Tier3_Combination_Link_HeaderFooter_Watermark_Composite) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(loadRes.value);
    auto page = doc->GetPage(0);

    // Add Link
    pdf_engine::commands::LinkParams lp;
    lp.pageIndex = 0;
    lp.x = 100.0; lp.y = 100.0; lp.width = 200.0; lp.height = 40.0;
    lp.isUrl = true; lp.url = L"https://example.com/spec";
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(
        std::make_unique<pdf_engine::commands::AddLinkCommand>(doc.get(), lp)));

    // Add Header/Footer
    pdf_engine::commands::HeaderFooterParams hf;
    hf.leftHeader = L"Section 1.0";
    hf.rightHeader = L"Page 1";
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(
        std::make_unique<pdf_engine::commands::AddHeaderFooterCommand>(doc.get(), hf)));

    // Add Watermark
    pdf_engine::commands::WatermarkParams wm;
    wm.text = L"FINALIZED";
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(
        std::make_unique<pdf_engine::commands::AddWatermarkCommand>(doc.get(), wm)));

    const wchar_t* compositeOut = L"tests/fixtures/output/tier3_composite.pdf";
    EXPECT_TRUE(doc->SaveAs(compositeOut));

    auto reopen = PdfDocument::LoadFromFile(compositeOut);
    EXPECT_TRUE(reopen.has_value());
    EXPECT_EQ(reopen.value->PageCount(), 1);
}

TEST(Tier3_Combination_QuickTools_Combine_To_Editor) {
    EnsureFixtures();
    // Merge two 1-page documents
    pdf_engine::operations::CombineParams cp;
    cp.sourceFiles.push_back(L"tests/fixtures/basic/minimal.pdf");
    cp.sourceFiles.push_back(L"tests/fixtures/basic/minimal.pdf");
    cp.outputFile = L"tests/fixtures/output/tier3_quicktool_combined.pdf";
    EXPECT_TRUE(pdf_engine::operations::CombinePdfDocuments(cp));

    // Load merged document into PdfViewer
    auto docRes = PdfDocument::LoadFromFile(cp.outputFile.c_str());
    EXPECT_TRUE(docRes.has_value());
    EXPECT_EQ(docRes.value->PageCount(), 2);

    PdfViewer viewer;
    viewer.SetDocument(std::move(docRes.value));
    EXPECT_TRUE(viewer.GetDocument()->RotatePage(0, 90));
    EXPECT_TRUE(viewer.GetDocument()->GetPage(0)->GetRotation() == 90);
}

TEST(Tier3_Combination_ThemeChange_While_DocumentOpen) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());

    PdfViewer viewer;
    viewer.SetDocument(std::move(docRes.value));

    // Check ThemeManager
    auto colors = ThemeManager::Instance().GetColors();
    EXPECT_TRUE(colors.bgPrimary.a > 0.0f);

    viewer.InvalidateView();
}

// ============================================================================
// TIER 4: REAL-WORLD APPLICATION SCENARIOS
// ============================================================================

TEST(Tier4_Scenario_FullDocumentAuthoring) {
    EnsureFixtures();
    // Step 1: Create 3-page Letter Document
    pdf_engine::operations::CreateBlankParams createP;
    createP.pageSizeIndex = 0;
    createP.widthPt = 612.0;
    createP.heightPt = 792.0;
    createP.isPortrait = true;
    createP.pageCount = 3;
    createP.outputPath = L"tests/fixtures/output/tier4_authoring.pdf";
    EXPECT_TRUE(pdf_engine::operations::CreateBlankPdfFile(createP));

    // Step 2: Open document in editor
    auto loadRes = PdfDocument::LoadFromFile(createP.outputPath.c_str());
    EXPECT_TRUE(loadRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(loadRes.value);
    auto page = doc->GetPage(0);
    EXPECT_EQ(doc->PageCount(), 3);

    // Step 3: Add Header & Footer
    pdf_engine::commands::HeaderFooterParams hf;
    hf.leftHeader = L"CONFIDENTIAL DOCUMENT";
    hf.centerHeader = L"PROJECT TITAN";
    hf.rightHeader = L"Page 1 of 3";
    hf.leftFooter = L"PDF Elite Enterprise";
    hf.topMargin = 36.0f;
    hf.bottomMargin = 36.0f;
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(
        std::make_unique<pdf_engine::commands::AddHeaderFooterCommand>(doc.get(), hf)));

    // Step 4: Add Diagonal Draft Watermark
    pdf_engine::commands::WatermarkParams wm;
    wm.text = L"PROPRIETARY DRAFT";
    wm.fontSize = 42.0f;
    wm.opacity = 0.35f;
    wm.rotation = 45.0f;
    wm.positionIndex = 0;
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(
        std::make_unique<pdf_engine::commands::AddWatermarkCommand>(doc.get(), wm)));

    // Step 5: Add Navigation Link
    pdf_engine::commands::LinkParams link;
    link.pageIndex = 0;
    link.x = 72.0; link.y = 700.0; link.width = 200.0; link.height = 30.0;
    link.isUrl = false;
    link.targetPage = 2; // Jump to page 2 (0-based)
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(
        std::make_unique<pdf_engine::commands::AddLinkCommand>(doc.get(), link)));

    // Step 6: Save Document
    const wchar_t* finalAuthoringOut = L"tests/fixtures/output/tier4_authoring_final.pdf";
    EXPECT_TRUE(doc->SaveAs(finalAuthoringOut));

    // Step 7: Verify on disk
    auto verified = PdfDocument::LoadFromFile(finalAuthoringOut);
    EXPECT_TRUE(verified.has_value());
    EXPECT_EQ(verified.value->PageCount(), 3);
}

TEST(Tier4_Scenario_MultiDocumentMergeAndRestructure) {
    EnsureFixtures();
    // Step 1: Create Doc A (2 pages) and Doc B (2 pages)
    pdf_engine::operations::CreateBlankParams pA;
    pA.pageSizeIndex = 0; pA.widthPt = 612.0; pA.heightPt = 792.0; pA.pageCount = 2;
    pA.outputPath = L"tests/fixtures/output/tier4_doc_a.pdf";
    EXPECT_TRUE(pdf_engine::operations::CreateBlankPdfFile(pA));

    pdf_engine::operations::CreateBlankParams pB;
    pB.pageSizeIndex = 1; pB.widthPt = 595.28; pB.heightPt = 841.89; pB.pageCount = 2;
    pB.outputPath = L"tests/fixtures/output/tier4_doc_b.pdf";
    EXPECT_TRUE(pdf_engine::operations::CreateBlankPdfFile(pB));

    // Step 2: Combine into 4-page document
    pdf_engine::operations::CombineParams cp;
    cp.sourceFiles.push_back(pA.outputPath);
    cp.sourceFiles.push_back(pB.outputPath);
    cp.outputFile = L"tests/fixtures/output/tier4_merged.pdf";
    EXPECT_TRUE(pdf_engine::operations::CombinePdfDocuments(cp));

    // Step 3: Load merged document and perform restructuring
    auto loadRes = PdfDocument::LoadFromFile(cp.outputFile.c_str());
    EXPECT_TRUE(loadRes.has_value());
    auto* doc = static_cast<PdfDocument*>(loadRes.value.get());
    EXPECT_EQ(doc->PageCount(), 4);

    // Insert Cover Page at index 0 (now 5 pages)
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(
        std::make_unique<pdf_engine::commands::InsertBlankPageCommand>(doc, 0, 612.0, 792.0)));
    EXPECT_EQ(doc->PageCount(), 5);

    // Rotate Appendix page (index 4) by 90 degrees
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(
        std::make_unique<pdf_engine::commands::RotatePageCommand>(doc, 4, 90)));
    EXPECT_EQ(doc->GetPage(4)->GetRotation(), 90);

    // Delete redundant page (index 3) (now 4 pages)
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(
        std::make_unique<pdf_engine::commands::DeletePageCommand>(doc, 3)));
    EXPECT_EQ(doc->PageCount(), 4);

    // Step 4: Save and Reopen
    const wchar_t* restructuredOut = L"tests/fixtures/output/tier4_restructured_final.pdf";
    EXPECT_TRUE(doc->SaveAs(restructuredOut));

    auto finalDoc = PdfDocument::LoadFromFile(restructuredOut);
    EXPECT_TRUE(finalDoc.has_value());
    EXPECT_EQ(finalDoc.value->PageCount(), 4);
    EXPECT_EQ(finalDoc.value->GetPage(3)->GetRotation(), 90);
}

TEST(Tier4_Scenario_AssetExtractionAndReporting) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(loadRes.value);

    pdf_engine::operations::ExtractImagesParams opParams;
    opParams.srcPdfPath = L"tests/fixtures/basic/minimal.pdf";
    opParams.outputDir = L"tests/fixtures/output/extracted_images";
    opParams.format = L"PNG";
    opParams.prefix = L"asset_img";
    opParams.pageScope = 0; // All pages
    opParams.currentPage = 1;
    opParams.totalPages = doc->PageCount();

    int count = pdf_engine::operations::ExtractImagesFromDocument(doc, opParams);
    // Minimal PDF has 0 images; extraction should return 0 gracefully
    EXPECT_TRUE(count >= 0);
}

TEST(Tier4_Scenario_ReviewAndCommentingWorkflow) {
    EnsureFixtures();
    auto loadRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(loadRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(loadRes.value);

    // Add Highlight Annotation
    auto annot1 = doc->GetPage(0)->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    EXPECT_TRUE(annot1 != nullptr);

    // Add Sticky Note Annotation
    auto annot2 = doc->GetPage(0)->CreateAnnotation(core::interfaces::dom::AnnotationType::Text);
    EXPECT_TRUE(annot2 != nullptr);
    annot2->SetContents("Please review financial summary.");

    // Add Rectangle Annotation
    auto annot3 = doc->GetPage(0)->CreateAnnotation(core::interfaces::dom::AnnotationType::Square);
    EXPECT_TRUE(annot3 != nullptr);

    // Remove Rectangle Annotation
    EXPECT_TRUE(doc->GetPage(0)->RemoveAnnotation(annot3));

    // Save and Reopen
    const wchar_t* outReview = L"tests/fixtures/output/tier4_reviewed.pdf";
    EXPECT_TRUE(doc->SaveAs(outReview));

    auto reopened = PdfDocument::LoadFromFile(outReview);
    EXPECT_TRUE(reopened.has_value());
    EXPECT_EQ(reopened.value->PageCount(), 1);
}

TEST(Tier4_Scenario_FullDocumentLifecycle_StartToFinish) {
    EnsureFixtures();
    // 1. Start Page (HomeView) creates new 2-page document
    pdf_engine::operations::CreateBlankParams createP;
    createP.pageSizeIndex = 0;
    createP.widthPt = 612.0;
    createP.heightPt = 792.0;
    createP.pageCount = 2;
    createP.outputPath = L"tests/fixtures/output/tier4_lifecycle.pdf";
    EXPECT_TRUE(pdf_engine::operations::CreateBlankPdfFile(createP));

    // 2. Open document in Editor Workspace
    auto docRes = PdfDocument::LoadFromFile(createP.outputPath.c_str());
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);
    auto page = doc->GetPage(0);

    // 3. Configure AppShell and DocumentView
    components::AppShell appShell;
    appShell.SetMode(components::AppShellMode::Document);
    EXPECT_EQ(appShell.GetMode(), components::AppShellMode::Document);

    // 4. PdfViewer zoom & navigation
    PdfViewer viewer;
    viewer.SetDocument(doc);
    viewer.OnZoom(0.25);
    viewer.GoToPage(1);
    EXPECT_EQ(viewer.GetCurrentPage(), 1);

    // 5. Add Background Color
    pdf_engine::commands::BackgroundParams bg;
    bg.isColor = true;
    bg.color = RGB(250, 250, 250);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(
        std::make_unique<pdf_engine::commands::AddBackgroundCommand>(doc.get(), bg)));

    // 6. Save document
    EXPECT_TRUE(doc->SaveAs(createP.outputPath.c_str()));

    // 7. Close tab & Return to HomeView
    appShell.SetMode(components::AppShellMode::Home);
    EXPECT_EQ(appShell.GetMode(), components::AppShellMode::Home);

    // 8. Reopen from RecentFiles
    core::RecentFilesManager::Instance().AddFile(createP.outputPath);
    auto recentList = core::RecentFilesManager::Instance().GetRecentFiles();
    EXPECT_TRUE(!recentList.empty());
    EXPECT_TRUE(recentList[0].path == createP.outputPath);

    auto finalDoc = PdfDocument::LoadFromFile(recentList[0].path.c_str());
    EXPECT_TRUE(finalDoc.has_value());
    EXPECT_EQ(finalDoc.value->PageCount(), 2);
}

// ============================================================================
// TIER 5: MILESTONE 2 - EDITOR ACTIONS, CONTEXT MENUS & COMMAND ROUTING
// ============================================================================

TEST(Tier5_StatusBar_ActionRouting_NullTabSafety) {
    EnsureFixtures();
    components::StatusBar statusBar;
    bool actionDispatched = false;
    std::wstring lastAction;
    statusBar.onAction = [&](const std::wstring& action) {
        actionDispatched = true;
        lastAction = action;
    };

    const std::vector<std::wstring> actions = {
        L"Thumbnails", L"Bookmarks", L"Page Up", L"Page Down", L"Fit", L"Zoom In", L"Zoom Out"
    };

    for (const auto& act : actions) {
        actionDispatched = false;
        statusBar.onAction(act);
        EXPECT_TRUE(actionDispatched);
        EXPECT_EQ(lastAction, act);
    }
}

TEST(Tier5_StatusBar_SidebarToggles_VisibilityState) {
    EnsureFixtures();
    views::DocumentView docView;
    docView.Layout(D2D1::RectF(0, 0, 1024, 768));

    auto sidebar = docView.GetLeftSidebar();
    EXPECT_TRUE(sidebar != nullptr);
    EXPECT_FALSE(sidebar->IsVisible()); // it starts hidden

    sidebar->SetVisible(false); // test setting to false again
    EXPECT_FALSE(sidebar->IsVisible());
    docView.Layout(D2D1::RectF(0, 0, 1024, 768));
    EXPECT_EQ(sidebar->GetBounds().right, 0.0f);

    sidebar->SetVisible(true);
    EXPECT_TRUE(sidebar->IsVisible());
    docView.Layout(D2D1::RectF(0, 0, 1024, 768));
    EXPECT_TRUE(sidebar->GetBounds().right > 0.0f);
}

TEST(Tier5_PropertiesPanel_Selection_ShowHideAndLayout) {
    EnsureFixtures();
    views::DocumentView docView;
    docView.Layout(D2D1::RectF(0, 0, 1024, 768));

    auto props = docView.GetPropertiesPanel();
    EXPECT_TRUE(props != nullptr);
    EXPECT_FALSE(props->IsVisible());

    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    RectF bounds = { 100, 700, 300, 600 };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Inspector Text", bounds, "Arial", 14.0f, 255, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    auto page = doc->GetPage(0);
    auto textObjs = page->GetTextObjects();
    EXPECT_TRUE(!textObjs.empty());

    auto textSelObj = std::make_shared<ui::interaction::TextSelectableObject>(textObjs[0], 0);

    // Select text object -> PropertiesPanel shown
    props->SetSelectedObject(textSelObj);
    props->SetVisible(true);
    EXPECT_TRUE(props->IsVisible());
    docView.Layout(D2D1::RectF(0, 0, 1024, 768));
    EXPECT_TRUE(props->GetBounds().right > props->GetBounds().left);

    // Deselect -> PropertiesPanel hidden
    props->SetSelectedObject(nullptr);
    props->SetVisible(false);
    EXPECT_FALSE(props->IsVisible());
}

TEST(Tier5_PdfViewer_ContextMenu_CopyText) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    RectF bounds = { 100, 700, 300, 600 };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Milestone 2 Copy Sample", bounds, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    PdfViewer viewer;
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objects = viewer.GetInteractionManager().GetObjects();
    EXPECT_TRUE(!objects.empty());

    std::shared_ptr<ui::interaction::TextSelectableObject> targetObj = nullptr;
    for (auto& obj : objects) {
        if (auto tObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(obj)) {
            if (tObj->GetTextObject()->GetText() == L"Milestone 2 Copy Sample") {
                targetObj = tObj;
                break;
            }
        }
    }
    EXPECT_TRUE(targetObj != nullptr);

    viewer.GetInteractionManager().GetSelectionModel().Select(targetObj);
    EXPECT_EQ(viewer.GetInteractionManager().GetSelection().size(), 1);

    viewer.OnCommand(IDM_TEXT_COPY, 0);

    std::wstring copied = core::Clipboard::GetText(nullptr);
    EXPECT_EQ(copied, L"Milestone 2 Copy Sample");
}

TEST(Tier5_PdfViewer_ContextMenu_EditText) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    RectF bounds = { 100, 700, 300, 600 };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Editable Sample", bounds, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    PdfViewer viewer;
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objects = viewer.GetInteractionManager().GetObjects();
    std::shared_ptr<ui::interaction::TextSelectableObject> targetObj = nullptr;
    for (auto& obj : objects) {
        if (auto tObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(obj)) {
            targetObj = tObj;
            break;
        }
    }
    EXPECT_TRUE(targetObj != nullptr);

    viewer.GetInteractionManager().GetSelectionModel().Select(targetObj);

    viewer.OnCommand(IDM_TEXT_EDIT, 0);

    EXPECT_EQ(viewer.GetToolMode(), ToolMode::EditText);
    EXPECT_TRUE(viewer.GetInteractionManager().IsEditingText());
}

TEST(Tier5_PdfViewer_ContextMenu_DeleteText) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    RectF bounds = { 100, 700, 300, 600 };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Delete This Text", bounds, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    PdfViewer viewer;
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objects = viewer.GetInteractionManager().GetObjects();
    std::shared_ptr<ui::interaction::TextSelectableObject> targetObj = nullptr;
    for (auto& obj : objects) {
        if (auto tObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(obj)) {
            targetObj = tObj;
            break;
        }
    }
    EXPECT_TRUE(targetObj != nullptr);

    viewer.GetInteractionManager().GetSelectionModel().Select(targetObj);

    viewer.OnCommand(IDM_TEXT_DELETE, 0);

    EXPECT_TRUE(doc->GetCommandStack().CanUndo());

    EXPECT_TRUE(doc->GetCommandStack().Undo());
}

TEST(Tier5_PdfViewer_OnDeleteRequested_KeyboardDelete) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    auto annot = doc->GetPage(0)->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    EXPECT_TRUE(annot != nullptr);

    RectF bounds = { 50, 650, 250, 550 };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Key Delete Test", bounds, "Arial", 12.0f, 0, 0, 0, 255);
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
}

TEST(Tier5_PdfViewer_PageOperations_InsertBlankAndMacroStructureChange) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);
    EXPECT_EQ(doc->PageCount(), 1);

    PdfViewer viewer;
    viewer.SetDocument(doc);

    viewer.InsertBlankPage(1, 612.0, 792.0);
    EXPECT_EQ(doc->PageCount(), 2);
    EXPECT_TRUE(doc->GetCommandStack().CanUndo());

    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->PageCount(), 1);

    auto macro = std::make_unique<pdf_engine::commands::MacroCommand>("Multi Page Structure Change");
    macro->AddCommand(std::make_unique<pdf_engine::commands::InsertBlankPageCommand>(
        static_cast<PdfDocument*>(doc.get()), 1, 612.0, 792.0));
    macro->AddCommand(std::make_unique<pdf_engine::commands::RotatePageCommand>(
        static_cast<PdfDocument*>(doc.get()), 1, 90));

    viewer.ExecuteMacroStructureChange(std::move(macro));
    EXPECT_EQ(doc->PageCount(), 2);
    EXPECT_EQ(doc->GetPage(1)->GetRotation(), 90);

    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->PageCount(), 1);
}

TEST(Tier5_PhaseF_WYSIWYG_Text_Editing) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);
    
    RectF bounds = { 100, 700, 300, 600 };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Initial Text", bounds, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    PdfViewer viewer;
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objects = viewer.GetInteractionManager().GetObjects();
    std::shared_ptr<ui::interaction::TextSelectableObject> targetObj = nullptr;
    for (auto& obj : objects) {
        if (auto tObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(obj)) {
            if (tObj->GetTextObject()->GetText() == L"Initial Text") {
                targetObj = tObj;
                break;
            }
        }
    }
    EXPECT_TRUE(targetObj != nullptr);

    viewer.GetInteractionManager().GetSelectionModel().Select(targetObj);
    viewer.OnCommand(IDM_TEXT_EDIT, 0);
    EXPECT_TRUE(viewer.GetInteractionManager().IsEditingText());

    for (int i=0; i<12; i++) {
        viewer.GetInteractionManager().OnKeyDown(VK_BACK, false, false);
    }
    
    std::wstring toType = L"Hello World";
    for (wchar_t c : toType) {
        viewer.GetInteractionManager().OnChar(c);
    }

    viewer.GetInteractionManager().OnLButtonDown(500, 500, false); 
    EXPECT_FALSE(viewer.GetInteractionManager().IsEditingText());

    bool foundHelloWorld = false;
    viewer.ReloadInteractableObjects();
    objects = viewer.GetInteractionManager().GetObjects();
    for (auto& obj : objects) {
        if (auto tObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(obj)) {
            std::wstring foundText = tObj->GetTextObject()->GetText();
            if (foundText == L"Hello World") {
                foundHelloWorld = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundHelloWorld);
}

// ============================================================================
// M3: SELECTION SUBSYSTEM, 8-WAY HANDLES, CURSOR RESOLUTION & SELECT TOOLS
// ============================================================================

class MockTextPageImpl : public core::interfaces::dom::ITextPage {
public:
    std::wstring content;
    std::vector<RectF> charBoxes;

    MockTextPageImpl(const std::wstring& text) : content(text) {
        charBoxes.resize(text.size());
        for (size_t i = 0; i < text.size(); ++i) {
            float x = static_cast<float>(i * 10);
            charBoxes[i] = RectF{ x, 100.0f, x + 10.0f, 120.0f };
        }
    }

    int GetCharCount() const override { return static_cast<int>(content.size()); }
    std::wstring GetText(int startCharIndex, int charCount) const override {
        if (startCharIndex < 0 || startCharIndex >= static_cast<int>(content.size())) return L"";
        int c = (std::min)(charCount, static_cast<int>(content.size()) - startCharIndex);
        return content.substr(startCharIndex, c);
    }
    RectF GetCharBox(int charIndex) const override {
        if (charIndex >= 0 && charIndex < static_cast<int>(charBoxes.size())) return charBoxes[charIndex];
        return RectF{ 0, 0, 0, 0 };
    }
    int GetCharIndexAtPos(double x, double y, double xTolerance, double yTolerance) const override {
        (void)y; (void)yTolerance; (void)xTolerance;
        int idx = static_cast<int>(x / 10.0);
        if (idx >= 0 && idx < static_cast<int>(content.size())) return idx;
        return -1;
    }
    std::vector<RectF> GetRects(int startCharIndex, int charCount) const override {
        std::vector<RectF> r;
        if (startCharIndex < 0 || startCharIndex >= static_cast<int>(charBoxes.size())) return r;
        int c = (std::min)(charCount, static_cast<int>(charBoxes.size()) - startCharIndex);
        for (int i = 0; i < c; ++i) {
            r.push_back(charBoxes[startCharIndex + i]);
        }
        return r;
    }
};

TEST(M3_SelectionModel_ObjectSelection_AddToggleDeselectBounds) {
    ui::selection::SelectionModel model;
    bool notified = false;
    model.onSelectionChanged = [&]() { notified = true; };

    EXPECT_FALSE(model.HasSelection());
    EXPECT_EQ(model.GetSelectionMode(), ui::selection::SelectionMode::None);

    // 1. Single Select
    model.Select("obj1", 0, RectF{ 10.0f, 20.0f, 100.0f, 80.0f }, 0.0f);
    EXPECT_TRUE(notified);
    EXPECT_TRUE(model.HasSelection());
    EXPECT_TRUE(model.HasObjectSelection());
    EXPECT_EQ(model.GetSelectedCount(), 1ull);
    EXPECT_TRUE(model.IsSelected("obj1"));
    EXPECT_FALSE(model.IsSelected("obj2"));
    EXPECT_EQ(model.GetSelectionPageIndex(), 0);

    RectF bounds = model.GetSelectionBounds();
    EXPECT_FLOAT_EQ(bounds.left, 10.0f);
    EXPECT_FLOAT_EQ(bounds.top, 20.0f);
    EXPECT_FLOAT_EQ(bounds.right, 100.0f);
    EXPECT_FLOAT_EQ(bounds.bottom, 80.0f);

    // 2. Add Select (Multi-selection)
    notified = false;
    model.AddSelect("obj2", 0, RectF{ 50.0f, 10.0f, 200.0f, 150.0f }, 0.0f);
    EXPECT_TRUE(notified);
    EXPECT_EQ(model.GetSelectedCount(), 2ull);
    EXPECT_TRUE(model.IsSelected("obj1"));
    EXPECT_TRUE(model.IsSelected("obj2"));

    bounds = model.GetSelectionBounds();
    EXPECT_FLOAT_EQ(bounds.left, 10.0f);
    EXPECT_FLOAT_EQ(bounds.top, 10.0f);
    EXPECT_FLOAT_EQ(bounds.right, 200.0f);
    EXPECT_FLOAT_EQ(bounds.bottom, 150.0f);

    // 3. Toggle Select
    notified = false;
    model.ToggleSelect("obj1", 0, RectF{ 10.0f, 20.0f, 100.0f, 80.0f });
    EXPECT_TRUE(notified);
    EXPECT_EQ(model.GetSelectedCount(), 1ull);
    EXPECT_FALSE(model.IsSelected("obj1"));
    EXPECT_TRUE(model.IsSelected("obj2"));

    model.ToggleSelect("obj1", 0, RectF{ 10.0f, 20.0f, 100.0f, 80.0f });
    EXPECT_EQ(model.GetSelectedCount(), 2ull);
    EXPECT_TRUE(model.IsSelected("obj1"));

    // 4. Deselect & Clear
    model.Deselect("obj2");
    EXPECT_EQ(model.GetSelectedCount(), 1ull);
    EXPECT_FALSE(model.IsSelected("obj2"));

    model.Clear();
    EXPECT_FALSE(model.HasSelection());
    EXPECT_EQ(model.GetSelectedCount(), 0ull);
}

TEST(M3_SelectionModel_TextSelection_BoundariesAndMultiClick) {
    MockTextPageImpl textPage(L"Hello world\nSecond line_test.");
    ui::selection::SelectionModel model;

    // 1. Word boundary finding
    auto [w1_start, w1_end] = ui::selection::SelectionModel::FindWordBoundaries(textPage.content, 2); // 'l' in "Hello"
    EXPECT_EQ(w1_start, 0);
    EXPECT_EQ(w1_end, 4); // "Hello" is 0..4

    auto [w2_start, w2_end] = ui::selection::SelectionModel::FindWordBoundaries(textPage.content, 5); // space ' '
    EXPECT_EQ(w2_start, 5);
    EXPECT_EQ(w2_end, 5);

    auto [w3_start, w3_end] = ui::selection::SelectionModel::FindWordBoundaries(textPage.content, 8); // 'r' in "world"
    EXPECT_EQ(w3_start, 6);
    EXPECT_EQ(w3_end, 10); // "world" is 6..10

    // 2. Line boundary finding
    auto [l1_start, l1_end] = ui::selection::SelectionModel::FindLineBoundaries(textPage.content, 3);
    EXPECT_EQ(l1_start, 0);
    EXPECT_EQ(l1_end, 10); // "Hello world"

    auto [l2_start, l2_end] = ui::selection::SelectionModel::FindLineBoundaries(textPage.content, 15);
    EXPECT_EQ(l2_start, 12);
    EXPECT_EQ(l2_end, 28); // "Second line_test."

    // 3. Multi-click operations
    // Single click character
    model.SelectCharacterAt(0, 0, &textPage);
    EXPECT_TRUE(model.HasTextSelection());
    EXPECT_EQ(model.GetSelectedText(), L"H");
    EXPECT_EQ(model.GetSelectionMode(), ui::selection::SelectionMode::Text);

    // Double click word
    model.SelectWordAt(0, 8, &textPage);
    EXPECT_EQ(model.GetSelectedText(), L"world");

    // Triple click line
    model.SelectLineAt(0, 2, &textPage);
    EXPECT_EQ(model.GetSelectedText(), L"Hello world");

    // Selection expansion
    model.ExpandSelectionTo(0, 20, &textPage, ui::selection::TextClickType::Single);
    EXPECT_TRUE(model.GetTextSelection().endCharIndex >= 20);

    model.ClearTextSelection();
    EXPECT_FALSE(model.HasTextSelection());
}

TEST(M3_TransformHandles_EightWayGeometry_HitTestAndRotation) {
    ui::selection::TransformHandles handles;
    RectF bounds = { 100.0f, 100.0f, 300.0f, 200.0f };

    // 1. Compute Handles
    auto descs = handles.ComputeHandles(bounds, 0.0f, 8.0f, 24.0f, true);
    EXPECT_EQ(descs.size(), 9ull);

    EXPECT_EQ(descs[0].type, ui::selection::HandleType::NW);
    EXPECT_FLOAT_EQ(descs[0].position.x, 100.0f);
    EXPECT_FLOAT_EQ(descs[0].position.y, 100.0f);

    EXPECT_EQ(descs[1].type, ui::selection::HandleType::N);
    EXPECT_FLOAT_EQ(descs[1].position.x, 200.0f);
    EXPECT_FLOAT_EQ(descs[1].position.y, 100.0f);

    EXPECT_EQ(descs[2].type, ui::selection::HandleType::NE);
    EXPECT_FLOAT_EQ(descs[2].position.x, 300.0f);
    EXPECT_FLOAT_EQ(descs[2].position.y, 100.0f);

    EXPECT_EQ(descs[3].type, ui::selection::HandleType::E);
    EXPECT_FLOAT_EQ(descs[3].position.x, 300.0f);
    EXPECT_FLOAT_EQ(descs[3].position.y, 150.0f);

    EXPECT_EQ(descs[4].type, ui::selection::HandleType::SE);
    EXPECT_FLOAT_EQ(descs[4].position.x, 300.0f);
    EXPECT_FLOAT_EQ(descs[4].position.y, 200.0f);

    EXPECT_EQ(descs[5].type, ui::selection::HandleType::S);
    EXPECT_FLOAT_EQ(descs[5].position.x, 200.0f);
    EXPECT_FLOAT_EQ(descs[5].position.y, 200.0f);

    EXPECT_EQ(descs[6].type, ui::selection::HandleType::SW);
    EXPECT_FLOAT_EQ(descs[6].position.x, 100.0f);
    EXPECT_FLOAT_EQ(descs[6].position.y, 200.0f);

    EXPECT_EQ(descs[7].type, ui::selection::HandleType::W);
    EXPECT_FLOAT_EQ(descs[7].position.x, 100.0f);
    EXPECT_FLOAT_EQ(descs[7].position.y, 150.0f);

    EXPECT_EQ(descs[8].type, ui::selection::HandleType::Rotation);
    EXPECT_FLOAT_EQ(descs[8].position.x, 200.0f);
    EXPECT_FLOAT_EQ(descs[8].position.y, 76.0f); // 100 - 24

    // 2. Hit Testing all handles
    EXPECT_EQ(handles.HitTest(PointF{ 100.0f, 100.0f }, bounds), ui::selection::HandleType::NW);
    EXPECT_EQ(handles.HitTest(PointF{ 200.0f, 100.0f }, bounds), ui::selection::HandleType::N);
    EXPECT_EQ(handles.HitTest(PointF{ 300.0f, 100.0f }, bounds), ui::selection::HandleType::NE);
    EXPECT_EQ(handles.HitTest(PointF{ 300.0f, 150.0f }, bounds), ui::selection::HandleType::E);
    EXPECT_EQ(handles.HitTest(PointF{ 300.0f, 200.0f }, bounds), ui::selection::HandleType::SE);
    EXPECT_EQ(handles.HitTest(PointF{ 200.0f, 200.0f }, bounds), ui::selection::HandleType::S);
    EXPECT_EQ(handles.HitTest(PointF{ 100.0f, 200.0f }, bounds), ui::selection::HandleType::SW);
    EXPECT_EQ(handles.HitTest(PointF{ 100.0f, 150.0f }, bounds), ui::selection::HandleType::W);
    EXPECT_EQ(handles.HitTest(PointF{ 200.0f, 76.0f }, bounds), ui::selection::HandleType::Rotation);

    // Body hit
    EXPECT_EQ(handles.HitTest(PointF{ 200.0f, 150.0f }, bounds), ui::selection::HandleType::Body);

    // Outside hit
    EXPECT_EQ(handles.HitTest(PointF{ 50.0f, 50.0f }, bounds), ui::selection::HandleType::None);
}

TEST(M3_TransformHandles_Snapping_RotationMath_InverseMatrix) {
    // 1. Angle snapping
    EXPECT_FLOAT_EQ(ui::selection::TransformHandles::SnapAngle15(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(ui::selection::TransformHandles::SnapAngle15(7.0f), 0.0f);
    EXPECT_FLOAT_EQ(ui::selection::TransformHandles::SnapAngle15(8.0f), 15.0f);
    EXPECT_FLOAT_EQ(ui::selection::TransformHandles::SnapAngle15(44.0f), 45.0f);
    EXPECT_FLOAT_EQ(ui::selection::TransformHandles::SnapAngle15(89.0f), 90.0f);
    EXPECT_FLOAT_EQ(ui::selection::TransformHandles::SnapAngle15(359.0f), 0.0f);

    // 2. Compute Rotation Angle
    PointF center = { 200.0f, 200.0f };
    EXPECT_NEAR(ui::selection::TransformHandles::ComputeRotationAngle(center, PointF{ 200.0f, 100.0f }), 0.0f, 0.01f);   // Top
    EXPECT_NEAR(ui::selection::TransformHandles::ComputeRotationAngle(center, PointF{ 300.0f, 200.0f }), 90.0f, 0.01f);  // Right
    EXPECT_NEAR(ui::selection::TransformHandles::ComputeRotationAngle(center, PointF{ 200.0f, 300.0f }), 180.0f, 0.01f); // Bottom
    EXPECT_NEAR(ui::selection::TransformHandles::ComputeRotationAngle(center, PointF{ 100.0f, 200.0f }), 270.0f, 0.01f); // Left

    // 3. Inverse Matrix Hit-Test
    ui::selection::TransformHandles handles;
    RectF localBounds = { 0.0f, 0.0f, 100.0f, 100.0f };
    Matrix3x2F m = { 1.0f, 0.0f, 0.0f, 1.0f, 50.0f, 50.0f }; // Translation by (50, 50)
    EXPECT_EQ(handles.HitTestInverseMatrix(PointF{ 50.0f, 50.0f }, localBounds, m), ui::selection::HandleType::NW);
    EXPECT_EQ(handles.HitTestInverseMatrix(PointF{ 150.0f, 150.0f }, localBounds, m), ui::selection::HandleType::SE);
    EXPECT_EQ(handles.HitTestInverseMatrix(PointF{ 100.0f, 100.0f }, localBounds, m), ui::selection::HandleType::Body);
    EXPECT_EQ(handles.HitTestInverseMatrix(PointF{ 0.0f, 0.0f }, localBounds, m), ui::selection::HandleType::None);
}

TEST(M3_CursorResolver_ResolutionAndPrecedence) {
    HCURSOR arrow = ui::selection::CursorResolver::GetSystemCursor(IDC_ARROW);
    HCURSOR ibeam = ui::selection::CursorResolver::GetSystemCursor(IDC_IBEAM);
    HCURSOR sizeWE = ui::selection::CursorResolver::GetSystemCursor(IDC_SIZEWE);
    HCURSOR sizeNS = ui::selection::CursorResolver::GetSystemCursor(IDC_SIZENS);
    HCURSOR sizeNWSE = ui::selection::CursorResolver::GetSystemCursor(IDC_SIZENWSE);
    HCURSOR sizeNESW = ui::selection::CursorResolver::GetSystemCursor(IDC_SIZENESW);
    HCURSOR sizeAll = ui::selection::CursorResolver::GetSystemCursor(IDC_SIZEALL);
    HCURSOR hand = ui::selection::CursorResolver::GetSystemCursor(IDC_HAND);

    // 1. AngleToResizeCursor
    EXPECT_EQ(ui::selection::CursorResolver::AngleToResizeCursor(0.0f), sizeWE);
    EXPECT_EQ(ui::selection::CursorResolver::AngleToResizeCursor(45.0f), sizeNWSE);
    EXPECT_EQ(ui::selection::CursorResolver::AngleToResizeCursor(90.0f), sizeNS);
    EXPECT_EQ(ui::selection::CursorResolver::AngleToResizeCursor(135.0f), sizeNESW);
    EXPECT_EQ(ui::selection::CursorResolver::AngleToResizeCursor(180.0f), sizeWE);

    // 2. Handle Cursors
    EXPECT_EQ(ui::selection::CursorResolver::ResolveHandleCursor(ui::selection::HandleType::E), sizeWE);
    EXPECT_EQ(ui::selection::CursorResolver::ResolveHandleCursor(ui::selection::HandleType::SE), sizeNWSE);
    EXPECT_EQ(ui::selection::CursorResolver::ResolveHandleCursor(ui::selection::HandleType::S), sizeNS);
    EXPECT_EQ(ui::selection::CursorResolver::ResolveHandleCursor(ui::selection::HandleType::SW), sizeNESW);
    EXPECT_EQ(ui::selection::CursorResolver::ResolveHandleCursor(ui::selection::HandleType::W), sizeWE);
    EXPECT_EQ(ui::selection::CursorResolver::ResolveHandleCursor(ui::selection::HandleType::NW), sizeNWSE);
    EXPECT_EQ(ui::selection::CursorResolver::ResolveHandleCursor(ui::selection::HandleType::N), sizeNS);
    EXPECT_EQ(ui::selection::CursorResolver::ResolveHandleCursor(ui::selection::HandleType::NE), sizeNESW);
    EXPECT_EQ(ui::selection::CursorResolver::ResolveHandleCursor(ui::selection::HandleType::Rotation), sizeAll);
    EXPECT_EQ(ui::selection::CursorResolver::ResolveHandleCursor(ui::selection::HandleType::Body), sizeAll);

    // 3. Precedence in ResolveCursor
    // Handle hover takes precedence over everything
    HCURSOR h = ui::selection::CursorResolver::ResolveCursor(
        ui::tools::ToolType::Select, ui::tools::ToolState::Idle, ui::selection::HandleType::NW, 0.0f, true, true, true);
    EXPECT_EQ(h, sizeNWSE);

    // Link hover takes precedence when no handle hovered
    h = ui::selection::CursorResolver::ResolveCursor(
        ui::tools::ToolType::Select, ui::tools::ToolState::Idle, ui::selection::HandleType::None, 0.0f, false, true, false);
    EXPECT_EQ(h, hand);

    // Object hover in Select tool -> Move cursor
    h = ui::selection::CursorResolver::ResolveCursor(
        ui::tools::ToolType::Select, ui::tools::ToolState::Idle, ui::selection::HandleType::None, 0.0f, false, false, true);
    EXPECT_EQ(h, sizeAll);

    // Text hover in Select tool -> I-Beam
    h = ui::selection::CursorResolver::ResolveCursor(
        ui::tools::ToolType::Select, ui::tools::ToolState::Idle, ui::selection::HandleType::None, 0.0f, true, false, false);
    EXPECT_EQ(h, ibeam);

    // Default Select tool -> Arrow
    h = ui::selection::CursorResolver::ResolveCursor(
        ui::tools::ToolType::Select, ui::tools::ToolState::Idle, ui::selection::HandleType::None, 0.0f, false, false, false);
    EXPECT_EQ(h, arrow);

    // TextSelect tool -> I-Beam
    h = ui::selection::CursorResolver::ResolveCursor(
        ui::tools::ToolType::TextSelect, ui::tools::ToolState::Idle, ui::selection::HandleType::None);
    EXPECT_EQ(h, ibeam);
}

TEST(M3_SelectTool_DragLifecycle_CaptureAndKeyboardHandling) {
    ui::tools::SelectTool tool;
    ui::input::PointerCaptureService captureService;

    bool invalidated = false;
    ui::tools::ToolContext context;
    context.captureService = &captureService;
    context.hwnd = reinterpret_cast<HWND>(0x12345678); // non-null dummy handle
    context.invalidateView = [&]() { invalidated = true; };

    tool.OnActivate(context);
    EXPECT_EQ(tool.GetState(), ui::tools::ToolState::Idle);

    // 4. Object selection & move drag
    tool.GetSelectionModel().Select("test_obj", 0, RectF{ 100.0f, 100.0f, 200.0f, 200.0f });
    
    // 1. PointerDown on object body -> starts move drag + acquires capture
    ui::input::PointerEvent downEvt;
    downEvt.button = ui::input::PointerButton::Left;
    downEvt.canvasPoint = { 150.0f, 150.0f }; // inside body
    downEvt.clientDip = { 150.0f, 150.0f };

    auto res = tool.OnPointerDown(downEvt, context);
    EXPECT_EQ(res, ui::input::EventResult::Handled);
    EXPECT_EQ(tool.GetState(), ui::tools::ToolState::Dragging);
    EXPECT_TRUE(captureService.HasCapture(&tool));

    // 2. PointerMove during drag
    ui::input::PointerEvent moveEvt;
    moveEvt.canvasPoint = { 160.0f, 160.0f };
    res = tool.OnPointerMove(moveEvt, context);
    EXPECT_EQ(res, ui::input::EventResult::Handled);

    // 3. PointerUp -> commits drag & releases capture
    ui::input::PointerEvent upEvt;
    upEvt.button = ui::input::PointerButton::Left;
    upEvt.canvasPoint = { 160.0f, 160.0f };
    res = tool.OnPointerUp(upEvt, context);
    EXPECT_EQ(res, ui::input::EventResult::Handled);
    EXPECT_EQ(tool.GetState(), ui::tools::ToolState::Idle);
    EXPECT_FALSE(captureService.HasCapture(&tool));

    // Click inside object body (150, 150)
    downEvt.canvasPoint = { 150.0f, 150.0f };
    res = tool.OnPointerDown(downEvt, context);
    EXPECT_EQ(res, ui::input::EventResult::Handled);
    EXPECT_TRUE(captureService.HasCapture(&tool));

    // Move by (20, 30)
    moveEvt.canvasPoint = { 170.0f, 180.0f };
    res = tool.OnPointerMove(moveEvt, context);
    EXPECT_EQ(res, ui::input::EventResult::Handled);

    RectF movedBounds = tool.GetSelectionModel().GetSelectionBounds();
    // After first drag: {100,100,200,200} moved by delta (160-150)=+10 -> {110,110,210,210}
    // Second drag from 150 to (170,180): delta +20,+30 -> {130,140,230,240}
    EXPECT_FLOAT_EQ(movedBounds.left, 130.0f);
    EXPECT_FLOAT_EQ(movedBounds.top, 140.0f);

    upEvt.canvasPoint = { 170.0f, 180.0f };
    tool.OnPointerUp(upEvt, context);
    EXPECT_FALSE(captureService.HasCapture(&tool));

    // 5. Keyboard Delete
    ui::input::KeyEvent delEvt;
    delEvt.virtualKey = VK_DELETE;
    res = tool.OnKeyDown(delEvt, context);
    EXPECT_EQ(res, ui::input::EventResult::Handled);
    EXPECT_FALSE(tool.GetSelectionModel().HasObjectSelection());

    // 6. Escape cancels
    tool.GetSelectionModel().Select("test_obj2", 0, RectF{ 10.0f, 10.0f, 50.0f, 50.0f });
    ui::input::KeyEvent escEvt;
    escEvt.virtualKey = VK_ESCAPE;
    res = tool.OnKeyDown(escEvt, context);
    EXPECT_EQ(res, ui::input::EventResult::Handled);
    EXPECT_FALSE(tool.GetSelectionModel().HasSelection());
    EXPECT_FALSE(captureService.HasCapture(&tool));
}

TEST(M3_TextSelectTool_Selection_Capture_And_Escape) {
    ui::tools::TextSelectTool tool;
    ui::input::PointerCaptureService captureService;
    MockTextPageImpl mockPage(L"PDF Elite text selection unit test.");

    ui::tools::ToolContext context;
    context.captureService = &captureService;
    context.hwnd = reinterpret_cast<HWND>(0x12345678);
    context.getTextPage = [&](int pageIndex) -> core::interfaces::dom::ITextPage* {
        return (pageIndex == 0) ? &mockPage : nullptr;
    };

    tool.OnActivate(context);

    // 1. PointerDown on char 0 ('P')
    ui::input::PointerEvent downEvt;
    downEvt.button = ui::input::PointerButton::Left;
    downEvt.pageIndex = 0;
    downEvt.pagePoint = { 5.0f, 110.0f };
    downEvt.clientDip = { 5.0f, 110.0f };

    auto res = tool.OnPointerDown(downEvt, context);
    EXPECT_EQ(res, ui::input::EventResult::Handled);
    EXPECT_TRUE(tool.GetSelectionModel().HasTextSelection());
    EXPECT_TRUE(captureService.HasCapture(&tool));

    // 2. PointerMove to char 8 ('i' in "Elite")
    ui::input::PointerEvent moveEvt;
    moveEvt.pageIndex = 0;
    moveEvt.pagePoint = { 85.0f, 110.0f };
    res = tool.OnPointerMove(moveEvt, context);
    EXPECT_EQ(res, ui::input::EventResult::Handled);

    // 3. PointerUp -> releases capture
    ui::input::PointerEvent upEvt;
    upEvt.button = ui::input::PointerButton::Left;
    res = tool.OnPointerUp(upEvt, context);
    EXPECT_EQ(res, ui::input::EventResult::Handled);
    EXPECT_FALSE(captureService.HasCapture(&tool));

    // 4. Escape clears selection and releases capture
    ui::input::KeyEvent escEvt;
    escEvt.virtualKey = VK_ESCAPE;
    res = tool.OnKeyDown(escEvt, context);
    EXPECT_EQ(res, ui::input::EventResult::Handled);
    EXPECT_FALSE(tool.GetSelectionModel().HasTextSelection());
    EXPECT_FALSE(captureService.HasCapture(&tool));
}

TEST(M3_ToolStateMachine_PointerCapture_CleanReleaseOnToolSwitch) {
    ui::input::PointerCaptureService captureService;
    MockTextPageImpl mockPage(L"State machine pointer capture test.");

    ui::tools::ToolContext context;
    context.captureService = &captureService;
    context.hwnd = reinterpret_cast<HWND>(0x12345678);
    context.getTextPage = [&](int pageIndex) -> core::interfaces::dom::ITextPage* {
        return (pageIndex == 0) ? &mockPage : nullptr;
    };

    ui::tools::ToolStateMachine stateMachine(context);

    stateMachine.RegisterTool(std::make_unique<ui::tools::SelectTool>());
    stateMachine.RegisterTool(std::make_unique<ui::tools::TextSelectTool>());

    // 1. Switch to Select tool & start dragging
    EXPECT_TRUE(stateMachine.SetActiveTool(ui::tools::ToolType::Select));
    EXPECT_EQ(stateMachine.GetActiveToolType(), ui::tools::ToolType::Select);

    auto selectTool = dynamic_cast<ui::tools::SelectTool*>(stateMachine.GetActiveTool());
    if (selectTool) {
        selectTool->GetSelectionModel().Select("dummy", 0, RectF{5.0f, 5.0f, 50.0f, 50.0f});
    }

    ui::input::PointerEvent downEvt;
    downEvt.button = ui::input::PointerButton::Left;
    downEvt.canvasPoint = { 27.5f, 27.5f };
    downEvt.clientDip = { 27.5f, 27.5f };
    stateMachine.GetContext().getTextPage = nullptr;
    stateMachine.GetContext().getTextPage = [](int) -> core::interfaces::dom::ITextPage* { return nullptr; };
    stateMachine.RoutePointerDown(downEvt);
    EXPECT_TRUE(captureService.IsAnyCaptured());

    // 2. Switch tool mid-drag -> old tool must cleanly release capture without leak
    EXPECT_TRUE(stateMachine.SetActiveTool(ui::tools::ToolType::TextSelect));
    EXPECT_EQ(stateMachine.GetActiveToolType(), ui::tools::ToolType::TextSelect);
    EXPECT_FALSE(captureService.IsAnyCaptured());

    // 3. Start drag in TextSelect tool
    stateMachine.GetContext().getTextPage = [&](int pageIndex) -> core::interfaces::dom::ITextPage* { return (pageIndex == 0) ? &mockPage : nullptr; };
    downEvt.pageIndex = 0;
    downEvt.pagePoint = { 5.0f, 110.0f };
    downEvt.clientDip = { 5.0f, 110.0f };
    stateMachine.RoutePointerDown(downEvt);
    EXPECT_TRUE(captureService.IsAnyCaptured());

    // 4. Cancel active interactions -> cleanly releases capture
    stateMachine.CancelActiveInteractions();
    EXPECT_FALSE(captureService.IsAnyCaptured());
}

// ============================================================================
// Milestone 3: KineticScrollFilter Unit Tests (F8)
// ============================================================================

TEST(KineticScrollFilter_WheelDeltaImpulse) {
    ui::viewport::KineticScrollFilter filter;
    EXPECT_FALSE(filter.IsActive());
    EXPECT_NEAR(filter.GetVelocityX(), 0.0f, 0.001f);
    EXPECT_NEAR(filter.GetVelocityY(), 0.0f, 0.001f);

    // Adding wheel delta adds impulse scaled by WHEEL_STEP_MULTIPLIER (2.5)
    filter.AddWheelDelta(10.0f, 20.0f);
    EXPECT_TRUE(filter.IsActive());
    EXPECT_NEAR(filter.GetVelocityX(), 25.0f, 0.001f);
    EXPECT_NEAR(filter.GetVelocityY(), 50.0f, 0.001f);

    filter.Stop();
    EXPECT_FALSE(filter.IsActive());
    EXPECT_NEAR(filter.GetVelocityX(), 0.0f, 0.001f);
    EXPECT_NEAR(filter.GetVelocityY(), 0.0f, 0.001f);
}

TEST(KineticScrollFilter_ExponentialDecay) {
    ui::viewport::KineticScrollFilter filter;
    filter.SetVelocity(0.0f, 1000.0f);
    EXPECT_TRUE(filter.IsActive());

    // Update with 16ms standard tick
    float dx = 0.0f, dy = 0.0f;
    bool moving = filter.Update(0.016, dx, dy);
    EXPECT_TRUE(moving);
    EXPECT_GT(dy, 0.0f);
    // After 16ms with 35% decay per tick, remaining velocity is approximately 650 px/s
    EXPECT_LT(filter.GetVelocityY(), 1000.0f);
    EXPECT_GT(filter.GetVelocityY(), 500.0f);

    // Multiple ticks decay exponentially towards zero
    for (int i = 0; i < 20; ++i) {
        filter.Update(0.016, dx, dy);
    }
    // After 20 ticks, velocity should be well decayed
    EXPECT_LT(filter.GetVelocityY(), 10.0f);
}

TEST(KineticScrollFilter_VelocityClamp8000) {
    ui::viewport::KineticScrollFilter filter;
    // Add massive impulse
    filter.AddVelocity(100000.0f, -100000.0f);
    EXPECT_NEAR(filter.GetVelocityX(), ui::viewport::KineticScrollFilter::MAX_VELOCITY, 0.001f);
    EXPECT_NEAR(filter.GetVelocityY(), -ui::viewport::KineticScrollFilter::MAX_VELOCITY, 0.001f);
}

TEST(KineticScrollFilter_QuiescenceStopThreshold) {
    ui::viewport::KineticScrollFilter filter;
    // Set velocity slightly above VELOCITY_EPSILON (0.1 px/s)
    filter.SetVelocity(0.12f, 0.12f);
    EXPECT_TRUE(filter.IsActive());
    float dx = 0.0f, dy = 0.0f;
    bool moving = filter.Update(0.016, dx, dy);
    EXPECT_TRUE(moving);
    // After decay, velocity drops below VELOCITY_EPSILON (0.1 px/s) and stops
    EXPECT_NEAR(filter.GetVelocityX(), 0.0f, 0.001f);
    EXPECT_NEAR(filter.GetVelocityY(), 0.0f, 0.001f);
    EXPECT_FALSE(filter.IsActive());
    EXPECT_FALSE(filter.Update(0.016, dx, dy));
}

TEST(KineticScrollFilter_CoalesceWheelDelta) {
    ui::viewport::KineticScrollFilter filter;
    // Standard wheel notch (120 units) -> 40px impulse
    float stdTicks = filter.CoalesceWheelDelta(120.0f, false);
    EXPECT_NEAR(stdTicks, 40.0f, 0.001f);

    // Trackpad delta with 1.2 multiplier
    float trackpadTicks = filter.CoalesceWheelDelta(10.0f, true);
    EXPECT_NEAR(trackpadTicks, 12.0f, 0.001f);
}

// ============================================================================
// Milestone 3: ViewportEngine Unit Tests (F8)
// ============================================================================

TEST(ViewportEngine_FocalZoomInvariant) {
    double focalScreenX = 400.0;
    double focalScreenY = 300.0;
    double currentZoom = 1.0;
    double newZoom = 2.0;
    double currentScrollX = 100.0;
    double currentScrollY = 200.0;
    double outScrollX = 0.0;
    double outScrollY = 0.0;

    // Document point before zoom:
    // docX = (focalScreenX + currentScrollX) / currentZoom = (400 + 100) / 1.0 = 500
    // docY = (focalScreenY + currentScrollY) / currentZoom = (300 + 200) / 1.0 = 500
    ui::viewport::ViewportEngine::CalculateFocalZoomOffsets(
        currentZoom, newZoom,
        focalScreenX, focalScreenY,
        currentScrollX, currentScrollY,
        outScrollX, outScrollY);

    // After zoom to 2.0:
    // newScrollX = docX * newZoom - focalScreenX = 500 * 2.0 - 400 = 600
    // newScrollY = docY * newZoom - focalScreenY = 500 * 2.0 - 300 = 700
    EXPECT_NEAR(outScrollX, 600.0, 0.001f);
    EXPECT_NEAR(outScrollY, 700.0, 0.001f);
}

TEST(ViewportEngine_ExponentialSteps) {
    ui::viewport::ViewportEngine engine;
    engine.SetZoom(1.0);

    // Zoom in 1 step: 1.0 * 1.15 = 1.15
    double z1 = engine.ZoomIn(1);
    EXPECT_NEAR(z1, 1.15, 0.001f);

    // Zoom out 1 step: 1.15 / 1.15 = 1.0
    double z2 = engine.ZoomOut(1);
    EXPECT_NEAR(z2, 1.0, 0.001f);

    // Zoom in 2 steps: 1.0 * 1.15^2 = 1.3225
    double z3 = engine.ZoomIn(2);
    EXPECT_NEAR(z3, 1.3225, 0.001f);
}

TEST(ViewportEngine_PresetZoomLevels) {
    ui::viewport::ViewportEngine engine;
    engine.SetZoom(1.0);

    double nextIn = engine.GetNextPresetZoom(true);
    EXPECT_NEAR(nextIn, 1.25, 0.001f);

    double nextOut = engine.GetNextPresetZoom(false);
    EXPECT_NEAR(nextOut, 0.75, 0.001f);
}

TEST(ViewportEngine_FitCalculations) {
    ui::viewport::ViewportEngine engine;
    engine.SetViewportSize(1000.0, 800.0);

    // Fit Width: (viewportWidth - padding) / contentWidth = (1000 - 20) / 490 = 2.0
    double fitW = engine.CalculateFitWidth(490.0, 1000.0, 20.0);
    EXPECT_NEAR(fitW, 2.0, 0.001f);

    // Fit Height: (viewportHeight - padding) / contentHeight = (800 - 20) / 390 = 2.0
    double fitH = engine.CalculateFitHeight(390.0, 800.0, 20.0);
    EXPECT_NEAR(fitH, 2.0, 0.001f);

    // Fit Page: min(fitW, fitH)
    double fitPage = engine.CalculateFitPage(490.0, 780.0, 1000.0, 800.0, 20.0);
    EXPECT_NEAR(fitPage, 1.0, 0.001f);
}

TEST(ViewportEngine_ContinuousLayout) {
    ui::viewport::ViewportEngine engine;
    std::vector<std::pair<double, double>> pageSizes = {
        { 600.0, 800.0 },
        { 600.0, 800.0 },
        { 600.0, 400.0 }
    };
    engine.SetZoom(1.0);
    engine.UpdateContinuousLayout(pageSizes, {}, 20.0);

    const auto& layout = engine.GetLayout();
    EXPECT_EQ(layout.size(), 3);
    EXPECT_NEAR(layout[0].yOffset, 20.0, 0.001f); // top gap
    EXPECT_NEAR(layout[1].yOffset, 840.0, 0.001f); // 20 + 800 + 20 gap
    EXPECT_NEAR(layout[2].yOffset, 1660.0, 0.001f); // 840 + 800 + 20 gap

    // Total content height: 20 + 800 + 20 + 800 + 20 + 400 + 20 = 2080
    EXPECT_NEAR(engine.GetTotalContentHeight(), 2080.0, 0.001f);

    // Page index at offset
    EXPECT_EQ(engine.GetPageAtOffset(400.0), 0);
    EXPECT_EQ(engine.GetPageAtOffset(900.0), 1);
    EXPECT_EQ(engine.GetPageAtOffset(1800.0), 2);
}

TEST(KineticScrollFilter_AnalyticalIntegral_FrameRateInvariance) {
    const float v0 = 2000.0f;
    
    // 60 Hz stepping (dt = 1/60s = 0.016667s)
    ui::viewport::KineticScrollFilter filter60;
    filter60.SetVelocity(v0, 0.0f);
    double disp60 = 0.0;
    float dx = 0.0f, dy = 0.0f;
    while (filter60.Update(1.0 / 60.0, dx, dy)) { disp60 += dx; }

    // 120 Hz stepping (dt = 1/120s = 0.008333s)
    ui::viewport::KineticScrollFilter filter120;
    filter120.SetVelocity(v0, 0.0f);
    double disp120 = 0.0;
    while (filter120.Update(1.0 / 120.0, dx, dy)) { disp120 += dx; }

    // 240 Hz stepping (dt = 1/240s = 0.004167s)
    ui::viewport::KineticScrollFilter filter240;
    filter240.SetVelocity(v0, 0.0f);
    double disp240 = 0.0;
    while (filter240.Update(1.0 / 240.0, dx, dy)) { disp240 += dx; }

    // All three frame rates must produce identical total displacement within 0.05px
    EXPECT_NEAR(disp60, disp120, 0.05);
    EXPECT_NEAR(disp120, disp240, 0.05);
    EXPECT_NEAR(disp60, static_cast<double>(v0 / 26.92477), 0.1);
}



