#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <fpdfview.h>
#include <fpdf_annot.h>
#include <fpdf_edit.h>
#include <fpdf_save.h>

#include "../src/core/CommandStack.h"
#include "../src/pdf_engine/include/pdf_engine/commands/TransformAnnotationCommand.h"
#include "../src/pdf_engine/include/pdf_engine/commands/AddAnnotationCommand.h"
#include "../src/pdf_engine/include/pdf_engine/commands/AddInkAnnotationCommand.h"
#include "../src/ui/include/menu/ContextMenuManager.h"
#include "../src/ui/include/search/SearchHighlightOverlay.h"

#define ASSERT(condition) do { if (!(condition)) { std::cerr << "Assertion failed: " << #condition << " at line " << __LINE__ << std::endl; std::abort(); } } while(0)

// Helper mock command for unit tests
class MockValCommand : public core::ICommand {
public:
    MockValCommand(int* target, int delta, size_t memSize = 100, const std::string& desc = "MockVal")
        : m_target(target), m_delta(delta), m_memSize(memSize), m_desc(desc) {}

    bool Execute() override {
        if (!m_target) return false;
        *m_target += m_delta;
        return true;
    }

    bool Undo() override {
        if (!m_target) return false;
        *m_target -= m_delta;
        return true;
    }

    size_t GetMemorySize() const override { return m_memSize; }
    std::string GetDescription() const override { return m_desc; }

private:
    int* m_target;
    int m_delta;
    size_t m_memSize;
    std::string m_desc;
};

// Helper mock failing command
class MockFailingCommand : public core::ICommand {
public:
    MockFailingCommand(int* target) : m_target(target) {}
    bool Execute() override { return false; }
    bool Undo() override { if (m_target) (*m_target)--; return true; }
    size_t GetMemorySize() const override { return 50; }
private:
    int* m_target;
};

// Helper mergeable mock command (simulates continuous mouse drag or slider updates)
class MockMergeableDragCommand : public core::ICommand {
public:
    MockMergeableDragCommand(int* target, int fromVal, int toVal)
        : m_target(target), m_fromVal(fromVal), m_toVal(toVal) {}

    bool Execute() override {
        if (!m_target) return false;
        *m_target = m_toVal;
        return true;
    }

    bool Undo() override {
        if (!m_target) return false;
        *m_target = m_fromVal;
        return true;
    }

    bool CanMergeWith(const core::ICommand* other) const override {
        const auto* dragOther = dynamic_cast<const MockMergeableDragCommand*>(other);
        return dragOther && (dragOther->m_target == this->m_target);
    }

    bool MergeWith(std::unique_ptr<ICommand>& other) override {
        const auto* dragOther = dynamic_cast<const MockMergeableDragCommand*>(other.get());
        if (!dragOther || dragOther->m_target != this->m_target) return false;
        // Merge updates destination value while retaining initial starting value
        m_toVal = dragOther->m_toVal;
        return true;
    }

    int GetFromVal() const { return m_fromVal; }
    int GetToVal() const { return m_toVal; }

private:
    int* m_target;
    int m_fromVal;
    int m_toVal;
};

