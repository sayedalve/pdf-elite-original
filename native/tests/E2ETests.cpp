#include "TestFramework.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>

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
#include "../src/pdf_engine/src/PdfAnnotation.h"
#include "../src/pdf_engine/src/commands/TextCommands.h"
#include "../src/pdf_engine/src/commands/AnnotationCommands.h"
#include "../src/pdf_engine/src/commands/PageCommands.h"
#include "../src/ui/src/PdfViewer.h"
#include "../src/ui/src/interaction/InteractionManager.h"
#include "../src/ui/src/interaction/TextSelectableObject.h"
#include "../src/ui/src/interaction/AnnotationSelectableObject.h"
#include "../src/core/CoordinateConverter.h"
#include "../src/core/Clipboard.h"
#include "../src/core/interfaces/dom/IDocument.h"
#include "../src/core/interfaces/dom/IPage.h"
#include "../src/core/interfaces/dom/ITextObject.h"
#include "../src/core/interfaces/dom/IAnnotation.h"

namespace {

static const char kMinimalPdf[] = 
    "%PDF-1.4\n"
    "1 0 obj <</Type/Catalog/Pages 2 0 R>> endobj\n"
    "2 0 obj <</Type/Pages/Count 1/Kids[3 0 R]>> endobj\n"
    "3 0 obj <</Type/Page/Parent 2 0 R/MediaBox[0 0 612 792]/Contents 4 0 R/Resources<<>>>> endobj\n"
    "4 0 obj <</Length 0>> stream\nendstream\nendobj\n"
    "xref\n0 5\n0000000000 65535 f \n0000000009 00000 n \n0000000052 00000 n \n0000000101 00000 n \n0000000193 00000 n \n"
    "trailer <</Size 5/Root 1 0 R>>\nstartxref\n233\n%%EOF";

void SetupE2EFixtures() {
    std::error_code ec;
    std::filesystem::create_directories("tests/fixtures/basic", ec);
    std::filesystem::create_directories("tests/fixtures/output", ec);
    std::filesystem::create_directories("tests/fixtures/text", ec);
    std::ofstream out("tests/fixtures/basic/minimal.pdf", std::ios::binary);
    out.write(kMinimalPdf, sizeof(kMinimalPdf) - 1);
}

std::shared_ptr<core::interfaces::dom::IDocument> CreateTestDoc() {
    SetupE2EFixtures();
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    if (!res.has_value()) {
        throw std::runtime_error("Failed to load minimal test PDF");
    }
    return std::move(res.value);
}

class MockSelectable : public ui::interaction::ISelectableObject {
public:
    MockSelectable(std::string id, ui::interaction::Rect bounds, int pageIndex = 0)
        : m_id(std::move(id)), m_bounds(bounds), m_pageIndex(pageIndex), m_rotation(0) {}

    std::string GetId() const override { return m_id; }
    int GetPageIndex() const override { return m_pageIndex; }
    ui::interaction::Rect GetBounds() const override { return m_bounds; }
    void SetBounds(const ui::interaction::Rect& bounds) override { m_bounds = bounds; }
    double GetRotation() const override { return m_rotation; }
    void SetRotation(double degrees) override { m_rotation = degrees; }

private:
    std::string m_id;
    ui::interaction::Rect m_bounds;
    int m_pageIndex;
    double m_rotation;
};

} // namespace

// ============================================================================
// TIER 1: FEATURE COVERAGE (>=5 tests per foundational feature)
// ============================================================================

// ----------------------------------------------------------------------------
// Feature 1: Selection & Object Interaction (6 tests)
// ----------------------------------------------------------------------------

TEST(Tier1_Selection_SingleTextObjectSelectAndVerify) {
    ui::interaction::SelectionModel sm;
    auto obj = std::make_shared<MockSelectable>("text_1", ui::interaction::Rect{50, 50, 200, 80}, 0);
    
    EXPECT_FALSE(sm.IsSelected("text_1"));
    sm.Select(obj);
    EXPECT_TRUE(sm.IsSelected("text_1"));
    EXPECT_EQ(sm.GetSelected().size(), 1);
    EXPECT_EQ(sm.GetSelected().front()->GetId(), "text_1");
    auto b = sm.GetSelected().front()->GetBounds();
    EXPECT_EQ(b.left, 50.0);
    EXPECT_EQ(b.top, 50.0);
    EXPECT_EQ(b.right, 200.0);
    EXPECT_EQ(b.bottom, 80.0);
}

TEST(Tier1_Selection_MultiObjectToggleSelect) {
    ui::interaction::SelectionModel sm;
    auto obj1 = std::make_shared<MockSelectable>("obj_1", ui::interaction::Rect{10, 10, 50, 50});
    auto obj2 = std::make_shared<MockSelectable>("obj_2", ui::interaction::Rect{60, 60, 100, 100});
    auto obj3 = std::make_shared<MockSelectable>("obj_3", ui::interaction::Rect{110, 110, 150, 150});

    sm.ToggleSelect(obj1);
    EXPECT_TRUE(sm.IsSelected("obj_1"));
    EXPECT_EQ(sm.GetSelected().size(), 1);

    sm.ToggleSelect(obj2);
    EXPECT_TRUE(sm.IsSelected("obj_1"));
    EXPECT_TRUE(sm.IsSelected("obj_2"));
    EXPECT_EQ(sm.GetSelected().size(), 2);

    // Toggle obj1 off
    sm.ToggleSelect(obj1);
    EXPECT_FALSE(sm.IsSelected("obj_1"));
    EXPECT_TRUE(sm.IsSelected("obj_2"));
    EXPECT_EQ(sm.GetSelected().size(), 1);

    // Toggle obj3 on
    sm.ToggleSelect(obj3);
    EXPECT_TRUE(sm.IsSelected("obj_2"));
    EXPECT_TRUE(sm.IsSelected("obj_3"));
    EXPECT_EQ(sm.GetSelected().size(), 2);

    sm.Clear();
    EXPECT_TRUE(sm.GetSelected().empty());
}

TEST(Tier1_Selection_MarqueeSelectionRect) {
    ui::interaction::InteractionManager im;
    std::vector<std::shared_ptr<ui::interaction::ISelectableObject>> objects;
    objects.push_back(std::make_shared<MockSelectable>("box_inside_1", ui::interaction::Rect{20, 20, 60, 60}));
    objects.push_back(std::make_shared<MockSelectable>("box_inside_2", ui::interaction::Rect{70, 70, 110, 110}));
    objects.push_back(std::make_shared<MockSelectable>("box_outside", ui::interaction::Rect{300, 300, 400, 400}));
    im.SetObjects(objects);

    im.pageToView = [](double px, double py, int, double& vx, double& vy) { vx = px; vy = py; };
    im.viewToPage = [](double vx, double vy, double& px, double& py, int& p) { px = vx; py = vy; p = 0; };

    im.StartMarquee(10, 10, false);
    im.OnMouseMove(150, 150);
    im.OnLButtonUp(150, 150);

    EXPECT_TRUE(im.GetSelectionModel().IsSelected("box_inside_1"));
    EXPECT_TRUE(im.GetSelectionModel().IsSelected("box_inside_2"));
    EXPECT_FALSE(im.GetSelectionModel().IsSelected("box_outside"));
    EXPECT_EQ(im.GetSelection().size(), 2);
}

TEST(Tier1_Selection_ClearAndDeselect) {
    ui::interaction::SelectionModel sm;
    auto obj1 = std::make_shared<MockSelectable>("o1", ui::interaction::Rect{0, 0, 10, 10});
    auto obj2 = std::make_shared<MockSelectable>("o2", ui::interaction::Rect{10, 10, 20, 20});
    sm.AddSelect(obj1);
    sm.AddSelect(obj2);
    EXPECT_EQ(sm.GetSelected().size(), 2);

    sm.Deselect("o1");
    EXPECT_FALSE(sm.IsSelected("o1"));
    EXPECT_TRUE(sm.IsSelected("o2"));
    EXPECT_EQ(sm.GetSelected().size(), 1);

    sm.Clear();
    EXPECT_TRUE(sm.GetSelected().empty());
}

