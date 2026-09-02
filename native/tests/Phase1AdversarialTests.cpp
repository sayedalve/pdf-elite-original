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
#include <random>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commctrl.h>
#include <d2d1.h>
#include <dwrite.h>

#include <fpdfview.h>
#include <fpdf_edit.h>
#include <fpdf_text.h>
#include <fpdf_annot.h>

#include "../src/pdf_engine/src/PdfiumLibrary.h"
#include "../src/pdf_engine/src/PdfDocument.h"
#include "../src/pdf_engine/src/PdfPage.h"
#include "../src/pdf_engine/src/PdfTextObject.h"
#include "../src/pdf_engine/src/PdfAnnotation.h"
#include "../src/core/interfaces/dom/CommandStack.h"
#include "../src/core/interfaces/dom/ICommand.h"
#include "../src/core/CoordinateConverter.h"
#include "../src/pdf_engine/src/commands/PageCommands.h"
#include "../src/pdf_engine/src/commands/AnnotationCommands.h"
#include "../src/pdf_engine/src/commands/TextCommands.h"
#include "../src/pdf_engine/src/commands/ImageCommands.h"
#include "../src/pdf_engine/src/commands/MacroCommand.h"

#include "../src/ui/src/interaction/InteractionManager.h"
#include "../src/ui/src/interaction/SelectionModel.h"
#include "../src/ui/src/interaction/TextSelectableObject.h"
#include "../src/ui/src/interaction/AnnotationSelectableObject.h"
#include "../src/ui/src/PdfViewer.h"
#include "../src/ui/src/GraphicsDevice.h"
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

static void EnsureFixtures() {
    std::error_code ec;
    std::filesystem::create_directories("tests/fixtures/output", ec);
    std::filesystem::create_directories("tests/fixtures/basic", ec);
    WriteTestFile(L"tests/fixtures/basic/minimal.pdf", kMinimalPdf, sizeof(kMinimalPdf) - 1);
}

static std::shared_ptr<PdfDocument> CreateTestDoc() {
    EnsureFixtures();
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    if (!res.has_value()) {
        throw std::runtime_error("Failed to load minimal test PDF");
    }
    return std::shared_ptr<PdfDocument>(std::move(res.value));
}

} // namespace

// ============================================================================
// CHALLENGE 1: TOOL SWITCHING, POINTER STATE & MOUSE CAPTURE ROBUSTNESS
// ============================================================================

TEST(Challenge1_1_ToolSwitchPointerStateTransitions) {
    EnsureFixtures();
    ui::interaction::InteractionManager im;
    
    im.pageToView = [](double px, double py, int, double& vx, double& vy) { vx = px; vy = py; };
    im.viewToPage = [](double vx, double vy, double& px, double& py, int& p) { px = vx; py = vy; p = 0; };
    
    EXPECT_TRUE(im.GetCursor() == nullptr);
    
    im.EnterNewTextMode(100.0, 100.0);
    HCURSOR cur = im.GetCursor();
    EXPECT_TRUE(cur != nullptr);
    
    im.CancelTextEdit();
    EXPECT_FALSE(im.IsEditingText());
    EXPECT_TRUE(im.GetCursor() == nullptr);
}

TEST(Challenge1_2_ToolSwitchCancelsActiveInteractions) {
    EnsureFixtures();
    PdfViewer viewer;
    viewer.SetToolMode(ToolMode::Pan);
    EXPECT_EQ(static_cast<int>(viewer.GetToolMode()), static_cast<int>(ToolMode::Pan));
    
    viewer.GetInteractionManager().EnterNewTextMode(50.0, 50.0);
    
    viewer.SetToolMode(ToolMode::Select);
    EXPECT_FALSE(viewer.GetInteractionManager().IsEditingText());
    EXPECT_EQ(static_cast<int>(viewer.GetToolMode()), static_cast<int>(ToolMode::Select));
    
    viewer.SetToolMode(ToolMode::Highlight);
    EXPECT_EQ(static_cast<int>(viewer.GetToolMode()), static_cast<int>(ToolMode::Highlight));
    
    viewer.SetToolMode(ToolMode::AddText);
    EXPECT_EQ(static_cast<int>(viewer.GetToolMode()), static_cast<int>(ToolMode::AddText));
}

