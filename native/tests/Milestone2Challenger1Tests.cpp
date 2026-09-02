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
#include "../src/pdf_engine/src/PdfTextObject.h"
#include "../src/pdf_engine/src/commands/TextCommands.h"
#include "../src/pdf_engine/src/commands/ImageCommands.h"
#include "../src/pdf_engine/src/commands/AnnotationCommands.h"
#include "../src/pdf_engine/src/commands/MacroCommand.h"
#include "../src/ui/src/PdfViewer.h"
#include "../src/ui/src/interaction/InteractionManager.h"
#include "../src/ui/src/interaction/TextSelectableObject.h"
#include "../src/ui/src/interaction/ImageSelectableObject.h"
#include "../src/ui/src/interaction/AnnotationSelectableObject.h"
#include "../src/core/Clipboard.h"
#include "../src/core/interfaces/dom/IDocument.h"
#include "../src/core/interfaces/dom/IPage.h"
#include "../src/core/interfaces/dom/IAnnotation.h"
#include "../src/core/interfaces/dom/IImage.h"

namespace {

static const char kMinimalPdf[] = 
    "%PDF-1.4\n"
    "1 0 obj <</Type/Catalog/Pages 2 0 R>> endobj\n"
    "2 0 obj <</Type/Pages/Count 1/Kids[3 0 R]>> endobj\n"
    "3 0 obj <</Type/Page/Parent 2 0 R/MediaBox[0 0 612 792]/Contents 4 0 R/Resources<<>>>> endobj\n"
    "4 0 obj <</Length 0>> stream\nendstream\nendobj\n"
    "xref\n0 5\n0000000000 65535 f \n0000000009 00000 n \n0000000052 00000 n \n0000000101 00000 n \n0000000193 00000 n \n"
    "trailer <</Size 5/Root 1 0 R>>\nstartxref\n233\n%%EOF";

void EnsureFixtures() {
    std::error_code ec;
    std::filesystem::create_directories("tests/fixtures/basic", ec);
    std::filesystem::create_directories("tests/fixtures/images", ec);
    std::ofstream out("tests/fixtures/basic/minimal.pdf", std::ios::binary);
    out.write(kMinimalPdf, sizeof(kMinimalPdf) - 1);
}

} // namespace

// ============================================================================
// SUITE 1: Context Menu Text Commands (IDM_TEXT_COPY, IDM_TEXT_EDIT, IDM_TEXT_DELETE)
// ============================================================================

TEST(M2Challenger1_ContextMenu_Copy_ExactAndUnicode) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    std::wstring testStr = L"PDF-Elite Challenger 1 \u00A9 2026 \u03A9 \u4E16\u754C";
    RectF bounds = { 100, 700, 400, 600 };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, testStr, bounds, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    PdfViewer viewer;
    viewer.Initialize(nullptr);
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objects = viewer.GetInteractionManager().GetObjects();
    EXPECT_TRUE(!objects.empty());

    std::shared_ptr<ui::interaction::TextSelectableObject> targetObj = nullptr;
    for (auto& obj : objects) {
        if (auto tObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(obj)) {
            if (tObj->GetTextObject()->GetText() == testStr) {
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
    EXPECT_EQ(copied, testStr);
}

TEST(M2Challenger1_ContextMenu_Copy_EdgeCases_NoSelection_MultiSelection) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    // Set known clipboard text first
    core::Clipboard::SetText(nullptr, L"Baseline_Clipboard_Text");

    PdfViewer viewer;
    viewer.Initialize(nullptr);
    viewer.SetDocument(doc);

    // Empty selection
    viewer.GetInteractionManager().GetSelectionModel().Clear();
    viewer.OnCommand(IDM_TEXT_COPY, 0);
    EXPECT_EQ(core::Clipboard::GetText(nullptr), L"Baseline_Clipboard_Text");

    // Add 2 texts and multi-select
    auto add1 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Text A", RectF{50, 700, 150, 650}, "Arial", 12.0f, 0, 0, 0, 255);
    auto add2 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Text B", RectF{50, 600, 150, 550}, "Arial", 12.0f, 0, 0, 0, 255);
    doc->GetCommandStack().ExecuteCommand(std::move(add1));
    doc->GetCommandStack().ExecuteCommand(std::move(add2));

    viewer.ReloadInteractableObjects();
    auto objects = viewer.GetInteractionManager().GetObjects();
    EXPECT_TRUE(objects.size() >= 2);

    for (auto& obj : objects) {
        viewer.GetInteractionManager().GetSelectionModel().AddSelect(obj);
    }
    EXPECT_TRUE(viewer.GetInteractionManager().GetSelection().size() >= 2);

    // OnCommand for text context menu only operates when selection.size() == 1
    viewer.OnCommand(IDM_TEXT_COPY, 0);
    EXPECT_EQ(core::Clipboard::GetText(nullptr), L"Baseline_Clipboard_Text");
}

TEST(M2Challenger1_ContextMenu_Edit_Transition_Typing_Commit_Cancel) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    RectF bounds = { 100, 700, 300, 600 };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Editable Block", bounds, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    PdfViewer viewer;
    viewer.Initialize(nullptr);
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

    // Type character
    viewer.OnChar(L'!');

    // Cancel edit via Escape
    viewer.OnKeyDown(VK_ESCAPE);
    EXPECT_FALSE(viewer.GetInteractionManager().IsEditingText());

    // Re-enter and commit via Return
    viewer.GetInteractionManager().GetSelectionModel().Select(targetObj);
    viewer.OnCommand(IDM_TEXT_EDIT, 0);
    EXPECT_TRUE(viewer.GetInteractionManager().IsEditingText());

    viewer.OnKeyDown(VK_RETURN);
    EXPECT_FALSE(viewer.GetInteractionManager().IsEditingText());
}