void TestCommandStackWithPdfium() {
    FPDF_InitLibrary();

    // Create a dummy document
    FPDF_DOCUMENT doc = FPDF_CreateNewDocument();
    ASSERT(doc != nullptr);

    // Create a page
    FPDF_PAGE page = FPDFPage_New(doc, 0, 612.0, 792.0); // Standard Letter size
    ASSERT(page != nullptr);
    
    // Add an annotation directly to test Transform
    FPDF_ANNOTATION annot = FPDFPage_CreateAnnot(page, FPDF_ANNOT_SQUARE);
    FS_RECTF initialRect = { 10.0f, 10.0f, 100.0f, 100.0f };
    FPDFAnnot_SetRect(annot, &initialRect);
    FPDFPage_CloseAnnot(annot);
    FPDF_ClosePage(page);

    core::CommandStack stack;

    // 1. Test TransformAnnotationCommand
    FS_RECTF newRect = { 50.0f, 50.0f, 150.0f, 150.0f };
    auto transformCmd = std::make_unique<pdf_engine::commands::TransformAnnotationCommand>(doc, 0, 0, initialRect, newRect);
    
    stack.ExecuteCommand(std::move(transformCmd));

    // Verify Rect is newRect
    page = FPDF_LoadPage(doc, 0);
    annot = FPDFPage_GetAnnot(page, 0);
    FS_RECTF currentRect;
    FPDFAnnot_GetRect(annot, &currentRect);
    ASSERT(currentRect.left == 50.0f && currentRect.right == 150.0f);
    FPDFPage_CloseAnnot(annot);

    // Undo
    ASSERT(stack.Undo());
    page = FPDF_LoadPage(doc, 0);
    annot = FPDFPage_GetAnnot(page, 0);
    FPDFAnnot_GetRect(annot, &currentRect);
    ASSERT(currentRect.left == 10.0f && currentRect.right == 100.0f); // Back to initial
    FPDFPage_CloseAnnot(annot);

    // Redo
    ASSERT(stack.Redo());
    page = FPDF_LoadPage(doc, 0);
    annot = FPDFPage_GetAnnot(page, 0);
    FPDFAnnot_GetRect(annot, &currentRect);
    ASSERT(currentRect.left == 50.0f && currentRect.right == 150.0f); // Back to newRect
    FPDFPage_CloseAnnot(annot);
    FPDF_ClosePage(page);

    // 2. Test AddAnnotationCommand
    FS_RECTF addRect = { 200.0f, 200.0f, 300.0f, 300.0f };
    auto addCmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(doc, 0, FPDF_ANNOT_CIRCLE, addRect);
    
    stack.ExecuteCommand(std::move(addCmd));

    page = FPDF_LoadPage(doc, 0);
    int annotCount = FPDFPage_GetAnnotCount(page);
    ASSERT(annotCount == 2); // 1 square, 1 circle
    FPDF_ClosePage(page);

    // Undo Add
    ASSERT(stack.Undo());
    page = FPDF_LoadPage(doc, 0);
    annotCount = FPDFPage_GetAnnotCount(page);
    ASSERT(annotCount == 1); // Back to 1 square
    FPDF_ClosePage(page);

    FPDF_CloseDocument(doc);
    FPDF_DestroyLibrary();
}

void TestAddInkAnnotation() {
    FPDF_InitLibrary();
    FPDF_DOCUMENT doc = FPDF_CreateNewDocument();
    FPDF_PAGE page = FPDFPage_New(doc, 0, 612.0, 792.0);
    FPDF_ClosePage(page);

    core::CommandStack stack;

    std::vector<pdf_engine::commands::AddInkAnnotationCommand::Stroke> strokes;
    strokes.push_back({{10.0f, 10.0f}, {20.0f, 20.0f}, {30.0f, 10.0f}});

    FS_RECTF rect = {10.0f, 10.0f, 30.0f, 20.0f};
    auto inkCmd = std::make_unique<pdf_engine::commands::AddInkAnnotationCommand>(
        doc, 0, strokes, rect, 0xFFFF0000, 2.0f);
    
    stack.ExecuteCommand(std::move(inkCmd));

    page = FPDF_LoadPage(doc, 0);
    ASSERT(FPDFPage_GetAnnotCount(page) == 1);
    
    FPDF_ANNOTATION annot = FPDFPage_GetAnnot(page, 0);
    ASSERT(FPDFAnnot_GetSubtype(annot) == FPDF_ANNOT_INK);
    
    // Validate that the stroke is saved
    unsigned long pathCount = FPDFAnnot_GetInkListCount(annot);
    ASSERT(pathCount == 1);
    
    FPDFPage_CloseAnnot(annot);
    FPDF_ClosePage(page);

    // Undo
    ASSERT(stack.Undo());
    page = FPDF_LoadPage(doc, 0);
    ASSERT(FPDFPage_GetAnnotCount(page) == 0);
    FPDF_ClosePage(page);

    FPDF_CloseDocument(doc);
    FPDF_DestroyLibrary();
}