TEST(Challenge1_3_RapidToolSwitching500CyclesStress) {
    EnsureFixtures();
    PdfViewer viewer;
    
    ToolMode modes[] = { ToolMode::Pan, ToolMode::Select, ToolMode::AddText, ToolMode::Highlight, ToolMode::EditText };
    
    for (int i = 0; i < 500; ++i) {
        ToolMode m = modes[i % 5];
        viewer.SetToolMode(m);
        EXPECT_EQ(static_cast<int>(viewer.GetToolMode()), static_cast<int>(m));
        if (i % 10 == 0) {
            viewer.CancelActiveInteractions();
        }
    }
}

// ============================================================================
// CHALLENGE 2: TEXT CREATION, IN-PLACE EDIT & ATOMIC SAVING TO DISK
// ============================================================================

TEST(Challenge2_1_TextCreationPrecisionAndReloadVerification) {
    EnsureFixtures();
    const wchar_t* outPath = L"tests/fixtures/output/challenge_text_create.pdf";
    
    // Step 1: Create PDF with text
    {
        auto doc = CreateTestDoc();
        
        RectF textBounds{ 120.0f, 400.0f, 320.0f, 440.0f };
        auto cmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
            doc.get(), 0, L"Empirical Challenge Text 2026", textBounds, "Arial", 16.0f, 255, 0, 0, 255
        );
        EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(cmd)));
        
        auto page = doc->GetPage(0);
        auto texts = page->GetTextObjects();
        EXPECT_EQ(texts.size(), 1ull);
        EXPECT_TRUE(texts[0]->GetText() == L"Empirical Challenge Text 2026");
        
        // Save atomically
        EXPECT_TRUE(doc->SaveAs(outPath));
    }
    
    // Step 2: Reload from disk with brand new document instance
    {
        auto reloadRes = PdfDocument::LoadFromFile(outPath);
        EXPECT_TRUE(reloadRes.has_value());
        auto reloadDoc = std::move(reloadRes.value);
        
        auto page = reloadDoc->GetPage(0);
        EXPECT_TRUE(page != nullptr);
        
        auto texts = page->GetTextObjects();
        EXPECT_EQ(texts.size(), 1ull);
        EXPECT_TRUE(texts[0]->GetText() == L"Empirical Challenge Text 2026");
        
        // Validate coordinates: origin X around 120, Y around 400
        RectF bounds = texts[0]->GetBounds();
        EXPECT_TRUE(std::abs(bounds.left - 120.0f) < 2.0f);
        EXPECT_TRUE(std::abs(bounds.bottom - 400.0f) < 2.0f);
    }
}

TEST(Challenge2_2_TextInPlaceEditMultilinePreservationAndSaving) {
    EnsureFixtures();
    const wchar_t* outPath = L"tests/fixtures/output/challenge_multiline_edit.pdf";
    
    // Step 1: Create initial text and then edit multiline
    {
        auto doc = CreateTestDoc();
        
        RectF textBounds{ 100.0f, 500.0f, 300.0f, 540.0f };
        auto cmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
            doc.get(), 0, L"Initial Line", textBounds, "Arial", 12.0f, 0, 0, 0, 255
        );
        EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(cmd)));
        
        auto page = doc->GetPage(0);
        auto texts = page->GetTextObjects();
        EXPECT_EQ(texts.size(), 1ull);
        
        // Edit to 3 lines
        std::vector<core::interfaces::dom::TextLineData> newLines = {
            { L"Header Line A", 0.0f, 0.0f, 100.0f, 14.0f },
            { L"Body Paragraph Line B", 0.0f, 16.0f, 150.0f, 14.0f },
            { L"Footer Line C", 0.0f, 32.0f, 90.0f, 14.0f }
        };
        
        auto editCmd = std::make_unique<pdf_engine::commands::EditMultilineTextCommand>(
            texts[0], std::vector<core::interfaces::dom::TextLineData>{}, newLines
        );
        EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(editCmd)));
        
        EXPECT_TRUE(doc->SaveAs(outPath));
    }
    
    // Step 2: Reload and verify multiline preservation
    {
        auto reloadRes = PdfDocument::LoadFromFile(outPath);
        EXPECT_TRUE(reloadRes.has_value());
        auto reloadDoc = std::move(reloadRes.value);
        
        auto page = reloadDoc->GetPage(0);
        auto texts = page->GetTextObjects();
        EXPECT_EQ(texts.size(), 1ull);
        
        std::wstring content = texts[0]->GetText();
        EXPECT_TRUE(content.find(L"Header Line A") != std::wstring::npos);
        EXPECT_TRUE(content.find(L"Body Paragraph Line B") != std::wstring::npos);
        EXPECT_TRUE(content.find(L"Footer Line C") != std::wstring::npos);
    }
}