TEST(M2Challenger1_ContextMenu_Delete_CommandStack_UndoRedoMultiCycles) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    std::wstring originalText = L"Delete Me Via Menu";
    RectF bounds = { 100, 700, 300, 600 };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, originalText, bounds, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    PdfViewer viewer;
    viewer.Initialize(nullptr);
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

    // Verify deleted
    auto texts = doc->GetPage(0)->GetTextObjects();
    EXPECT_EQ(texts.size(), 0);
    EXPECT_TRUE(doc->GetCommandStack().CanUndo());

    // 5 Cycles of Undo and Redo
    for (int cycle = 0; cycle < 5; ++cycle) {
        EXPECT_TRUE(doc->GetCommandStack().Undo());
        auto restoredTexts = doc->GetPage(0)->GetTextObjects();
        EXPECT_EQ(restoredTexts.size(), 1);
        EXPECT_EQ(restoredTexts[0]->GetText(), originalText);
        EXPECT_TRUE(doc->GetCommandStack().CanRedo());

        EXPECT_TRUE(doc->GetCommandStack().Redo());
        auto reDeletedTexts = doc->GetPage(0)->GetTextObjects();
        EXPECT_EQ(reDeletedTexts.size(), 0);
        EXPECT_TRUE(doc->GetCommandStack().CanUndo());
    }
}

// ============================================================================
// SUITE 2: Canvas Object Deletion via Keyboard VK_DELETE
// ============================================================================

TEST(M2Challenger1_KeyboardDelete_SingleText_ViaOnKeyDown) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    std::wstring textStr = L"Keyboard Delete Target";
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, textStr, RectF{50, 700, 250, 650}, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    PdfViewer viewer;
    viewer.Initialize(nullptr);
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objects = viewer.GetInteractionManager().GetObjects();
    EXPECT_EQ(objects.size(), 1);
    viewer.GetInteractionManager().GetSelectionModel().Select(objects[0]);

    // Dispatch VK_DELETE via viewer.OnKeyDown
    viewer.OnKeyDown(VK_DELETE);

    EXPECT_TRUE(viewer.GetInteractionManager().GetSelection().empty());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 0);
    EXPECT_TRUE(doc->GetCommandStack().CanUndo());

    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 1);
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects()[0]->GetText(), textStr);

    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 0);
}