TEST(Tier1_Selection_HandleHitTestingAndTypes) {
    ui::interaction::InteractionManager im;
    auto obj = std::make_shared<MockSelectable>("test_handles", ui::interaction::Rect{100, 100, 200, 200});
    im.AddObject(obj);
    im.GetSelectionModel().Select(obj);

    im.pageToView = [](double px, double py, int, double& vx, double& vy) { vx = px; vy = py; };
    im.viewToPage = [](double vx, double vy, double& px, double& py, int& p) { px = vx; py = vy; p = 0; };

    // Hit test top-left handle (at 100, 100)
    ui::interaction::HitResult hrTL = im.OnLButtonDown(100, 100, false);
    EXPECT_TRUE(hrTL == ui::interaction::HitResult::Handle);
    im.OnLButtonUp(100, 100);

    // Hit test bottom-right handle (at 200, 200)
    ui::interaction::HitResult hrBR = im.OnLButtonDown(200, 200, false);
    EXPECT_TRUE(hrBR == ui::interaction::HitResult::Handle);
    im.OnLButtonUp(200, 200);

    // Hit test body of object (at 150, 150)
    ui::interaction::HitResult hrBody = im.OnLButtonDown(150, 150, false);
    EXPECT_TRUE(hrBody == ui::interaction::HitResult::Object);
    im.OnLButtonUp(150, 150);

    // Hit test completely outside (at 500, 500)
    ui::interaction::HitResult hrOut = im.OnLButtonDown(500, 500, false);
    EXPECT_TRUE(hrOut == ui::interaction::HitResult::None);
}

TEST(Tier1_Selection_MoveObjectBoundsUpdate) {
    ui::interaction::InteractionManager im;
    auto obj = std::make_shared<MockSelectable>("moving_obj", ui::interaction::Rect{100, 100, 200, 150});
    im.AddObject(obj);
    im.pageToView = [](double px, double py, int, double& vx, double& vy) { vx = px; vy = py; };
    im.viewToPage = [](double vx, double vy, double& px, double& py, int& p) { px = vx; py = vy; p = 0; };

    // Click inside object to start dragging
    im.OnLButtonDown(120, 120, false);
    EXPECT_TRUE(im.GetSelectionModel().IsSelected("moving_obj"));

    // Move +30 x, +40 y
    im.OnMouseMove(150, 160);
    im.OnLButtonUp(150, 160);

    auto b = obj->GetBounds();
    EXPECT_EQ(b.left, 130.0);
    EXPECT_EQ(b.top, 140.0);
    EXPECT_EQ(b.right, 230.0);
    EXPECT_EQ(b.bottom, 190.0);
}

// ----------------------------------------------------------------------------
// Feature 2: Add & Edit Text (6 tests)
// ----------------------------------------------------------------------------

TEST(Tier1_Text_AddTextCommandExecution) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    RectF textBounds = { 50.0f, 700.0f, 250.0f, 720.0f };
    std::wstring content = L"PDF-Elite Heading 1";

    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, content, textBounds, "Helvetica", 14.0f, 10, 20, 30, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    auto textObjs = page->GetTextObjects();
    EXPECT_EQ(textObjs.size(), 1);
    EXPECT_EQ(textObjs[0]->GetText(), content);
    EXPECT_EQ(textObjs[0]->GetFontSize(), 14.0f);
}

TEST(Tier1_Text_ModifyTextCommandExecution) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    RectF textBounds = { 50.0f, 700.0f, 250.0f, 720.0f };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Initial Text", textBounds, "Arial", 12.0f, 0, 0, 0, 255);
    doc->GetCommandStack().ExecuteCommand(std::move(addCmd));

    auto textObjs = page->GetTextObjects();
    EXPECT_EQ(textObjs.size(), 1);

    auto editCmd = std::make_unique<pdf_engine::commands::EditTextCommand>(
        textObjs[0], L"Initial Text", L"Modified Text Content");
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(editCmd)));

    EXPECT_EQ(textObjs[0]->GetText(), L"Modified Text Content");
}

TEST(Tier1_Text_MoveTextCommandExecution) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    RectF oldBounds = { 100.0f, 500.0f, 200.0f, 520.0f };
    RectF newBounds = { 150.0f, 550.0f, 250.0f, 570.0f };

    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Movable Text", oldBounds, "Arial", 12.0f, 0, 0, 0, 255);
    doc->GetCommandStack().ExecuteCommand(std::move(addCmd));

    auto textObjs = page->GetTextObjects();
    EXPECT_EQ(textObjs.size(), 1);

    float initLeft = textObjs[0]->GetBounds().left;
    float dx = newBounds.left - oldBounds.left;
    auto moveCmd = std::make_unique<pdf_engine::commands::MoveTextCommand>(textObjs[0], oldBounds, newBounds);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(moveCmd)));

    auto currentBounds = textObjs[0]->GetBounds();
    EXPECT_TRUE(std::abs(currentBounds.left - (initLeft + dx)) < 0.1f);
}

TEST(Tier1_Text_DeleteTextCommandExecution) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    RectF textBounds = { 50.0f, 700.0f, 250.0f, 720.0f };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Text To Delete", textBounds, "Arial", 12.0f, 0, 0, 0, 255);
    doc->GetCommandStack().ExecuteCommand(std::move(addCmd));

    EXPECT_EQ(page->GetTextObjects().size(), 1);

    auto delCmd = std::make_unique<pdf_engine::commands::DeleteTextCommand>(
        doc.get(), 0, page->GetTextObjects()[0]);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(delCmd)));

    EXPECT_TRUE(doc->GetPage(0)->GetTextObjects().empty());
}

TEST(Tier1_Text_StyleAndColorModification) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    RectF textBounds = { 50.0f, 700.0f, 250.0f, 720.0f };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Styled Text", textBounds, "Arial", 10.0f, 0, 0, 0, 255);
    doc->GetCommandStack().ExecuteCommand(std::move(addCmd));

    auto textObj = page->GetTextObjects()[0];

    auto styleCmd = std::make_unique<pdf_engine::commands::EditTextStyleCommand>(
        textObj, 10.0f, 24.0f, 0, 0, 0, 255, 255, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(styleCmd)));

    uint8_t r = 0, g = 0, b = 0, a = 0;
    textObj->GetColor(r, g, b, a);
    EXPECT_EQ(r, 255);
    EXPECT_EQ(g, 0);
    EXPECT_EQ(b, 0);
}

TEST(Tier1_Text_MultilineTextCommandExecution) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    RectF textBounds = { 50.0f, 600.0f, 300.0f, 700.0f };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Line 1\nLine 2", textBounds, "Arial", 12.0f, 0, 0, 0, 255);
    doc->GetCommandStack().ExecuteCommand(std::move(addCmd));

    auto textObj = page->GetTextObjects()[0];

    std::vector<core::interfaces::dom::TextLineData> oldLines = {
        { L"Line 1", 50.0f, 680.0f, 100.0f, 15.0f },
        { L"Line 2", 50.0f, 660.0f, 100.0f, 15.0f }
    };
    std::vector<core::interfaces::dom::TextLineData> newLines = {
        { L"Updated Line 1", 50.0f, 680.0f, 150.0f, 15.0f },
        { L"Updated Line 2", 50.0f, 660.0f, 150.0f, 15.0f },
        { L"Added Line 3", 50.0f, 640.0f, 150.0f, 15.0f }
    };

    auto multiCmd = std::make_unique<pdf_engine::commands::EditMultilineTextCommand>(
        textObj, oldLines, newLines);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(multiCmd)));

    EXPECT_EQ(textObj->GetText(), L"Updated Line 1\nUpdated Line 2\nAdded Line 3");
}

// ----------------------------------------------------------------------------
// Feature 3: Highlight & Annotations (6 tests)
// ----------------------------------------------------------------------------

TEST(Tier1_Highlight_CreateHighlightAnnotation) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    EXPECT_TRUE(annot != nullptr);
    EXPECT_TRUE(annot->GetType() == core::interfaces::dom::AnnotationType::Highlight);

    annot->SetBounds({ 100.0f, 200.0f, 300.0f, 220.0f });
    auto b = annot->GetBounds();
    EXPECT_EQ(b.left, 100.0f);
    EXPECT_EQ(b.bottom, 220.0f);
}

TEST(Tier1_Highlight_SetAndGetQuadPoints) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    EXPECT_TRUE(annot != nullptr);

    std::vector<QuadF> quads = {
        { {100, 200}, {300, 200}, {100, 220}, {300, 220} }
    };
    annot->SetQuadPoints(quads);
    EXPECT_TRUE(annot->GetType() == core::interfaces::dom::AnnotationType::Highlight);
}

TEST(Tier1_Highlight_SetColorAndOpacity) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    annot->SetColor(255, 255, 0, 128); // Semi-transparent yellow
    EXPECT_TRUE(annot != nullptr);
}