void TestCommandStackGenerations() {
    core::CommandStack stack;
    ASSERT(stack.GetGeneration() == 0);

    int val = 0;
    stack.ExecuteCommand(std::make_unique<MockValCommand>(&val, 10));
    ASSERT(val == 10);
    ASSERT(stack.GetGeneration() == 1);

    stack.ExecuteCommand(std::make_unique<MockValCommand>(&val, 5));
    ASSERT(val == 15);
    ASSERT(stack.GetGeneration() == 2);

    ASSERT(stack.Undo());
    ASSERT(val == 10);
    ASSERT(stack.GetGeneration() == 3);

    ASSERT(stack.Redo());
    ASSERT(val == 15);
    ASSERT(stack.GetGeneration() == 4);

    stack.Clear();
    ASSERT(stack.GetGeneration() == 5);
    ASSERT(!stack.CanUndo());
    ASSERT(!stack.CanRedo());
}

void TestCommandStackMemoryBounding() {
    int val = 0;
    // Set max memory to 2500 bytes and max depth to 100
    core::CommandStack stack(100, 2500);

    // Push 4 commands of 1000 bytes each (4000 bytes total)
    stack.ExecuteCommand(std::make_unique<MockValCommand>(&val, 1, 1000));
    ASSERT(stack.GetUndoCount() == 1);
    ASSERT(stack.GetCurrentMemoryUsage() == 1000);

    stack.ExecuteCommand(std::make_unique<MockValCommand>(&val, 2, 1000));
    ASSERT(stack.GetUndoCount() == 2);
    ASSERT(stack.GetCurrentMemoryUsage() == 2000);

    stack.ExecuteCommand(std::make_unique<MockValCommand>(&val, 3, 1000));
    // 3000 exceeds 2500 -> first command (1000) evicted -> 2000 left
    ASSERT(stack.GetUndoCount() == 2);
    ASSERT(stack.GetCurrentMemoryUsage() == 2000);

    stack.ExecuteCommand(std::make_unique<MockValCommand>(&val, 4, 1000));
    // 3000 exceeds 2500 -> oldest evicted -> 2000 left (commands +3 and +4)
    ASSERT(stack.GetUndoCount() == 2);
    ASSERT(stack.GetCurrentMemoryUsage() == 2000);

    ASSERT(val == 10); // 1 + 2 + 3 + 4 = 10

    // Undo +4
    ASSERT(stack.Undo());
    ASSERT(val == 6);

    // Undo +3
    ASSERT(stack.Undo());
    ASSERT(val == 3);

    // No more undos since +1 and +2 were evicted due to memory limits
    ASSERT(!stack.CanUndo());
}

void TestCommandStackMaxDepth() {
    int val = 0;
    core::CommandStack stack(3, 1000000);

    for (int i = 1; i <= 5; ++i) {
        stack.ExecuteCommand(std::make_unique<MockValCommand>(&val, i, 10));
    }

    ASSERT(val == 15);
    ASSERT(stack.GetUndoCount() == 3); // Capped at depth 3

    ASSERT(stack.Undo()); // Undo 5 -> 10
    ASSERT(val == 10);
    ASSERT(stack.Undo()); // Undo 4 -> 6
    ASSERT(val == 6);
    ASSERT(stack.Undo()); // Undo 3 -> 3
    ASSERT(val == 3);
    ASSERT(!stack.CanUndo()); // 1 and 2 were pruned
}

void TestCommandCoalescing() {
    int position = 0;
    core::CommandStack stack;

    // Simulate mouse dragging from 0 to 10, then 10 to 20, then 20 to 35
    stack.ExecuteCommand(std::make_unique<MockMergeableDragCommand>(&position, 0, 10));
    ASSERT(position == 10);
    ASSERT(stack.GetUndoCount() == 1);

    stack.ExecuteCommand(std::make_unique<MockMergeableDragCommand>(&position, 10, 20));
    ASSERT(position == 20);
    ASSERT(stack.GetUndoCount() == 1); // Merged into previous command!

    stack.ExecuteCommand(std::make_unique<MockMergeableDragCommand>(&position, 20, 35));
    ASSERT(position == 35);
    ASSERT(stack.GetUndoCount() == 1); // Still merged into one single undo step!

    // Single undo restores all the way back to initial 0!
    ASSERT(stack.Undo());
    ASSERT(position == 0);

    // Single redo brings to final 35!
    ASSERT(stack.Redo());
    ASSERT(position == 35);
}