TEST(Challenge2_3_AtomicSaveFileSafetyAndTmpCleanup) {
    EnsureFixtures();
    const wchar_t* outPath = L"tests/fixtures/output/challenge_atomic_test.pdf";
    std::wstring tmpPath = std::wstring(outPath) + L".tmp";
    
    auto doc = CreateTestDoc();
    
    // Execute save
    EXPECT_TRUE(doc->SaveAs(outPath));
    EXPECT_TRUE(std::filesystem::exists(outPath));
    // Tmp file must NOT remain behind
    EXPECT_FALSE(std::filesystem::exists(tmpPath));
    
    // Save again over existing file
    EXPECT_TRUE(doc->SaveAs(outPath));
    EXPECT_TRUE(std::filesystem::exists(outPath));
    EXPECT_FALSE(std::filesystem::exists(tmpPath));
}

// ============================================================================
// CHALLENGE 3: UNDO/REDO COMPLETE DETACHMENT FROM PDFIUM DOCUMENT TREE
// ============================================================================

TEST(Challenge3_1_TextUndoRedoCompleteDetachmentFromDocumentTree) {
    EnsureFixtures();
    auto doc = CreateTestDoc();
    
    auto page = doc->GetPage(0);
    EXPECT_EQ(page->GetTextObjects().size(), 0ull);
    
    // 1. Insert Text
    auto cmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Transient Object", RectF{50.0f, 100.0f, 200.0f, 130.0f}, "Arial", 12.0f, 0, 0, 0, 255
    );
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(cmd)));
    EXPECT_EQ(page->GetTextObjects().size(), 1ull);
    EXPECT_TRUE(page->GetTextObjects()[0]->GetText() == L"Transient Object");
    
    // 2. Undo -> Must completely detach from page tree
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(page->GetTextObjects().size(), 0ull);
    
    // Text page inspection: must not contain the text
    auto textPage = page->LoadTextPage();
    if (textPage) {
        EXPECT_EQ(textPage->GetCharCount(), 0);
    }
    
    // 3. Redo -> Must re-attach cleanly
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_EQ(page->GetTextObjects().size(), 1ull);
    EXPECT_TRUE(page->GetTextObjects()[0]->GetText() == L"Transient Object");
    
    // 4. Undo again -> Detached cleanly
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(page->GetTextObjects().size(), 0ull);
}

TEST(Challenge3_2_AnnotationUndoRedoCompleteDetachment) {
    EnsureFixtures();
    auto doc = CreateTestDoc();
    
    auto page = doc->GetPage(0);
    EXPECT_EQ(page->GetAnnotations().size(), 0ull);
    
    // Insert Highlight Annotation
    RectF bounds{ 50.0f, 100.0f, 200.0f, 120.0f };
    auto annotCmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 0, core::interfaces::dom::AnnotationType::Highlight, bounds
    );
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(annotCmd)));
    EXPECT_EQ(page->GetAnnotations().size(), 1ull);
    
    // Undo -> Annotation removed completely
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(page->GetAnnotations().size(), 0ull);
    
    // Redo -> Annotation restored
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_EQ(page->GetAnnotations().size(), 1ull);
    
    // Undo
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(page->GetAnnotations().size(), 0ull);
}