TEST(M2Challenger1_KeyboardDelete_SingleAnnotation_UndoRedo) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    auto page = doc->GetPage(0);
    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    EXPECT_TRUE(annot != nullptr);
    annot->SetContents("Adversarial Highlight Note");
    annot->SetBounds(RectF{100, 500, 300, 480});

    PdfViewer viewer;
    viewer.Initialize(nullptr);
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objects = viewer.GetInteractionManager().GetObjects();
    EXPECT_TRUE(!objects.empty());

    std::shared_ptr<ui::interaction::AnnotationSelectableObject> annotObj = nullptr;
    for (auto& obj : objects) {
        if (auto aObj = std::dynamic_pointer_cast<ui::interaction::AnnotationSelectableObject>(obj)) {
            annotObj = aObj;
            break;
        }
    }
    EXPECT_TRUE(annotObj != nullptr);

    viewer.GetInteractionManager().GetSelectionModel().Select(annotObj);
    viewer.OnKeyDown(VK_DELETE);

    EXPECT_TRUE(doc->GetPage(0)->GetAnnotations().empty());
    EXPECT_TRUE(doc->GetCommandStack().CanUndo());

    EXPECT_TRUE(doc->GetCommandStack().Undo());
    auto restoredAnnots = doc->GetPage(0)->GetAnnotations();
    EXPECT_EQ(restoredAnnots.size(), 1);
    EXPECT_EQ(restoredAnnots[0]->GetContents(), std::string("Adversarial Highlight Note"));
    EXPECT_EQ(restoredAnnots[0]->GetType(), core::interfaces::dom::AnnotationType::Highlight);

    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_TRUE(doc->GetPage(0)->GetAnnotations().empty());
}

TEST(M2Challenger1_KeyboardDelete_SingleImage_UndoRedo) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    auto page = doc->GetPage(0);
    std::vector<uint8_t> rawBgra(32 * 32 * 4, 180);
    RectF imgBounds = { 50, 400, 150, 300 };
    auto img = page->InsertImageFromMemory(rawBgra, 32, 32, imgBounds);
    EXPECT_TRUE(img != nullptr);

    PdfViewer viewer;
    viewer.Initialize(nullptr);
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objects = viewer.GetInteractionManager().GetObjects();
    EXPECT_TRUE(!objects.empty());

    std::shared_ptr<ui::interaction::ImageSelectableObject> imgObj = nullptr;
    for (auto& obj : objects) {
        if (auto iObj = std::dynamic_pointer_cast<ui::interaction::ImageSelectableObject>(obj)) {
            imgObj = iObj;
            break;
        }
    }
    EXPECT_TRUE(imgObj != nullptr);

    viewer.GetInteractionManager().GetSelectionModel().Select(imgObj);
    viewer.OnKeyDown(VK_DELETE);

    EXPECT_TRUE(doc->GetPage(0)->GetImages().empty());
    EXPECT_TRUE(doc->GetCommandStack().CanUndo());

    EXPECT_TRUE(doc->GetCommandStack().Undo());
    auto restoredImages = doc->GetPage(0)->GetImages();
    EXPECT_EQ(restoredImages.size(), 1);
    EXPECT_EQ(restoredImages[0]->GetWidth(), 32);
    EXPECT_EQ(restoredImages[0]->GetHeight(), 32);

    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_TRUE(doc->GetPage(0)->GetImages().empty());
}

TEST(M2Challenger1_KeyboardDelete_MultiObject_MacroAtomicUndoRedo) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    auto page = doc->GetPage(0);

    // 1. Text 1
    auto add1 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Multi Text 1", RectF{50, 700, 200, 650}, "Arial", 12.0f, 0, 0, 0, 255);
    doc->GetCommandStack().ExecuteCommand(std::move(add1));

    // 2. Text 2
    auto add2 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Multi Text 2", RectF{50, 600, 200, 550}, "Arial", 12.0f, 0, 0, 0, 255);
    doc->GetCommandStack().ExecuteCommand(std::move(add2));

    // 3. Annotation
    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    annot->SetContents("Multi Annot");

    // 4. Image
    std::vector<uint8_t> rawBgra(16 * 16 * 4, 120);
    page->InsertImageFromMemory(rawBgra, 16, 16, RectF{50, 400, 100, 350});

    PdfViewer viewer;
    viewer.Initialize(nullptr);
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objects = viewer.GetInteractionManager().GetObjects();
    EXPECT_EQ(objects.size(), 4);

    for (auto& obj : objects) {
        viewer.GetInteractionManager().GetSelectionModel().AddSelect(obj);
    }
    EXPECT_EQ(viewer.GetInteractionManager().GetSelection().size(), 4);

    viewer.OnKeyDown(VK_DELETE);

    // All objects deleted
    EXPECT_TRUE(viewer.GetInteractionManager().GetSelection().empty());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 0);
    EXPECT_EQ(doc->GetPage(0)->GetAnnotations().size(), 0);
    EXPECT_EQ(doc->GetPage(0)->GetImages().size(), 0);
    EXPECT_TRUE(doc->GetCommandStack().CanUndo());

    // 3 Atomic Cycles of Undo/Redo
    for (int cycle = 0; cycle < 3; ++cycle) {
        EXPECT_TRUE(doc->GetCommandStack().Undo());
        EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 2);
        EXPECT_EQ(doc->GetPage(0)->GetAnnotations().size(), 1);
        EXPECT_EQ(doc->GetPage(0)->GetImages().size(), 1);

        EXPECT_TRUE(doc->GetCommandStack().Redo());
        EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 0);
        EXPECT_EQ(doc->GetPage(0)->GetAnnotations().size(), 0);
        EXPECT_EQ(doc->GetPage(0)->GetImages().size(), 0);
    }
}