void TestMacroCommandAtomicRollback() {
    int val = 100;
    auto macro = std::make_unique<core::MacroCommand>("Composite Arithmetic");

    macro->AddCommand(std::make_unique<MockValCommand>(&val, 10)); // val -> 110
    macro->AddCommand(std::make_unique<MockValCommand>(&val, 20)); // val -> 130
    macro->AddCommand(std::make_unique<MockFailingCommand>(&val));  // Fails!

    // Execution must fail and roll back all steps cleanly
    ASSERT(!macro->Execute());
    ASSERT(val == 100); // Completely reverted!

    // Now test successful macro
    auto goodMacro = std::make_unique<core::MacroCommand>("Success Macro");
    goodMacro->AddCommand(std::make_unique<MockValCommand>(&val, 50));
    goodMacro->AddCommand(std::make_unique<MockValCommand>(&val, 25));

    core::CommandStack stack;
    stack.ExecuteCommand(std::move(goodMacro));
    ASSERT(val == 175);
    ASSERT(stack.GetUndoCount() == 1);

    ASSERT(stack.Undo());
    ASSERT(val == 100);

    ASSERT(stack.Redo());
    ASSERT(val == 175);
}

void TestSaveStateTracking() {
    int val = 0;
    core::CommandStack stack;

    ASSERT(!stack.IsDirty());

    stack.ExecuteCommand(std::make_unique<MockValCommand>(&val, 10));
    ASSERT(stack.IsDirty());

    stack.MarkSaved();
    ASSERT(!stack.IsDirty());

    stack.ExecuteCommand(std::make_unique<MockValCommand>(&val, 20));
    ASSERT(stack.IsDirty());

    ASSERT(stack.Undo());
    ASSERT(!stack.IsDirty()); // Back at save position!

    ASSERT(stack.Undo());
    ASSERT(stack.IsDirty()); // Before save position

    ASSERT(stack.Redo());
    ASSERT(!stack.IsDirty()); // Back at save position!
}

void TestContextMenuManager() {
    auto& mgr = ui::menu::ContextMenuManager::Instance();

    // 1. Text selection context
    ui::menu::ContextMenuInfo textSelInfo;
    textSelInfo.targetType = ui::menu::TargetType::TextSelection;
    textSelInfo.selectedText = L"Sample PDF Text";
    auto textSelItems = mgr.BuildMenuItems(textSelInfo);
    ASSERT(!textSelItems.empty());
    bool foundCopy = false, foundSearch = false;
    for (const auto& item : textSelItems) {
        if (item.id == ui::menu::CommandIds::TextCopy) foundCopy = true;
        if (item.id == ui::menu::CommandIds::TextSearch) foundSearch = true;
    }
    ASSERT(foundCopy);
    ASSERT(foundSearch);

    // 2. Annotation context
    ui::menu::ContextMenuInfo annotInfo;
    annotInfo.targetType = ui::menu::TargetType::Annotation;
    auto annotItems = mgr.BuildMenuItems(annotInfo);
    bool foundDeleteAnnot = false, foundProps = false;
    for (const auto& item : annotItems) {
        if (item.id == ui::menu::CommandIds::AnnotDelete) foundDeleteAnnot = true;
        if (item.id == ui::menu::CommandIds::AnnotProperties) foundProps = true;
    }
    ASSERT(foundDeleteAnnot);
    ASSERT(foundProps);

    // 3. Image context
    ui::menu::ContextMenuInfo imgInfo;
    imgInfo.targetType = ui::menu::TargetType::ImageObject;
    auto imgItems = mgr.BuildMenuItems(imgInfo);
    bool foundExtract = false, foundReplace = false;
    for (const auto& item : imgItems) {
        if (item.id == ui::menu::CommandIds::ImageExtract) foundExtract = true;
        if (item.id == ui::menu::CommandIds::ImageReplace) foundReplace = true;
    }
    ASSERT(foundExtract);
    ASSERT(foundReplace);

    // 4. Customizer test
    bool customizerRan = false;
    mgr.SetCustomizer(ui::menu::TargetType::PageCanvas, [&](std::vector<ui::menu::MenuItem>& items, const ui::menu::ContextMenuInfo&) {
        customizerRan = true;
        items.push_back(ui::menu::MenuItem::Action(9999, L"Custom Action"));
    });

    ui::menu::ContextMenuInfo canvasInfo;
    canvasInfo.targetType = ui::menu::TargetType::PageCanvas;
    auto canvasItems = mgr.BuildMenuItems(canvasInfo);
    ASSERT(customizerRan);
    bool foundCustom = false;
    for (const auto& item : canvasItems) {
        if (item.id == 9999) foundCustom = true;
    }
    ASSERT(foundCustom);
    mgr.ClearCustomizer(ui::menu::TargetType::PageCanvas);
}