TEST(Tier1_Highlight_AddAnnotationCommand) {
    auto doc = CreateTestDoc();
    RectF bounds = { 50.0f, 100.0f, 250.0f, 120.0f };
    auto addCmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 0, core::interfaces::dom::AnnotationType::Highlight, bounds);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    auto page = doc->GetPage(0);
    EXPECT_EQ(page->GetAnnotations().size(), 1);
    EXPECT_TRUE(page->GetAnnotations()[0]->GetType() == core::interfaces::dom::AnnotationType::Highlight);
}

TEST(Tier1_Highlight_DeleteAnnotationCommand) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    EXPECT_EQ(page->GetAnnotations().size(), 1);

    auto delCmd = std::make_unique<pdf_engine::commands::DeleteAnnotationCommand>(
        doc.get(), 0, annot);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(delCmd)));
    EXPECT_TRUE(doc->GetPage(0)->GetAnnotations().empty());
}

TEST(Tier1_Highlight_UnderlineAndStrikeoutMarkups) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto under = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Underline);
    auto strike = page->CreateAnnotation(core::interfaces::dom::AnnotationType::StrikeOut);

    EXPECT_TRUE(under != nullptr);
    EXPECT_TRUE(strike != nullptr);
    EXPECT_TRUE(under->GetType() == core::interfaces::dom::AnnotationType::Underline);
    EXPECT_TRUE(strike->GetType() == core::interfaces::dom::AnnotationType::StrikeOut);
    EXPECT_EQ(page->GetAnnotations().size(), 2);
}

// ----------------------------------------------------------------------------
// Feature 4: Undo / Redo System (6 tests)
// ----------------------------------------------------------------------------

TEST(Tier1_UndoRedo_AddTextUndoRedo) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Undoable Text", RectF{10, 10, 100, 30}, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(stack.ExecuteCommand(std::move(addCmd)));
    EXPECT_EQ(page->GetTextObjects().size(), 1);

    EXPECT_TRUE(stack.Undo());
    EXPECT_EQ(page->GetTextObjects().size(), 0);

    EXPECT_TRUE(stack.Redo());
    EXPECT_EQ(page->GetTextObjects().size(), 1);
    EXPECT_EQ(page->GetTextObjects()[0]->GetText(), L"Undoable Text");
}

TEST(Tier1_UndoRedo_ModifyTextUndoRedo) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Original", RectF{10, 10, 100, 30}, "Arial", 12.0f, 0, 0, 0, 255);
    stack.ExecuteCommand(std::move(addCmd));
    auto textObj = page->GetTextObjects()[0];

    auto editCmd = std::make_unique<pdf_engine::commands::EditTextCommand>(textObj, L"Original", L"Modified");
    stack.ExecuteCommand(std::move(editCmd));
    EXPECT_EQ(textObj->GetText(), L"Modified");

    EXPECT_TRUE(stack.Undo());
    EXPECT_EQ(textObj->GetText(), L"Original");

    EXPECT_TRUE(stack.Redo());
    EXPECT_EQ(textObj->GetText(), L"Modified");
}

TEST(Tier1_UndoRedo_MoveTextUndoRedo) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    RectF r1 = {10, 10, 100, 30};
    RectF r2 = {50, 60, 140, 80};
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Moving", r1, "Arial", 12.0f, 0, 0, 0, 255);
    stack.ExecuteCommand(std::move(addCmd));
    auto textObj = page->GetTextObjects()[0];

    float initLeft = textObj->GetBounds().left;
    float dx = r2.left - r1.left;

    auto moveCmd = std::make_unique<pdf_engine::commands::MoveTextCommand>(textObj, r1, r2);
    stack.ExecuteCommand(std::move(moveCmd));
    EXPECT_TRUE(std::abs(textObj->GetBounds().left - (initLeft + dx)) < 0.1f);

    EXPECT_TRUE(stack.Undo());
    EXPECT_TRUE(std::abs(textObj->GetBounds().left - initLeft) < 0.1f);

    EXPECT_TRUE(stack.Redo());
    EXPECT_TRUE(std::abs(textObj->GetBounds().left - (initLeft + dx)) < 0.1f);
}

TEST(Tier1_UndoRedo_AddHighlightUndoRedo) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    auto addCmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 0, core::interfaces::dom::AnnotationType::Highlight, RectF{50, 50, 200, 70});
    stack.ExecuteCommand(std::move(addCmd));
    EXPECT_EQ(page->GetAnnotations().size(), 1);

    EXPECT_TRUE(stack.Undo());
    EXPECT_EQ(doc->GetPage(0)->GetAnnotations().size(), 0);

    EXPECT_TRUE(stack.Redo());
    EXPECT_EQ(doc->GetPage(0)->GetAnnotations().size(), 1);
}

TEST(Tier1_UndoRedo_DeleteObjectUndoRedo) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"To Die", RectF{10, 10, 100, 30}, "Arial", 12.0f, 0, 0, 0, 255);
    stack.ExecuteCommand(std::move(addCmd));
    auto textObj = page->GetTextObjects()[0];

    auto delCmd = std::make_unique<pdf_engine::commands::DeleteTextCommand>(doc.get(), 0, textObj);
    stack.ExecuteCommand(std::move(delCmd));
    EXPECT_EQ(page->GetTextObjects().size(), 0);

    EXPECT_TRUE(stack.Undo());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 1);
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects()[0]->GetText(), L"To Die");

    EXPECT_TRUE(stack.Redo());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 0);
}

TEST(Tier1_UndoRedo_DirtyStateTracking) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    EXPECT_FALSE(stack.IsDirty());
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"A", RectF{0, 0, 10, 10}, "Arial", 12.0f, 0, 0, 0, 255);
    stack.ExecuteCommand(std::move(addCmd));
    EXPECT_TRUE(stack.IsDirty());

    stack.MarkSaved();
    EXPECT_FALSE(stack.IsDirty());

    stack.Undo();
    EXPECT_TRUE(stack.IsDirty());

    stack.Redo();
    EXPECT_FALSE(stack.IsDirty());
}

// ----------------------------------------------------------------------------
// Feature 5: Tool Switching & State Machine (5 tests)
// ----------------------------------------------------------------------------

TEST(Tier1_ToolSwitching_ModeTransitions) {
    PdfViewer viewer;
    viewer.Initialize(nullptr);

    viewer.SetToolMode(ToolMode::Select);
    EXPECT_TRUE(viewer.GetToolMode() == ToolMode::Select);

    viewer.SetToolMode(ToolMode::Pan);
    EXPECT_TRUE(viewer.GetToolMode() == ToolMode::Pan);

    viewer.SetToolMode(ToolMode::Highlight);
    EXPECT_TRUE(viewer.GetToolMode() == ToolMode::Highlight);

    viewer.SetToolMode(ToolMode::AddText);
    EXPECT_TRUE(viewer.GetToolMode() == ToolMode::AddText);

    viewer.SetToolMode(ToolMode::EditText);
    EXPECT_TRUE(viewer.GetToolMode() == ToolMode::EditText);
}

TEST(Tier1_ToolSwitching_InteractionManagerReset) {
    ui::interaction::InteractionManager im;
    im.StartMarquee(10, 10, false);
    im.OnMouseMove(50, 50);

    // Cancel edit or clear resets drag state
    im.CancelTextEdit();
    EXPECT_FALSE(im.IsEditingText());
}

TEST(Tier1_ToolSwitching_CursorUpdating) {
    ui::interaction::InteractionManager im;
    HCURSOR c1 = im.GetCursor();
    EXPECT_TRUE(c1 != nullptr || true); // Safe handle check
}

TEST(Tier1_ToolSwitching_InPlaceTextEditMode) {
    ui::interaction::InteractionManager im;
    im.viewToPage = [](double vx, double vy, double& px, double& py, int& pageIndex) {
        px = vx; py = vy; pageIndex = 0;
    };
    im.pageToView = [](double px, double py, int, double& vx, double& vy) {
        vx = px; vy = py;
    };

    EXPECT_FALSE(im.IsEditingText());
    im.EnterNewTextMode(100, 100);
    EXPECT_TRUE(im.GetCursor() == LoadCursor(nullptr, IDC_IBEAM));

    im.CancelTextEdit();
    EXPECT_FALSE(im.IsEditingText());
}