TEST(M2Challenger1_KeyboardDelete_EmptySelection_NoOp) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    PdfViewer viewer;
    viewer.Initialize(nullptr);
    viewer.SetDocument(doc);
    viewer.GetInteractionManager().GetSelectionModel().Clear();

    viewer.OnKeyDown(VK_DELETE);
    EXPECT_FALSE(doc->GetCommandStack().CanUndo());
}

TEST(M2Challenger1_KeyboardDelete_InsideTextEditor_CharacterLevelOnly) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"ABCDEF", RectF{50, 700, 200, 650}, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    PdfViewer viewer;
    viewer.Initialize(nullptr);
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objects = viewer.GetInteractionManager().GetObjects();
    EXPECT_EQ(objects.size(), 1);

    auto textObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(objects[0]);
    EXPECT_TRUE(textObj != nullptr);

    viewer.GetInteractionManager().GetSelectionModel().Select(textObj);
    viewer.OnCommand(IDM_TEXT_EDIT, 0);
    EXPECT_TRUE(viewer.GetInteractionManager().IsEditingText());

    // Press VK_DELETE inside text editing mode
    viewer.OnKeyDown(VK_DELETE);

    // Text object in DOM must NOT be removed by in-editor delete!
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 1);

    viewer.OnKeyDown(VK_ESCAPE);
    EXPECT_FALSE(viewer.GetInteractionManager().IsEditingText());
}

// ============================================================================
// SUITE 3: Undo/Redo Semantics & CommandStack Boundary Stress
// ============================================================================

TEST(M2Challenger1_CommandStack_EmptyStack_Safety) {
    core::interfaces::dom::CommandStack stack;
    EXPECT_FALSE(stack.CanUndo());
    EXPECT_FALSE(stack.CanRedo());
    EXPECT_FALSE(stack.Undo());
    EXPECT_FALSE(stack.Redo());
}

TEST(M2Challenger1_CommandStack_BranchingHistory_ClearsRedo) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    auto add1 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Branch A", RectF{50, 700, 200, 650}, "Arial", 12.0f, 0, 0, 0, 255);
    auto add2 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Branch B", RectF{50, 600, 200, 550}, "Arial", 12.0f, 0, 0, 0, 255);

    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(add1)));
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(add2)));
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 2);

    // Undo 1 step
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 1);
    EXPECT_TRUE(doc->GetCommandStack().CanRedo());

    // Execute new command -> Redo stack must be cleared
    auto add3 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Branch C", RectF{50, 500, 200, 450}, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(add3)));

    EXPECT_FALSE(doc->GetCommandStack().CanRedo());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 2);
}