void TestSearchHighlightOverlay() {
    ui::search::SearchHighlightOverlay overlay;
    ASSERT(overlay.GetResultCount() == 0);
    ASSERT(overlay.GetActiveIndex() == -1);

    std::vector<core::models::SearchResult> results = {
        { 0, 10, 5 },
        { 0, 50, 5 },
        { 1, 20, 5 }
    };

    overlay.SetResults(results, 0);
    ASSERT(overlay.GetResultCount() == 3);
    ASSERT(overlay.GetActiveIndex() == 0);
    ASSERT(overlay.GetActiveResult() != nullptr);
    ASSERT(overlay.GetActiveResult()->charIndex == 10);

    overlay.SetActiveIndex(2);
    ASSERT(overlay.GetActiveIndex() == 2);
    ASSERT(overlay.GetActiveResult()->pageIndex == 1);

    // Mock TextPage and converter for AutoScroll test
    class MockTextPage : public core::interfaces::dom::ITextPage {
    public:
        int GetCharCount() const override { return 100; }
        std::wstring GetText(int, int) const override { return L"test"; }
        RectF GetCharBox(int) const override { return { 0.0f, 0.0f, 10.0f, 10.0f }; }
        int GetCharIndexAtPos(double, double, double, double) const override { return 0; }
        std::vector<RectF> GetRects(int start, int) const override {
            if (start == 10) return { { 50.0f, 100.0f, 150.0f, 120.0f } };
            if (start == 50) return { { 50.0f, 800.0f, 150.0f, 820.0f } };
            return {};
        }
    };

    MockTextPage mockTp;
    auto textPageProvider = [&](int) -> core::interfaces::dom::ITextPage* {
        return &mockTp;
    };

    auto pageToView = [](double px, double py, int, double& vx, double& vy) {
        vx = px;
        vy = py;
    };

    // Active result is at charIndex 10 -> Y in [100, 120]
    overlay.SetActiveIndex(0);
    // If viewport height is 600 and current scroll is 0, match at 100 is visible (no scroll needed)
    auto scrollRes1 = overlay.CalculateAutoScroll(600.0f, 0.0f, textPageProvider, pageToView);
    ASSERT(!scrollRes1.shouldScroll);

    // Active result is at charIndex 50 -> Y in [800, 820]
    overlay.SetActiveIndex(1);
    // Match at 800 is below viewport (0..600), so auto-scroll is required!
    auto scrollRes2 = overlay.CalculateAutoScroll(600.0f, 0.0f, textPageProvider, pageToView);
    ASSERT(scrollRes2.shouldScroll);
    // Ideal centered scroll = 800 - (600 - 20)/2 = 800 - 290 = 510
    ASSERT(scrollRes2.newScrollY >= 500.0f && scrollRes2.newScrollY <= 520.0f);

    overlay.Clear();
    ASSERT(overlay.GetResultCount() == 0);
    ASSERT(overlay.GetActiveIndex() == -1);
}

int main() {
    std::cout << "Running CommandStackTests...\n";
    TestCommandStackWithPdfium();
    TestAddInkAnnotation();
    TestCommandStackGenerations();
    TestCommandStackMemoryBounding();
    TestCommandStackMaxDepth();
    TestCommandCoalescing();
    TestMacroCommandAtomicRollback();
    TestSaveStateTracking();
    TestContextMenuManager();
    TestSearchHighlightOverlay();
    std::cout << "All Command Stack, ContextMenu, and Search tests passed successfully!\n";
    return 0;
}