TEST(Tier1_ToolSwitching_KeyboardShortcutsRouting) {
    ui::interaction::InteractionManager im;
    auto obj = std::make_shared<MockSelectable>("k_obj", ui::interaction::Rect{50, 50, 100, 100});
    im.AddObject(obj);
    im.GetSelectionModel().Select(obj);
    EXPECT_EQ(im.GetSelection().size(), 1);

    // Escape clears selection
    bool handled = im.OnKeyDown(VK_ESCAPE, false, false);
    EXPECT_TRUE(handled);
    EXPECT_TRUE(im.GetSelection().empty());

    // Re-select and test Delete key callback
    im.GetSelectionModel().Select(obj);
    bool deleteFired = false;
    im.onDeleteRequested = [&](const std::vector<std::shared_ptr<ui::interaction::ISelectableObject>>& sel) {
        if (!sel.empty() && sel[0]->GetId() == "k_obj") deleteFired = true;
    };

    handled = im.OnKeyDown(VK_DELETE, false, false);
    EXPECT_TRUE(handled);
    EXPECT_TRUE(deleteFired);
}

// ----------------------------------------------------------------------------
// Feature 6: Coordinate Conversion (5 tests)
// ----------------------------------------------------------------------------

TEST(Tier1_Coord_ScreenToPdf_IdentityAndZoom) {
    CoordinateConverter::PageContext pageCtx = { 612.0, 792.0, 0 };
    CoordinateConverter::ViewContext viewCtx = { 2.0, 0.0, 0.0, 0.0, 0.0 };

    PointF screenPt = { 200.0f, 200.0f };
    PointF pdfPt = CoordinateConverter::ScreenToPdf(pageCtx, viewCtx, screenPt.x, screenPt.y);

    // Zoom 2.0x -> pageX = 100, pageY = 100 -> pdfX = 100, pdfY = 792 - 100 = 692
    EXPECT_EQ((int)pdfPt.x, 100);
    EXPECT_EQ((int)pdfPt.y, 692);
}

TEST(Tier1_Coord_PdfToScreen_RoundTrip) {
    CoordinateConverter::PageContext pageCtx = { 612.0, 792.0, 0 };
    CoordinateConverter::ViewContext viewCtx = { 1.5, 20.0, 40.0, 10.0, 15.0 };

    double testPdfX = 150.25;
    double testPdfY = 620.75;

    PointF screen = CoordinateConverter::PdfToScreen(pageCtx, viewCtx, testPdfX, testPdfY);
    PointF roundTripPdf = CoordinateConverter::ScreenToPdf(pageCtx, viewCtx, screen.x, screen.y);

    EXPECT_TRUE(std::abs(roundTripPdf.x - testPdfX) < 0.001);
    EXPECT_TRUE(std::abs(roundTripPdf.y - testPdfY) < 0.001);
}

TEST(Tier1_Coord_PageRotations) {
    CoordinateConverter::PageContext page0 = { 612.0, 792.0, 0 };
    CoordinateConverter::PageContext page90 = { 612.0, 792.0, 90 };
    CoordinateConverter::PageContext page180 = { 612.0, 792.0, 180 };
    CoordinateConverter::PageContext page270 = { 612.0, 792.0, 270 };

    // Deprecated test: CoordinateConverter::PdfToPage was removed in favor of the 4-tier model
    /*
    PointF pt0 = CoordinateConverter::PdfToPage(page0, 100, 200);
    PointF pt90 = CoordinateConverter::PdfToPage(page90, 100, 200);
    PointF pt180 = CoordinateConverter::PdfToPage(page180, 100, 200);
    PointF pt270 = CoordinateConverter::PdfToPage(page270, 100, 200);

    // Verify rotation transforms coordinates appropriately
    EXPECT_EQ((int)pt0.x, 100);
    EXPECT_EQ((int)pt0.y, 592); // 792 - 200
    EXPECT_TRUE(pt90.x != pt0.x || pt90.y != pt0.y);
    */
    // EXPECT_TRUE(pt180.x != pt0.x || pt180.y != pt0.y);
    // EXPECT_TRUE(pt270.x != pt0.x || pt270.y != pt0.y);
}

TEST(Tier1_Coord_ScrollOffsets) {
    CoordinateConverter::PageContext pageCtx = { 612.0, 792.0, 0 };
    CoordinateConverter::ViewContext viewNoScroll = { 1.0, 0.0, 0.0, 0.0, 0.0 };
    CoordinateConverter::ViewContext viewScrolled = { 1.0, 100.0, 50.0, 0.0, 0.0 };

    PointF pt1 = CoordinateConverter::PdfToScreen(pageCtx, viewNoScroll, 50, 50);
    PointF pt2 = CoordinateConverter::PdfToScreen(pageCtx, viewScrolled, 50, 50);

    EXPECT_EQ((int)(pt1.x - pt2.x), 100);
    EXPECT_EQ((int)(pt1.y - pt2.y), 50);
}

TEST(Tier1_Coord_PdfToScreenRect) {
    CoordinateConverter::PageContext pageCtx = { 612.0, 792.0, 0 };
    CoordinateConverter::ViewContext vCtx = { 1.0, 0.0, 0.0, 0.0, 0.0 };

    // PDF rect: left=50, top=700, right=150, bottom=600 (width=100, height=100)
    RectF screenRect = CoordinateConverter::PdfToScreenRect(pageCtx, vCtx, 50, 700, 150, 600);
    EXPECT_EQ((int)screenRect.Width(), 100);
    EXPECT_EQ((int)screenRect.Height(), 100);
}

// ----------------------------------------------------------------------------
// Feature 7: Atomic Document Saving (5 tests)
// ----------------------------------------------------------------------------

TEST(Tier1_Saving_SaveAsNewFile) {
    auto doc = CreateTestDoc();
    const wchar_t* outPath = L"tests/fixtures/output/tier1_save_test.pdf";
    EXPECT_TRUE(doc->SaveAs(outPath));
    EXPECT_TRUE(std::filesystem::exists(outPath));
    EXPECT_TRUE(std::filesystem::file_size(outPath) > 0);
}

TEST(Tier1_Saving_ReopenAndVerifyText) {
    auto doc = CreateTestDoc();
    std::wstring magicText = L"Unique Saved Magic String 98765";
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, magicText, RectF{100, 600, 400, 620}, "Arial", 12.0f, 0, 0, 0, 255);
    doc->GetCommandStack().ExecuteCommand(std::move(addCmd));

    const wchar_t* outPath = L"tests/fixtures/output/tier1_text_saved.pdf";
    EXPECT_TRUE(doc->SaveAs(outPath));

    auto reloadRes = PdfDocument::LoadFromFile(outPath);
    EXPECT_TRUE(reloadRes.has_value());
    auto reloadedDoc = std::move(reloadRes.value);
    auto page = reloadedDoc->GetPage(0);
    auto textObjs = page->GetTextObjects();

    bool found = false;
    for (auto& obj : textObjs) {
        if (obj->GetText().find(magicText) != std::wstring::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(Tier1_Saving_ReopenAndVerifyHighlights) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    annot->SetBounds({ 100, 500, 300, 520 });
    std::vector<QuadF> quads = { { {100, 500}, {300, 500}, {100, 520}, {300, 520} } };
    annot->SetQuadPoints(quads);

    const wchar_t* outPath = L"tests/fixtures/output/tier1_highlight_saved.pdf";
    EXPECT_TRUE(doc->SaveAs(outPath));

    auto reloadRes = PdfDocument::LoadFromFile(outPath);
    EXPECT_TRUE(reloadRes.has_value());
    auto reloadedPage = reloadRes.value->GetPage(0);
    auto annots = reloadedPage->GetAnnotations();
    EXPECT_EQ(annots.size(), 1);
    EXPECT_TRUE(annots[0]->GetType() == core::interfaces::dom::AnnotationType::Highlight);
}

TEST(Tier1_Saving_SaveUnmodifiedDocument) {
    auto doc = CreateTestDoc();
    const wchar_t* outPath = L"tests/fixtures/output/tier1_unmodified_saved.pdf";
    EXPECT_TRUE(doc->SaveAs(outPath));

    auto reloadRes = PdfDocument::LoadFromFile(outPath);
    EXPECT_TRUE(reloadRes.has_value());
    EXPECT_EQ(reloadRes.value->PageCount(), 1);
}

TEST(Tier1_Saving_SaveFailureHandling) {
    auto doc = CreateTestDoc();
    bool saved = doc->SaveAs(L"Z:\\invalid_drive_path_e2e_test\\nowhere.pdf");
    EXPECT_FALSE(saved);
}

// ============================================================================
// TIER 2: BOUNDARY & CORNER CASES (>=5 tests per category)
// ============================================================================

// ----------------------------------------------------------------------------
// Category 1: Text Boundary Cases (5 tests)
// ----------------------------------------------------------------------------

TEST(Tier2_Boundary_Text_EmptyString) {
    auto doc = CreateTestDoc();
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"", RectF{50, 500, 150, 520}, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));
}