TEST(M2Challenger1_SequentialDeletions_ChainedUndoRedo) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);
    auto page = doc->GetPage(0);

    auto add1 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Sequential 1", RectF{50, 700, 200, 650}, "Arial", 12.0f, 0, 0, 0, 255);
    auto add2 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Sequential 2", RectF{50, 600, 200, 550}, "Arial", 12.0f, 0, 0, 0, 255);
    auto add3 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Sequential 3", RectF{50, 500, 200, 450}, "Arial", 12.0f, 0, 0, 0, 255);

    doc->GetCommandStack().ExecuteCommand(std::move(add1));
    doc->GetCommandStack().ExecuteCommand(std::move(add2));
    doc->GetCommandStack().ExecuteCommand(std::move(add3));

    PdfViewer viewer;
    viewer.Initialize(nullptr);
    viewer.SetDocument(doc);
    viewer.ReloadInteractableObjects();

    auto objs = viewer.GetInteractionManager().GetObjects();
    EXPECT_EQ(objs.size(), 3);

    // Delete first
    viewer.GetInteractionManager().GetSelectionModel().Select(objs[0]);
    viewer.OnKeyDown(VK_DELETE);
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 2);

    // Delete second
    viewer.ReloadInteractableObjects();
    objs = viewer.GetInteractionManager().GetObjects();
    EXPECT_EQ(objs.size(), 2);
    viewer.GetInteractionManager().GetSelectionModel().Select(objs[0]);
    viewer.OnKeyDown(VK_DELETE);
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 1);

    // Delete third
    viewer.ReloadInteractableObjects();
    objs = viewer.GetInteractionManager().GetObjects();
    EXPECT_EQ(objs.size(), 1);
    viewer.GetInteractionManager().GetSelectionModel().Select(objs[0]);
    viewer.OnKeyDown(VK_DELETE);
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 0);

    // Sequential Undo (reverse order)
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 1);
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 2);
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 3);

    // Sequential Redo (forward order)
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 2);
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 1);
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 0);
}

TEST(M2Challenger1_PdfViewer_NullDoc_SafeHandling) {
    PdfViewer viewer;
    viewer.Initialize(nullptr);

    // Unset doc - all operations must safely no-op without crashes
    viewer.OnCommand(IDM_TEXT_COPY, 0);
    viewer.OnCommand(IDM_TEXT_EDIT, 0);
    viewer.OnCommand(IDM_TEXT_DELETE, 0);
    viewer.OnKeyDown(VK_DELETE);
    viewer.ReloadInteractableObjects();
    viewer.InvalidateView();
}

TEST(M2Challenger1_Clipboard_StressAndRetries) {
    // Test various buffer sizes
    std::vector<size_t> sizes = { 1, 64, 512, 4096, 32768 };
    for (size_t sz : sizes) {
        std::wstring s(sz, L'X');
        s[0] = L'\u00C9'; // Unicode É
        s[sz / 2] = L'\u4E16'; // CJK
        s[sz - 1] = L'\u03A9'; // Omega

        EXPECT_TRUE(core::Clipboard::SetText(nullptr, s));
        std::wstring retrieved = core::Clipboard::GetText(nullptr);
        EXPECT_EQ(retrieved.size(), s.size());
        EXPECT_EQ(retrieved, s);
    }
}

TEST(Repro_DeleteImageCommand_FailsDueToUnpopulatedPageImages) {
    EnsureFixtures();
    auto docRes = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(docRes.has_value());
    std::shared_ptr<core::interfaces::dom::IDocument> doc = std::move(docRes.value);

    // Keep page alive
    auto page = doc->GetPage(0);
    std::vector<uint8_t> rawBgra(32 * 32 * 4, 180);
    RectF imgBounds = { 50, 400, 150, 300 };
    auto img = page->InsertImageFromMemory(rawBgra, 32, 32, imgBounds);
    EXPECT_TRUE(img != nullptr);

    // Execute DeleteImageCommand directly
    auto delCmd = std::make_unique<pdf_engine::commands::DeleteImageCommand>(doc.get(), 0, img);
    bool executed = doc->GetCommandStack().ExecuteCommand(std::move(delCmd));
    
    std::cout << "[EMPIRICAL EVIDENCE] DeleteImageCommand::Execute returned " 
              << (executed ? "true" : "false") << "\n";
    // If executed is true, this empirically confirms the defect is resolved
    EXPECT_TRUE(executed);
}


int main() {
    PdfiumLibrary::Instance().Initialize();
    std::cout << "====================================================\n";
    std::cout << " Milestone 2 Challenger 1 Empirical Test Suite\n";
    std::cout << "====================================================\n\n";

    int result = TestRunner::Instance().RunAll();
    return result;
}