TEST(Challenge3_3_ConsecutiveUndoRedo100CyclesStress) {
    EnsureFixtures();
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    
    // Add 3 commands: Text 1, Text 2, Highlight
    doc->GetCommandStack().ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Object 1", RectF{10.0f, 10.0f, 100.0f, 30.0f}, "Arial", 12.0f, 0, 0, 0, 255
    ));
    doc->GetCommandStack().ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Object 2", RectF{10.0f, 40.0f, 100.0f, 60.0f}, "Arial", 12.0f, 0, 0, 0, 255
    ));
    
    RectF annotBounds{ 10.0f, 60.0f, 100.0f, 80.0f };
    doc->GetCommandStack().ExecuteCommand(std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 0, core::interfaces::dom::AnnotationType::Highlight, annotBounds
    ));
    
    EXPECT_EQ(page->GetTextObjects().size(), 2ull);
    EXPECT_EQ(page->GetAnnotations().size(), 1ull);
    
    // 100 full undo-redo cycles
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(doc->GetCommandStack().Undo()); // undo highlight
        EXPECT_TRUE(doc->GetCommandStack().Undo()); // undo text 2
        EXPECT_TRUE(doc->GetCommandStack().Undo()); // undo text 1
        EXPECT_EQ(page->GetTextObjects().size(), 0ull);
        EXPECT_EQ(page->GetAnnotations().size(), 0ull);
        
        EXPECT_TRUE(doc->GetCommandStack().Redo()); // redo text 1
        EXPECT_TRUE(doc->GetCommandStack().Redo()); // redo text 2
        EXPECT_TRUE(doc->GetCommandStack().Redo()); // redo highlight
        EXPECT_EQ(page->GetTextObjects().size(), 2ull);
        EXPECT_EQ(page->GetAnnotations().size(), 1ull);
    }
}

// ============================================================================
// CHALLENGE 4: SELECTION BOUNDARY CALCULATIONS, 9 HANDLES & DRAG MOVE
// ============================================================================

TEST(Challenge4_1_Selection9HandlesGeometry) {
    EnsureFixtures();
    ui::interaction::InteractionManager im;
    
    // Setup callbacks
    im.pageToView = [](double px, double py, int /*pageIndex*/, double& vx, double& vy) {
        vx = px;
        vy = py;
    };
    im.viewToPage = [](double vx, double vy, double& px, double& py, int& pageIndex) {
        px = vx;
        py = vy;
        pageIndex = 0;
    };
    
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    
    auto textObj = page->InsertTextObject(L"Selectable Text", RectF{100.0f, 100.0f, 300.0f, 200.0f}, "Arial", 12.0f);
    auto selObj = std::make_shared<ui::interaction::TextSelectableObject>(textObj, 0);
    im.AddObject(selObj);
    im.GetSelectionModel().Select(selObj);
    
    // Hit-testing on TopLeft Handle (100, 100)
    ui::interaction::HitResult resTL = im.OnLButtonDown(100.0, 100.0, false);
    EXPECT_TRUE(resTL == ui::interaction::HitResult::Handle);
    im.OnLButtonUp(100.0, 100.0);
    
    // Hit-testing on Right Handle (300, 150)
    ui::interaction::HitResult resR = im.OnLButtonDown(300.0, 150.0, false);
    EXPECT_TRUE(resR == ui::interaction::HitResult::Handle);
    im.OnLButtonUp(300.0, 150.0);
    
    // Hit-testing on BottomRight Handle (300, 200)
    ui::interaction::HitResult resBR = im.OnLButtonDown(300.0, 200.0, false);
    EXPECT_TRUE(resBR == ui::interaction::HitResult::Handle);
    im.OnLButtonUp(300.0, 200.0);
    
    // Hit-testing on Rotation Handle (200, 80)
    ui::interaction::HitResult resRot = im.OnLButtonDown(200.0, 80.0, false);
    EXPECT_TRUE(resRot == ui::interaction::HitResult::Handle);
    im.OnLButtonUp(200.0, 80.0);
    
    // Hit-testing on Object Body (200, 150)
    ui::interaction::HitResult resBody = im.OnLButtonDown(200.0, 150.0, false);
    EXPECT_TRUE(resBody == ui::interaction::HitResult::Object);
    im.OnLButtonUp(200.0, 150.0);
}

TEST(Challenge4_2_DragMoveNoObjectDuplicationOrMemoryLeak) {
    EnsureFixtures();
    ui::interaction::InteractionManager im;
    
    im.pageToView = [](double px, double py, int /*pageIndex*/, double& vx, double& vy) {
        vx = px;
        vy = py;
    };
    im.viewToPage = [](double vx, double vy, double& px, double& py, int& pageIndex) {
        px = vx;
        py = vy;
        pageIndex = 0;
    };
    
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    
    auto textObj = page->InsertTextObject(L"Draggable Text", RectF{100.0f, 100.0f, 250.0f, 140.0f}, "Arial", 12.0f);
    auto selObj = std::make_shared<ui::interaction::TextSelectableObject>(textObj, 0);
    im.AddObject(selObj);
    im.GetSelectionModel().Select(selObj);
    
    EXPECT_EQ(im.GetObjects().size(), 1ull);
    
    // Simulate 500 mouse moves during drag
    im.OnLButtonDown(150.0, 120.0, false);
    for (int i = 1; i <= 500; ++i) {
        im.OnMouseMove(150.0 + i * 0.1, 120.0 + i * 0.1);
    }
    im.OnLButtonUp(200.0, 170.0);
    
    // Object count must remain strictly 1 (no object cloning/duplication)
    EXPECT_EQ(im.GetObjects().size(), 1ull);
    EXPECT_EQ(page->GetTextObjects().size(), 1ull);
}