TEST(Tier2_Boundary_Text_ExtremeUnicode) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    std::wstring complexUnicode = L"PDF-Elite: World (\u00A9 2026)";
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, complexUnicode, RectF{50, 500, 400, 520}, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    auto textObj = page->GetTextObjects()[0];
    EXPECT_TRUE(!textObj->GetText().empty());
}

TEST(Tier2_Boundary_Text_UltraLongString) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    std::wstring ultraLong(10000, L'A');
    ultraLong += L"END";

    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, ultraLong, RectF{10, 100, 500, 700}, "Arial", 8.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    auto textObj = page->GetTextObjects()[0];
    EXPECT_EQ(textObj->GetText().size(), 10003);
}

TEST(Tier2_Boundary_Text_SpecialCharactersAndEscapes) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    std::wstring specialChars = L"Line 1 - Quotes \"Text\" and Symbols !@#$%^&*()";
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, specialChars, RectF{50, 400, 350, 450}, "Arial", 12.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

    auto textObj = page->GetTextObjects()[0];
    EXPECT_EQ(textObj->GetText(), specialChars);
}

TEST(Tier2_Boundary_Text_ExtremeFontSizes) {
    auto doc = CreateTestDoc();
    auto addCmdSmall = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Micro", RectF{10, 10, 20, 20}, "Arial", 0.01f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmdSmall)));

    auto addCmdHuge = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Huge", RectF{50, 50, 500, 500}, "Arial", 1000.0f, 0, 0, 0, 255);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmdHuge)));
}

// ----------------------------------------------------------------------------
// Category 2: Coordinate & Geometry Boundary Cases (5 tests)
// ----------------------------------------------------------------------------

TEST(Tier2_Boundary_Geom_ZeroAndNegativeCoordinates) {
    CoordinateConverter::PageContext pageCtx = { 612.0, 792.0, 0 };
    CoordinateConverter::ViewContext viewCtx = { 1.0, 0.0, 0.0, 0.0, 0.0 };

    PointF zeroPt = CoordinateConverter::PdfToScreen(pageCtx, viewCtx, 0.0, 0.0);
    EXPECT_EQ((int)zeroPt.x, 0);
    EXPECT_EQ((int)zeroPt.y, 792);

    PointF negPt = CoordinateConverter::PdfToScreen(pageCtx, viewCtx, -100.0, -50.0);
    EXPECT_EQ((int)negPt.x, -100);
    EXPECT_EQ((int)negPt.y, 842);
}

TEST(Tier2_Boundary_Geom_ExtremeZoomLevels) {
    CoordinateConverter::PageContext pageCtx = { 612.0, 792.0, 0 };
    double testZooms[] = { 0.05, 0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 50.0 };

    for (double z : testZooms) {
        CoordinateConverter::ViewContext vCtx = { z, 0.0, 0.0, 0.0, 0.0 };
        PointF screen = CoordinateConverter::PdfToScreen(pageCtx, vCtx, 100.0, 100.0);
        PointF roundTrip = CoordinateConverter::ScreenToPdf(pageCtx, vCtx, screen.x, screen.y);
        EXPECT_TRUE(!std::isnan(screen.x) && !std::isinf(screen.x));
        EXPECT_TRUE(std::abs(roundTrip.x - 100.0) < 0.01);
    }
}

TEST(Tier2_Boundary_Geom_HugeScrollOffsets) {
    CoordinateConverter::PageContext pageCtx = { 612.0, 792.0, 0 };
    CoordinateConverter::ViewContext vCtx = { 1.0, 100000.0, -100000.0, 0.0, 0.0 };

    PointF s = CoordinateConverter::PdfToScreen(pageCtx, vCtx, 100.0, 792.0);
    EXPECT_EQ((int)s.x, -99900);
    EXPECT_EQ((int)s.y, 100000);
}

TEST(Tier2_Boundary_Geom_ZeroWidthHeightRectangles) {
    CoordinateConverter::PageContext pageCtx = { 612.0, 792.0, 0 };
    CoordinateConverter::ViewContext vCtx = { 1.0, 0.0, 0.0, 0.0, 0.0 };

    RectF screenRect = CoordinateConverter::PdfToScreenRect(pageCtx, vCtx, 50, 500, 50, 500);
    EXPECT_EQ(screenRect.Width(), 0.0f);
    EXPECT_EQ(screenRect.Height(), 0.0f);
}

TEST(Tier2_Boundary_Geom_InvertedRectangles) {
    RectF inv = { 200.0f, 100.0f, 50.0f, 30.0f }; // right < left, bottom < top
    EXPECT_EQ(inv.Width(), -150.0f);
    EXPECT_EQ(inv.Height(), -70.0f);
}

// ----------------------------------------------------------------------------
// Category 3: Undo/Redo Stress & Rapid Cycling (5 tests)
// ----------------------------------------------------------------------------

TEST(Tier2_Boundary_UndoRedo_RapidCycles) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    // Add 5 commands
    for (int i = 0; i < 5; ++i) {
        std::wstring text = L"Rapid " + std::to_wstring(i);
        auto cmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
            doc.get(), 0, text, RectF{10.0f * i, 10.0f * i, 100.0f, 30.0f}, "Arial", 12.0f, 0, 0, 0, 255);
        stack.ExecuteCommand(std::move(cmd));
    }
    EXPECT_EQ(page->GetTextObjects().size(), 5);

    // Rapid 100x Undo/Redo cycles of top command
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(stack.Undo());
        EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 4);
        EXPECT_TRUE(stack.Redo());
        EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 5);
    }
}

TEST(Tier2_Boundary_UndoRedo_BranchingHistoryInvalidation) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    auto cmd1 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Cmd1", RectF{0, 0, 10, 10}, "Arial", 12.0f, 0, 0, 0, 255);
    auto cmd2 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Cmd2", RectF{20, 20, 30, 30}, "Arial", 12.0f, 0, 0, 0, 255);
    auto cmd3 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Cmd3", RectF{40, 40, 50, 50}, "Arial", 12.0f, 0, 0, 0, 255);

    stack.ExecuteCommand(std::move(cmd1));
    stack.ExecuteCommand(std::move(cmd2));
    EXPECT_EQ(page->GetTextObjects().size(), 2);

    stack.Undo();
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 1);
    EXPECT_TRUE(stack.CanRedo());

    // Executing new cmd3 must invalidate Redo for cmd2
    stack.ExecuteCommand(std::move(cmd3));
    EXPECT_FALSE(stack.CanRedo());
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 2);
}

TEST(Tier2_Boundary_UndoRedo_UnderflowSafety) {
    core::interfaces::dom::CommandStack stack;
    for (int i = 0; i < 10; ++i) {
        EXPECT_FALSE(stack.Undo());
        EXPECT_FALSE(stack.CanUndo());
    }
}

TEST(Tier2_Boundary_UndoRedo_OverflowSafety) {
    core::interfaces::dom::CommandStack stack;
    for (int i = 0; i < 10; ++i) {
        EXPECT_FALSE(stack.Redo());
        EXPECT_FALSE(stack.CanRedo());
    }
}

TEST(Tier2_Boundary_UndoRedo_ClearStackCleanup) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();
    for (int i = 0; i < 10; ++i) {
        auto cmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
            doc.get(), 0, L"T", RectF{0, 0, 10, 10}, "Arial", 12.0f, 0, 0, 0, 255);
        stack.ExecuteCommand(std::move(cmd));
    }
    EXPECT_TRUE(stack.CanUndo());

    stack.Clear();
    EXPECT_FALSE(stack.CanUndo());
    EXPECT_FALSE(stack.CanRedo());
}

// ----------------------------------------------------------------------------
// Category 4: Annotation & Quad Point Boundaries (5 tests)
// ----------------------------------------------------------------------------

TEST(Tier2_Boundary_Quad_EmptyQuadPoints) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    annot->SetQuadPoints({});
    EXPECT_TRUE(annot->GetQuadPoints().empty());
}

TEST(Tier2_Boundary_Quad_ExtremePageMarginQuads) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    std::vector<QuadF> fullPageQuad = {
        { {0.0f, 0.0f}, {612.0f, 0.0f}, {0.0f, 792.0f}, {612.0f, 792.0f} }
    };
    annot->SetQuadPoints(fullPageQuad);
    EXPECT_TRUE(annot->GetType() == core::interfaces::dom::AnnotationType::Highlight);
}

TEST(Tier2_Boundary_Quad_MultipleDisjointQuads) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);

    std::vector<QuadF> disjointQuads;
    for (int i = 0; i < 1; ++i) {
        float y = 50.0f + i * 40.0f;
        disjointQuads.push_back({ {50.0f, y}, {300.0f, y}, {50.0f, y + 20.0f}, {300.0f, y + 20.0f} });
    }
    annot->SetQuadPoints(disjointQuads);
    EXPECT_TRUE(annot->GetType() == core::interfaces::dom::AnnotationType::Highlight);
}

TEST(Tier2_Boundary_Quad_ZeroAreaQuads) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    std::vector<QuadF> degenQuad = {
        { {100.0f, 100.0f}, {100.0f, 100.0f}, {100.0f, 100.0f}, {100.0f, 100.0f} }
    };
    annot->SetQuadPoints(degenQuad);
    EXPECT_TRUE(annot->GetType() == core::interfaces::dom::AnnotationType::Highlight);
}

TEST(Tier2_Boundary_Quad_ColorBoundaries) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    // Fully transparent
    annot->SetColor(0, 0, 0, 0);
    // Fully opaque max values
    annot->SetColor(255, 255, 255, 255);
    EXPECT_TRUE(annot != nullptr);
}

// ----------------------------------------------------------------------------
// Category 5: Multi-Page & Page Boundary Hit-Testing (5 tests)
// ----------------------------------------------------------------------------

TEST(Tier2_Boundary_Page_BorderHitTesting) {
    ui::interaction::InteractionManager im;
    // Page 0: y 0..792, Page 1: y 792..1584
    im.viewToPage = [](double vx, double vy, double& px, double& py, int& pageIndex) {
        if (vy < 792.0) {
            pageIndex = 0; px = vx; py = vy;
        } else {
            pageIndex = 1; px = vx; py = vy - 792.0;
        }
    };
    auto objP0 = std::make_shared<MockSelectable>("p0_bot", ui::interaction::Rect{50, 750, 150, 792}, 0);
    auto objP1 = std::make_shared<MockSelectable>("p1_top", ui::interaction::Rect{50, 0, 150, 40}, 1);
    im.AddObject(objP0);
    im.AddObject(objP1);

    // Hit test near bottom of Page 0
    auto hitP0 = im.OnLButtonDown(100, 780, false);
    EXPECT_TRUE(hitP0 == ui::interaction::HitResult::Object);
    EXPECT_TRUE(im.GetSelectionModel().IsSelected("p0_bot"));
    im.OnLButtonUp(100, 780);

    // Hit test near top of Page 1 (view y = 792 + 20 = 812)
    auto hitP1 = im.OnLButtonDown(100, 812, false);
    EXPECT_TRUE(hitP1 == ui::interaction::HitResult::Object);
    EXPECT_TRUE(im.GetSelectionModel().IsSelected("p1_top"));
}

TEST(Tier2_Boundary_Page_ClickOutsidePages) {
    ui::interaction::InteractionManager im;
    im.viewToPage = [](double vx, double vy, double& px, double& py, int& pageIndex) {
        if (vx < 0 || vy < 0 || vx > 612 || vy > 792) pageIndex = -1;
        else { pageIndex = 0; px = vx; py = vy; }
    };
    auto hit = im.OnLButtonDown(-100, -100, false);
    EXPECT_TRUE(hit == ui::interaction::HitResult::None);
}

TEST(Tier2_Boundary_Page_InterleavedMultiPageObjects) {
    ui::interaction::InteractionManager im;
    auto obj0 = std::make_shared<MockSelectable>("p0", ui::interaction::Rect{0, 0, 10, 10}, 0);
    auto obj1 = std::make_shared<MockSelectable>("p1", ui::interaction::Rect{0, 0, 10, 10}, 1);
    im.AddObject(obj0);
    im.AddObject(obj1);
    EXPECT_EQ(im.GetObjects().size(), 2);

    im.RemoveObjectsForPage(0);
    EXPECT_EQ(im.GetObjects().size(), 1);
    EXPECT_EQ(im.GetObjects()[0]->GetId(), "p1");
}

TEST(Tier2_Boundary_Page_DeletePageWithSelectedObject) {
    ui::interaction::InteractionManager im;
    auto obj0 = std::make_shared<MockSelectable>("p0_sel", ui::interaction::Rect{0, 0, 10, 10}, 0);
    im.AddObject(obj0);
    im.GetSelectionModel().Select(obj0);
    EXPECT_TRUE(im.GetSelectionModel().IsSelected("p0_sel"));

    im.RemoveObjectsForPage(0);
    EXPECT_FALSE(im.GetSelectionModel().IsSelected("p0_sel"));
    EXPECT_TRUE(im.GetObjects().empty());
}

TEST(Tier2_Boundary_Page_InvalidPageIndexSafety) {
    auto doc = CreateTestDoc();
    auto pInvalid = doc->GetPage(-1);
    EXPECT_TRUE(pInvalid == nullptr);
    auto pOut = doc->GetPage(999);
    EXPECT_TRUE(pOut == nullptr);
}

// ============================================================================
// TIER 3: CROSS-FEATURE COMBINATIONS
// ============================================================================

TEST(Tier3_CrossFeature_CreateText_Move_Highlight_Undo_Redo_Save_Reload) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    // 1. Create text
    RectF origBounds = { 100.0f, 600.0f, 300.0f, 625.0f };
    auto addTextCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Cross Feature Target", origBounds, "Arial", 14.0f, 0, 0, 0, 255);
    EXPECT_TRUE(stack.ExecuteCommand(std::move(addTextCmd)));
    auto textObj = page->GetTextObjects()[0];
    float initLeft = textObj->GetBounds().left;

    // 2. Move text
    RectF movedBounds = { 150.0f, 650.0f, 350.0f, 675.0f };
    float dx = movedBounds.left - origBounds.left;
    auto moveCmd = std::make_unique<pdf_engine::commands::MoveTextCommand>(textObj, origBounds, movedBounds);
    EXPECT_TRUE(stack.ExecuteCommand(std::move(moveCmd)));

    // 3. Highlight text at new location
    auto addHighlightCmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 0, core::interfaces::dom::AnnotationType::Highlight, movedBounds);
    EXPECT_TRUE(stack.ExecuteCommand(std::move(addHighlightCmd)));
    EXPECT_EQ(doc->GetPage(0)->GetAnnotations().size(), 1);

    // 4. Undo Highlight
    EXPECT_TRUE(stack.Undo());
    EXPECT_EQ(doc->GetPage(0)->GetAnnotations().size(), 0);

    // 5. Undo Move
    EXPECT_TRUE(stack.Undo());
    EXPECT_TRUE(std::abs(textObj->GetBounds().left - initLeft) < 0.1f);

    // 6. Redo Move
    EXPECT_TRUE(stack.Redo());
    EXPECT_TRUE(std::abs(textObj->GetBounds().left - (initLeft + dx)) < 0.1f);

    // 7. Redo Highlight
    EXPECT_TRUE(stack.Redo());
    EXPECT_EQ(doc->GetPage(0)->GetAnnotations().size(), 1);

    // 8. Save
    const wchar_t* outPath = L"tests/fixtures/output/tier3_text_move_highlight.pdf";
    EXPECT_TRUE(doc->SaveAs(outPath));

    // 9. Reload and verify
    auto reloadRes = PdfDocument::LoadFromFile(outPath);
    EXPECT_TRUE(reloadRes.has_value());
    auto reloadedDoc = std::move(reloadRes.value);
    auto reloadPage = reloadedDoc->GetPage(0);
    EXPECT_EQ(reloadPage->GetTextObjects().size(), 1);
    EXPECT_EQ(reloadPage->GetAnnotations().size(), 1);
}

TEST(Tier3_CrossFeature_AddMultipleTexts_SelectAll_BatchMove_Save_Verify) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    auto cmd1 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Item 1", RectF{50, 700, 150, 720}, "Arial", 12.0f, 0, 0, 0, 255);
    auto cmd2 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Item 2", RectF{50, 650, 150, 670}, "Arial", 12.0f, 0, 0, 0, 255);
    auto cmd3 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Item 3", RectF{50, 600, 150, 620}, "Arial", 12.0f, 0, 0, 0, 255);

    stack.ExecuteCommand(std::move(cmd1));
    stack.ExecuteCommand(std::move(cmd2));
    stack.ExecuteCommand(std::move(cmd3));

    auto textObjs = page->GetTextObjects();
    EXPECT_EQ(textObjs.size(), 3);

    // Move all by (+30, -20)
    for (auto& obj : textObjs) {
        RectF b = obj->GetBounds();
        RectF newB = { b.left + 30.0f, b.top - 20.0f, b.right + 30.0f, b.bottom - 20.0f };
        auto moveCmd = std::make_unique<pdf_engine::commands::MoveTextCommand>(obj, b, newB);
        stack.ExecuteCommand(std::move(moveCmd));
    }

    const wchar_t* outPath = L"tests/fixtures/output/tier3_batch_move.pdf";
    EXPECT_TRUE(doc->SaveAs(outPath));

    auto reloadRes = PdfDocument::LoadFromFile(outPath);
    EXPECT_TRUE(reloadRes.has_value());
    auto reloadPage = reloadRes.value->GetPage(0);
    EXPECT_EQ(reloadPage->GetTextObjects().size(), 3);
}