// ============================================================================
// CHALLENGE 5: ZOOM / SCROLL COORDINATE STABILITY (0.01x TO 50x) & ROTATIONS
// ============================================================================

TEST(Challenge5_1_ExtremeZoomZeroDriftRoundtrip) {
    EnsureFixtures();
    
    double zooms[] = { 0.01, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0, 25.0, 50.0 };
    int rotations[] = { 0, 90, 180, 270 };
    
    CoordinateConverter::PageContext page{ 612.0, 792.0, 0 };
    
    for (double zoom : zooms) {
        for (int rot : rotations) {
            page.rotation = rot;
            CoordinateConverter::ViewContext view{ zoom, 150.0, 300.0, 50.0, 50.0 };
            
            // Test multiple sample points across the page
            PointF testPoints[] = {
                { 0.0f, 0.0f },
                { 612.0f, 792.0f },
                { 306.0f, 396.0f },
                { 123.456f, 654.321f },
                { 1.0f, 791.0f }
            };
            
            for (const auto& pt : testPoints) {
                PointF screen = CoordinateConverter::PdfToScreen(page, view, pt.x, pt.y);
                PointF roundtrip = CoordinateConverter::ScreenToPdf(page, view, screen.x, screen.y);
                
                double dx = std::abs(roundtrip.x - pt.x);
                double dy = std::abs(roundtrip.y - pt.y);
                
                EXPECT_TRUE(dx < 0.01);
                EXPECT_TRUE(dy < 0.01);
            }
        }
    }
}

TEST(Challenge5_2_RectConversionZeroDriftRoundtrip) {
    EnsureFixtures();
    
    double zooms[] = { 0.01, 0.1, 1.0, 10.0, 50.0 };
    int rotations[] = { 0, 90, 180, 270 };
    
    CoordinateConverter::PageContext page{ 612.0, 792.0, 0 };
    
    for (double zoom : zooms) {
        for (int rot : rotations) {
            page.rotation = rot;
            CoordinateConverter::ViewContext view{ zoom, 500.0, 1000.0, 100.0, 100.0 };
            
            RectF pdfRect{ 100.0f, 200.0f, 350.0f, 500.0f };
            
            RectF screenRect = CoordinateConverter::PdfToScreenRect(page, view, pdfRect.left, pdfRect.top, pdfRect.right, pdfRect.bottom);
            EXPECT_TRUE(screenRect.right > screenRect.left);
            EXPECT_TRUE(screenRect.bottom > screenRect.top);
            
            RectF roundtrip = CoordinateConverter::ScreenToPdfRect(page, view, screenRect.left, screenRect.top, screenRect.right, screenRect.bottom);
            
            EXPECT_TRUE(std::abs(roundtrip.left - pdfRect.left) < 0.05f);
            EXPECT_TRUE(std::abs(roundtrip.right - pdfRect.right) < 0.05f);
        }
    }
}

TEST(Challenge5_3_ExtremeScrollOffsetsZeroDrift) {
    EnsureFixtures();
    
    CoordinateConverter::PageContext page{ 612.0, 792.0, 0 };
    CoordinateConverter::ViewContext view{ 25.0, 500000.0, 1000000.0, 2000.0, 5000.0 };
    
    PointF pt{ 300.0f, 400.0f };
    PointF screen = CoordinateConverter::PdfToScreen(page, view, pt.x, pt.y);
    PointF roundtrip = CoordinateConverter::ScreenToPdf(page, view, screen.x, screen.y);
    
    EXPECT_TRUE(std::abs(roundtrip.x - pt.x) < 0.01);
    EXPECT_TRUE(std::abs(roundtrip.y - pt.y) < 0.01);
}

// Runner Entry Point
int main() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    PdfiumLibrary::Instance().Initialize();
    return TestRunner::Instance().RunAll();
}