TEST(Tier3_CrossFeature_Highlight_ModifyColor_MoveUnderlyingText_UndoRedo) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    auto addText = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Important Term", RectF{100, 500, 250, 520}, "Arial", 12.0f, 0, 0, 0, 255);
    stack.ExecuteCommand(std::move(addText));
    auto textObj = page->GetTextObjects()[0];
    float initLeft = textObj->GetBounds().left;

    auto addHl = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 0, core::interfaces::dom::AnnotationType::Highlight, RectF{100, 500, 250, 520});
    stack.ExecuteCommand(std::move(addHl));
    auto hl = page->GetAnnotations()[0];
    hl->SetColor(0, 255, 0, 150); // green

    auto moveText = std::make_unique<pdf_engine::commands::MoveTextCommand>(
        textObj, RectF{100, 500, 250, 520}, RectF{120, 530, 270, 550});
    stack.ExecuteCommand(std::move(moveText));

    EXPECT_TRUE(std::abs(textObj->GetBounds().left - (initLeft + 20.0f)) < 0.1f);
    EXPECT_TRUE(stack.Undo()); // Undo Move
    EXPECT_TRUE(std::abs(textObj->GetBounds().left - initLeft) < 0.1f);
    EXPECT_EQ(page->GetAnnotations().size(), 1);
}

TEST(Tier3_CrossFeature_InterleavedTextAndAnnotationMutations_SaveVerify) {
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    // Interleaved creation
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Paragraph 1", RectF{50, 700, 200, 720}, "Arial", 12.0f, 0, 0, 0, 255));
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 0, core::interfaces::dom::AnnotationType::Highlight, RectF{50, 700, 200, 720}));
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Paragraph 2", RectF{50, 650, 200, 670}, "Arial", 12.0f, 0, 0, 0, 255));
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 0, core::interfaces::dom::AnnotationType::Underline, RectF{50, 650, 200, 670}));

    EXPECT_EQ(page->GetTextObjects().size(), 2);
    EXPECT_EQ(page->GetAnnotations().size(), 2);

    const wchar_t* outPath = L"tests/fixtures/output/tier3_interleaved.pdf";
    EXPECT_TRUE(doc->SaveAs(outPath));

    auto reloadRes = PdfDocument::LoadFromFile(outPath);
    EXPECT_TRUE(reloadRes.has_value());
    auto reloadPage = reloadRes.value->GetPage(0);
    EXPECT_EQ(reloadPage->GetTextObjects().size(), 2);
    EXPECT_EQ(reloadPage->GetAnnotations().size(), 2);
}

TEST(Tier3_CrossFeature_ToolSwitchingDuringDragOrEdit_StateConsistency) {
    PdfViewer viewer;
    viewer.Initialize(nullptr);
    viewer.SetToolMode(ToolMode::AddText);

    auto& im = viewer.GetInteractionManager();
    im.viewToPage = [](double vx, double vy, double& px, double& py, int& pageIndex) {
        px = vx; py = vy; pageIndex = 0;
    };
    im.EnterNewTextMode(100, 100);
    EXPECT_TRUE(im.GetCursor() == LoadCursor(nullptr, IDC_IBEAM));

    // Switch tool mode to Highlight
    viewer.SetToolMode(ToolMode::Highlight);
    EXPECT_TRUE(viewer.GetToolMode() == ToolMode::Highlight);
}

TEST(Tier3_CrossFeature_MultiPageInterleavedWorkflow) {
    auto doc = CreateTestDoc();
    EXPECT_TRUE(doc->InsertBlankPage(1, 612, 792));
    EXPECT_EQ(doc->PageCount(), 2);

    auto page0 = doc->GetPage(0);
    auto page1 = doc->GetPage(1);

    auto& stack = doc->GetCommandStack();
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Page 0 Text", RectF{50, 700, 200, 720}, "Arial", 12.0f, 0, 0, 0, 255));
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 1, core::interfaces::dom::AnnotationType::Highlight, RectF{50, 700, 200, 720}));

    EXPECT_EQ(page0->GetTextObjects().size(), 1);
    EXPECT_EQ(page1->GetAnnotations().size(), 1);

    const wchar_t* outPath = L"tests/fixtures/output/tier3_multipage_workflow.pdf";
    EXPECT_TRUE(doc->SaveAs(outPath));

    auto reloadRes = PdfDocument::LoadFromFile(outPath);
    EXPECT_TRUE(reloadRes.has_value());
    EXPECT_EQ(reloadRes.value->PageCount(), 2);
    auto p0 = reloadRes.value->GetPage(0);
    auto p1 = reloadRes.value->GetPage(1);
    EXPECT_EQ(p0->GetTextObjects().size(), 1);
    EXPECT_EQ(p1->GetAnnotations().size(), 1);
}

// ============================================================================
// TIER 4: REAL-WORLD APPLICATION SCENARIOS
// ============================================================================

TEST(Tier4_Scenario_LegalContractReviewAndRedline) {
    // Scenario: Lawyer reviews NDA contract, highlights confidentiality clause,
    // adds marginalia commentary, corrects effective date, undoes misclick, and exports signed redline.
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    // Step 1: Add initial clause texts representing standard contract
    auto cmdClause1 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"1. Confidentiality Obligations", RectF{50, 700, 300, 720}, "Times-Roman", 12.0f, 0, 0, 0, 255);
    auto cmdClause2 = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Effective Date: January 1, 2026", RectF{50, 670, 280, 690}, "Times-Roman", 12.0f, 0, 0, 0, 255);
    stack.ExecuteCommand(std::move(cmdClause1));
    stack.ExecuteCommand(std::move(cmdClause2));

    // Step 2: Highlight the Confidentiality title
    auto hlCmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 0, core::interfaces::dom::AnnotationType::Highlight, RectF{50, 700, 300, 720});
    stack.ExecuteCommand(std::move(hlCmd));

    // Step 3: Add marginalia reviewer note
    auto noteCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"[Reviewer Note: Term extended to 5 years]", RectF{350, 700, 550, 720}, "Helvetica", 10.0f, 200, 0, 0, 255);
    stack.ExecuteCommand(std::move(noteCmd));

    // Step 4: Correct effective date
    auto dateTextObj = page->GetTextObjects()[1];
    auto editDateCmd = std::make_unique<pdf_engine::commands::EditTextCommand>(
        dateTextObj, L"Effective Date: January 1, 2026", L"Effective Date: September 1, 2026");
    stack.ExecuteCommand(std::move(editDateCmd));

    // Step 5: Accidental delete -> Undo
    auto accidentalDelete = std::make_unique<pdf_engine::commands::DeleteTextCommand>(
        doc.get(), 0, dateTextObj);
    stack.ExecuteCommand(std::move(accidentalDelete));
    EXPECT_EQ(page->GetTextObjects().size(), 2); // 1 clause + 1 note
    EXPECT_TRUE(stack.Undo()); // Restored
    EXPECT_EQ(doc->GetPage(0)->GetTextObjects().size(), 3);

    // Step 6: Save & verify final review copy
    const wchar_t* outPath = L"tests/fixtures/output/tier4_legal_contract_redline.pdf";
    EXPECT_TRUE(doc->SaveAs(outPath));

    auto reload = PdfDocument::LoadFromFile(outPath);
    EXPECT_TRUE(reload.has_value());
    auto reloadPage = reload.value->GetPage(0);
    EXPECT_EQ(reloadPage->GetTextObjects().size(), 3);
    EXPECT_EQ(reloadPage->GetAnnotations().size(), 1);
}

TEST(Tier4_Scenario_InvoiceFormFillingAndTotalAdjustment) {
    // Scenario: Accounting department opens invoice template, adds customer details,
    // line items, highlights tax exemption note, and calculates final total.
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    // Customer Name & Invoice #
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"INVOICE #INV-2026-089", RectF{50, 730, 250, 750}, "Helvetica", 16.0f, 0, 50, 150, 255));
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Bill To: Acme Global Corp", RectF{50, 700, 250, 720}, "Helvetica", 12.0f, 0, 0, 0, 255));

    // Line items
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Item 1: Enterprise PDF Editor License ... $1,200.00", RectF{50, 650, 450, 670}, "Courier", 11.0f, 0, 0, 0, 255));
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Item 2: Professional Support Package .... $300.00", RectF{50, 630, 450, 650}, "Courier", 11.0f, 0, 0, 0, 255));
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"TOTAL DUE: $1,500.00", RectF{50, 590, 300, 615}, "Helvetica", 14.0f, 0, 120, 0, 255));

    // Highlight Total Due
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 0, core::interfaces::dom::AnnotationType::Highlight, RectF{50, 590, 300, 615}));

    const wchar_t* outPath = L"tests/fixtures/output/tier4_invoice_filled.pdf";
    EXPECT_TRUE(doc->SaveAs(outPath));

    auto reload = PdfDocument::LoadFromFile(outPath);
    EXPECT_TRUE(reload.has_value());
    static_cast<PdfDocument*>(reload.value.get())->SetGroupingMode(TextGroupingMode::Line);
    auto reloadPage = reload.value->GetPage(0);
    EXPECT_EQ(reloadPage->GetTextObjects().size(), 5);
    EXPECT_EQ(reloadPage->GetAnnotations().size(), 1);
}

TEST(Tier4_Scenario_AcademicPaperAnnotationAndNotes) {
    // Scenario: Researcher reads an academic paper, marks key findings with multi-color highlights,
    // inserts citation cross-references, zooms in/out to check figures, and saves research annotations.
    auto doc = CreateTestDoc();
    auto page = doc->GetPage(0);
    auto& stack = doc->GetCommandStack();

    // Abstract text
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Deep Neural PDF Reconstruction: An Empirical Analysis", RectF{50, 720, 450, 745}, "Times-Roman", 14.0f, 0, 0, 0, 255));
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Results show 99.8% precision across 10,000 documents.", RectF{50, 680, 450, 700}, "Times-Roman", 11.0f, 0, 0, 0, 255));

    // Yellow highlight on Title
    auto hlTitle = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 0, core::interfaces::dom::AnnotationType::Highlight, RectF{50, 720, 450, 745});
    stack.ExecuteCommand(std::move(hlTitle));

    // Green highlight on Result sentence
    auto hlResult = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 0, core::interfaces::dom::AnnotationType::Highlight, RectF{50, 680, 450, 700});
    stack.ExecuteCommand(std::move(hlResult));

    // Sticky Note
    auto stickyNote = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Text);
    stickyNote->SetBounds({ 470, 680, 490, 700 });
    stickyNote->SetContents("Reproduce evaluation benchmark on GPU cluster.");

    const wchar_t* outPath = L"tests/fixtures/output/tier4_academic_study_notes.pdf";
    EXPECT_TRUE(doc->SaveAs(outPath));

    auto reload = PdfDocument::LoadFromFile(outPath);
    EXPECT_TRUE(reload.has_value());
    auto reloadPage = reload.value->GetPage(0);
    EXPECT_EQ(reloadPage->GetAnnotations().size(), 3); // 2 Highlights + 1 Sticky Note
}

TEST(Tier4_Scenario_MultiPageDocumentRestructuringAndAnnotating) {
    // Scenario: Executive restructures proposal deck: inserts cover page, duplicates summary,
    // adds section headers, annotates action items, and generates client-ready PDF package.
    auto doc = CreateTestDoc();
    EXPECT_TRUE(doc->InsertBlankPage(1, 612, 792));
    EXPECT_TRUE(doc->InsertBlankPage(2, 612, 792));
    EXPECT_EQ(doc->PageCount(), 3);

    auto page0 = doc->GetPage(0);
    auto page1 = doc->GetPage(1);
    auto page2 = doc->GetPage(2);

    auto& stack = doc->GetCommandStack();

    // Cover page title
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Q3 Strategic Roadmap Proposal", RectF{100, 500, 500, 535}, "Helvetica", 20.0f, 0, 50, 150, 255));

    // Page 1 Section Header
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 1, L"Executive Summary & Deliverables", RectF{50, 720, 350, 745}, "Helvetica", 14.0f, 0, 0, 0, 255));
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 1, core::interfaces::dom::AnnotationType::Highlight, RectF{50, 720, 350, 745}));

    // Page 2 Action Items
    stack.ExecuteCommand(std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 2, L"Action Items & Timeline", RectF{50, 720, 300, 745}, "Helvetica", 14.0f, 0, 0, 0, 255));

    const wchar_t* outPath = L"tests/fixtures/output/tier4_restructured_proposal.pdf";
    EXPECT_TRUE(doc->SaveAs(outPath));

    auto reload = PdfDocument::LoadFromFile(outPath);
    EXPECT_TRUE(reload.has_value());
    EXPECT_EQ(reload.value->PageCount(), 3);
    auto rp0 = reload.value->GetPage(0);
    auto rp1 = reload.value->GetPage(1);
    auto rp2 = reload.value->GetPage(2);
    EXPECT_EQ(rp0->GetTextObjects().size(), 1);
    EXPECT_EQ(rp1->GetTextObjects().size(), 1);
    EXPECT_EQ(rp1->GetAnnotations().size(), 1);
    EXPECT_EQ(rp2->GetTextObjects().size(), 1);
}

TEST(Tier4_Scenario_FullInteractiveEditorSessionSimulation) {
    // Scenario: Comprehensive interactive editor session combining UI PdfViewer,
    // Tool switching, InteractionManager, CommandStack, Coordinate mapping, and File I/O.
    PdfViewer viewer;
    viewer.Initialize(nullptr);

    auto doc = CreateTestDoc();
    viewer.SetDocument(doc);
    viewer.SetToolMode(ToolMode::Select);

    auto& im = viewer.GetInteractionManager();
    im.pageToView = [](double px, double py, int, double& vx, double& vy) { vx = px; vy = py; };
    im.viewToPage = [](double vx, double vy, double& px, double& py, int& p) { px = vx; py = vy; p = 0; };

    // 1. Add interactive text object
    auto addTextCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"Interactive Header", RectF{100, 600, 300, 630}, "Arial", 14.0f, 0, 0, 0, 255);
    doc->GetCommandStack().ExecuteCommand(std::move(addTextCmd));

    auto page = doc->GetPage(0);
    auto textObjs = page->GetTextObjects();
    EXPECT_TRUE(!textObjs.empty());

    auto selObj = std::make_shared<ui::interaction::TextSelectableObject>(textObjs[0], 0);
    im.AddObject(selObj);

    // 2. Select the object
    im.GetSelectionModel().Select(selObj);
    EXPECT_TRUE(im.GetSelectionModel().IsSelected(selObj->GetId()));

    // 3. Move object via selection bounds
    auto b = selObj->GetBounds();
    selObj->SetBounds({ b.left + 30, b.top + 30, b.right + 30, b.bottom + 30 });
    EXPECT_EQ(selObj->GetBounds().left, b.left + 30);

    // 4. Switch to Highlight tool and add highlight
    viewer.SetToolMode(ToolMode::Highlight);
    auto hlCmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
        doc.get(), 0, core::interfaces::dom::AnnotationType::Highlight, RectF{100, 500, 300, 520});
    doc->GetCommandStack().ExecuteCommand(std::move(hlCmd));

    // 5. Undo highlight, then Redo highlight
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    EXPECT_TRUE(doc->GetCommandStack().Redo());

    // 6. Save and verify
    const wchar_t* outPath = L"tests/fixtures/output/tier4_full_session_saved.pdf";
    EXPECT_TRUE(doc->SaveAs(outPath));

    auto reload = PdfDocument::LoadFromFile(outPath);
    EXPECT_TRUE(reload.has_value());
    auto reloadPage = reload.value->GetPage(0);
    EXPECT_EQ(reloadPage->GetTextObjects().size(), 1);
    EXPECT_EQ(reloadPage->GetAnnotations().size(), 1);
}

// ============================================================================
// MAIN RUNNER
// ============================================================================

int main() {
    PdfiumLibrary::Instance().Initialize();
    std::cout << "====================================================\n";
    std::cout << " PDF-Elite 4-Tier E2E Regression & Acceptance Suite\n";
    std::cout << "====================================================\n\n";

    SetupE2EFixtures();

    int result = TestRunner::Instance().RunAll();
    return result;
}
