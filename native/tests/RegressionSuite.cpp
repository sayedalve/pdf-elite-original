#include "TestFramework.h"
#include <fstream>
#include <filesystem>
#include <future>
#include <fpdfview.h>
#include "../src/pdf_engine/src/PdfiumLibrary.h"
#include "../src/pdf_engine/src/PdfDocument.h"
#include "../src/pdf_engine/src/PdfPage.h"
#include "../src/pdf_engine/src/SearchEngine.h"
#include "../src/ui/src/ThemeManager.h"
#include "../src/ui/src/interaction/InteractionManager.h"
#include "../src/core/interfaces/dom/CommandStack.h"
#include "../src/core/interfaces/dom/ICommand.h"
#include "../src/pdf_engine/src/commands/PageCommands.h"
#include "../src/pdf_engine/src/commands/ImageCommands.h"
#include "../src/pdf_engine/src/commands/TextCommands.h"
#include "../src/pdf_engine/src/commands/AnnotationCommands.h"
#include "../src/ui/src/interaction/AnnotationSelectableObject.h"
#include "../src/ui/src/annotation/IAnnotationHandler.h"
#include "../src/ui/src/annotation/AnnotationHandlerFactory.h"
#include "../src/ui/src/annotation/TextAnnotationHandler.h"
#include "../src/ui/src/annotation/HighlightAnnotationHandler.h"
#include "../src/ui/src/annotation/InkAnnotationHandler.h"
#include "../src/ui/src/annotation/ShapeAnnotationHandler.h"
#include "../src/ui/src/annotation/FreeTextAnnotationHandler.h"

void create_test_file(const wchar_t* path, const char* data, size_t size) {
    std::ofstream out(path, std::ios::binary);
    if (size > 0) out.write(data, size);
}

// Valid minimal PDF
const char minimal_pdf[] = 
    "%PDF-1.4\n"
    "1 0 obj <</Type/Catalog/Pages 2 0 R>> endobj\n"
    "2 0 obj <</Type/Pages/Count 1/Kids[3 0 R]>> endobj\n"
    "3 0 obj <</Type/Page/Parent 2 0 R/MediaBox[0 0 612 792]/Contents 4 0 R/Resources<<>>>> endobj\n"
    "4 0 obj <</Length 0>> stream\nendstream\nendobj\n"
    "xref\n0 5\n0000000000 65535 f \n0000000009 00000 n \n0000000052 00000 n \n0000000101 00000 n \n0000000193 00000 n \n"
    "trailer <</Size 5/Root 1 0 R>>\nstartxref\n233\n%%EOF";

// Setup function
void SetupFixtures() {
    std::error_code ec;
    std::filesystem::create_directories("tests/fixtures/malformed", ec);
    std::filesystem::create_directories("tests/fixtures/basic", ec);
    std::filesystem::create_directories("tests/fixtures/text", ec);
    std::filesystem::create_directories("tests/fixtures/unicode", ec);
    std::filesystem::create_directories("tests/fixtures/output", ec);

    create_test_file(L"tests/fixtures/malformed/empty.pdf", "", 0);
    create_test_file(L"tests/fixtures/malformed/truncated.pdf", "%PDF-", 5);
    
    const char garbage[] = "%PDF-1.4\n%äüöß\n1 0 obj\n<garbage here\n";
    create_test_file(L"tests/fixtures/malformed/corrupt.pdf", garbage, sizeof(garbage)-1);

    create_test_file(L"tests/fixtures/basic/minimal.pdf", minimal_pdf, sizeof(minimal_pdf)-1);

    if (std::filesystem::exists("native/tests/basic_text.pdf") && !std::filesystem::exists("tests/fixtures/text/basic_text.pdf")) {
        std::filesystem::copy_file("native/tests/basic_text.pdf", "tests/fixtures/text/basic_text.pdf", std::filesystem::copy_options::overwrite_existing, ec);
    }
    if (std::filesystem::exists("native/tests/unicode_prototype.pdf") && !std::filesystem::exists("tests/fixtures/unicode/unicode_text.pdf")) {
        std::filesystem::copy_file("native/tests/unicode_prototype.pdf", "tests/fixtures/unicode/unicode_text.pdf", std::filesystem::copy_options::overwrite_existing, ec);
    }
}

// ---------------------------------------------------------
// Security Tests (Malformed PDFs)
// ---------------------------------------------------------

TEST(Security_EmptyFile_HandledSafely) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/malformed/empty.pdf");
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ((int)res.error, (int)ErrorCode::InvalidFormat);
}

TEST(Security_TruncatedFile_HandledSafely) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/malformed/truncated.pdf");
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ((int)res.error, (int)ErrorCode::InvalidFormat);
}

TEST(Security_CorruptFile_HandledSafely) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/malformed/corrupt.pdf");
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ((int)res.error, (int)ErrorCode::InvalidFormat);
}

// ---------------------------------------------------------
// Core PDF Loading
// ---------------------------------------------------------

TEST(Core_LoadMinimalPDF) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    EXPECT_EQ(doc->PageCount(), 1);
}

TEST(Core_SaveAsAndReopen) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    
    const wchar_t* out_path = L"tests/fixtures/basic/minimal_saved.pdf";
    bool saved = res.value->SaveAs(out_path);
    EXPECT_TRUE(saved);

    auto reopen_res = PdfDocument::LoadFromFile(out_path);
    EXPECT_TRUE(reopen_res.has_value());
    EXPECT_EQ(reopen_res.value->PageCount(), 1);
}

TEST(Core_SaveAsFailure) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    // Try to save to an invalid drive or path
    bool saved = res.value->SaveAs(L"Z:\\invalid_folder_that_does_not_exist\\test.pdf");
    EXPECT_FALSE(saved);
}

TEST(Core_CommandStackDirtyState) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    
    auto doc = std::move(res.value);
    auto& stack = doc->GetCommandStack();
    
    // Initially not dirty
    EXPECT_FALSE(stack.IsDirty());
    
    // Execute a dummy command to make it dirty
    struct DummyCommand : public core::interfaces::dom::ICommand {
    public:
        bool Execute() override { return true; }
        bool Undo() override { return true; }
        std::string GetDescription() const override { return "Dummy"; }
    };
    
    stack.ExecuteCommand(std::make_unique<DummyCommand>());
    EXPECT_TRUE(stack.IsDirty());
    
    // Mark saved
    stack.MarkSaved();
    EXPECT_FALSE(stack.IsDirty());
    
    // Undo should make it dirty again
    stack.Undo();
    EXPECT_TRUE(stack.IsDirty());
    
    // Redo should make it clean again (since it returns to the saved state)
    stack.Redo();
    EXPECT_FALSE(stack.IsDirty());
}


// ---------------------------------------------------------
// Page Operations
// ---------------------------------------------------------

TEST(Page_ExtractPageSize) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto page = res.value->GetPage(0);
    EXPECT_TRUE(page != nullptr);
    
    auto size = page->GetSize();
    EXPECT_EQ((int)size.width, 612);
    EXPECT_EQ((int)size.height, 792);
}

TEST(Page_Rotation) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto page = res.value->GetPage(0);
    EXPECT_TRUE(page != nullptr);
    
    EXPECT_EQ(page->GetRotation(), 0);
    
    page->SetRotation(90);
    EXPECT_EQ(page->GetRotation(), 90);
}

TEST(Page_InsertBlankPage) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    
    EXPECT_EQ(doc->PageCount(), 1);
    EXPECT_TRUE(doc->InsertBlankPage(1, 612, 792));
    EXPECT_EQ(doc->PageCount(), 2);
    
    EXPECT_TRUE(doc->SaveAs(L"tests/fixtures/basic/inserted.pdf"));
}

TEST(Page_DeletePage) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/inserted.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    
    EXPECT_EQ(doc->PageCount(), 2);
    EXPECT_TRUE(doc->DeletePage(1));
    EXPECT_EQ(doc->PageCount(), 1);
    
    // Deleting last page should fail
    EXPECT_FALSE(doc->DeletePage(0));
}

TEST(Page_DuplicatePage) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    
    EXPECT_EQ(doc->PageCount(), 1);
    EXPECT_TRUE(doc->DuplicatePage(0));
    EXPECT_EQ(doc->PageCount(), 2);
}

TEST(Page_MovePage) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    
    EXPECT_TRUE(doc->InsertBlankPage(1, 800, 600));
    EXPECT_EQ(doc->PageCount(), 2);
    
    // Page 0 is 612x792, Page 1 is 800x600
    EXPECT_TRUE(doc->MovePage(1, 0));
    // Now Page 0 should be 800x600
    auto size = doc->GetPageSize(0);
    EXPECT_EQ((int)size.width, 800);
}

TEST(Page_RotatePageAPI) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    
    EXPECT_TRUE(doc->RotatePage(0, 90));
    auto page = doc->GetPage(0);
    EXPECT_EQ(page->GetRotation(), 90);
}

TEST(Document_ExtractPages) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    
    EXPECT_TRUE(doc->InsertBlankPage(1, 800, 600));
    EXPECT_TRUE(doc->InsertBlankPage(2, 400, 300));
    
    std::vector<int> extractIndices = { 0, 2 };
    auto extracted = doc->ExtractPages(extractIndices);
    EXPECT_TRUE(extracted != nullptr);
    EXPECT_EQ(extracted->PageCount(), 2);
    auto size = extracted->GetPageSize(1); // was index 2
    EXPECT_EQ((int)size.width, 400);
}

TEST(Document_MergeInsertPagesFrom) {
    auto res1 = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    auto doc1 = std::move(res1.value);
    
    auto res2 = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    auto doc2 = std::move(res2.value);
    
    std::vector<int> srcIndices = { 0 };
    EXPECT_TRUE(doc1->InsertPagesFrom(doc2.get(), srcIndices, 1));
    EXPECT_EQ(doc1->PageCount(), 2);
}

// ---------------------------------------------------------
// Performance Benchmarks
// ---------------------------------------------------------

TEST(Perf_PdfOpen) {
    for (int i = 0; i < 10; ++i) {
        auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
        EXPECT_TRUE(res.has_value());
    }
}

TEST(Perf_Save) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    const wchar_t* out_path = L"tests/fixtures/basic/perf_save.pdf";
    for (int i = 0; i < 5; ++i) {
        bool saved = res.value->SaveAs(out_path);
        EXPECT_TRUE(saved);
    }
}

// ---------------------------------------------------------
// Text Extraction
// ---------------------------------------------------------

TEST(Text_ExtractBasicText) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/text/basic_text.pdf");
    EXPECT_TRUE(res.has_value());
    auto page = res.value->GetPage(0);
    EXPECT_TRUE(page != nullptr);
    
    auto textPage = page->LoadTextPage();
    EXPECT_TRUE(textPage != nullptr);
    
    int charCount = textPage->GetCharCount();
    EXPECT_TRUE(charCount > 0);
    
    std::wstring text = textPage->GetText(0, charCount);
    EXPECT_TRUE(text.find(L"Hello PDF Elite") != std::wstring::npos);
    EXPECT_TRUE(text.find(L"1234567890") != std::wstring::npos);
}

TEST(Text_ExtractUnicodeText) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/unicode/unicode_text.pdf");
    EXPECT_TRUE(res.has_value());
    auto page = res.value->GetPage(0);
    
    auto textPage = page->LoadTextPage();
    int charCount = textPage->GetCharCount();
    EXPECT_TRUE(charCount > 0);
}

// ---------------------------------------------------------
// Text Editing Prototype (Phase 15A)
// ---------------------------------------------------------

TEST(Text_EditBasicText) {
    // 1. Open basic_text.pdf
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/text/basic_text.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    auto page = doc->GetPage(0);
    EXPECT_TRUE(page != nullptr);
    
    // 2. Enumerate text objects
    auto textObjs = page->GetTextObjects();
    EXPECT_TRUE(!textObjs.empty());
    
    // 3. Find "Hello World"
    std::shared_ptr<core::interfaces::dom::ITextObject> targetObj = nullptr;
    for (auto& obj : textObjs) {
        std::wstring text = obj->GetText();
        if (text.find(L"Hello World") != std::wstring::npos || text.find(L"Hello") != std::wstring::npos) {
            targetObj = obj;
            break;
        }
    }
    
    EXPECT_TRUE(targetObj != nullptr);
    if (!targetObj) return;
    
    // 4. Modify text
    std::wstring oldText = targetObj->GetText();
    std::wcout << L"Found text: " << oldText << L"\n";
    
    bool changed = targetObj->SetText(L"Hello PDF Elite");
    EXPECT_TRUE(changed);
    
    // 5. Verify text immediately
    std::wstring newText = targetObj->GetText();
    EXPECT_TRUE(newText == L"Hello PDF Elite");
    
    // 6. Save
    const wchar_t* out_path = L"tests/fixtures/text/basic_text_edited.pdf";
    bool saved = doc->SaveAs(out_path);
    EXPECT_TRUE(saved);
    
    // 7. Reopen and verify
    auto res2 = PdfDocument::LoadFromFile(out_path);
    EXPECT_TRUE(res2.has_value());
    auto doc2 = std::move(res2.value);
    auto page2 = doc2->GetPage(0);
    
    auto textObjs2 = page2->GetTextObjects();
    bool foundNewText = false;
    bool foundOldText = false;
    for (auto& obj : textObjs2) {
        std::wstring text = obj->GetText();
        if (text.find(L"Hello PDF Elite") != std::wstring::npos) {
            foundNewText = true;
        }
        if (text.find(L"Hello World") != std::wstring::npos) {
            foundOldText = true;
        }
    }
    
    EXPECT_TRUE(foundNewText);
    EXPECT_FALSE(foundOldText);
    
    // Also verify with TextPage (searchability)
    auto textPage2 = page2->LoadTextPage();
    std::wstring fullText = textPage2->GetText(0, textPage2->GetCharCount());
    EXPECT_FALSE(fullText.find(L"Hello World") != std::wstring::npos);
}


// ---------------------------------------------------------
// Undo / Redo Command Stack
// ---------------------------------------------------------

TEST(Command_BasicUndoRedo) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    
    auto& stack = doc->GetCommandStack();
    EXPECT_EQ(doc->PageCount(), 1);
    
    auto insertCmd = std::make_unique<pdf_engine::commands::InsertBlankPageCommand>(doc.get(), 1, 612, 792);
    EXPECT_TRUE(stack.ExecuteCommand(std::move(insertCmd)));
    EXPECT_EQ(doc->PageCount(), 2);
    EXPECT_TRUE(stack.IsDirty());
    
    EXPECT_TRUE(stack.Undo());
    EXPECT_EQ(doc->PageCount(), 1);
    EXPECT_FALSE(stack.IsDirty()); // Undo back to save state
    
    EXPECT_TRUE(stack.Redo());
    EXPECT_EQ(doc->PageCount(), 2);
    EXPECT_TRUE(stack.IsDirty());
}

TEST(Command_RedoInvalidation) {
    core::interfaces::dom::CommandStack stack;
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    auto doc = std::move(res.value);
    
    auto cmd1 = std::make_unique<pdf_engine::commands::InsertBlankPageCommand>(doc.get(), 1, 100, 100);
    auto cmd2 = std::make_unique<pdf_engine::commands::InsertBlankPageCommand>(doc.get(), 2, 200, 200);
    auto cmd3 = std::make_unique<pdf_engine::commands::InsertBlankPageCommand>(doc.get(), 1, 300, 300);
    
    stack.ExecuteCommand(std::move(cmd1));
    stack.ExecuteCommand(std::move(cmd2));
    EXPECT_EQ(doc->PageCount(), 3);
    
    stack.Undo();
    EXPECT_EQ(doc->PageCount(), 2);
    EXPECT_TRUE(stack.CanRedo());
    
    // Execute cmd3 should invalidate cmd2 redo
    stack.ExecuteCommand(std::move(cmd3));
    EXPECT_FALSE(stack.CanRedo());
    EXPECT_EQ(doc->PageCount(), 3);
    auto size = doc->GetPageSize(1);
    EXPECT_EQ((int)size.width, 300);
}

// ---------------------------------------------------------
// Search Engine
// ---------------------------------------------------------

TEST(Search_BasicMatch) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/text/basic_text.pdf");
    EXPECT_TRUE(res.has_value());
    
    std::shared_ptr<core::interfaces::dom::IDocument> sharedDoc = std::move(res.value);
    pdf_engine::SearchEngine search(sharedDoc);
    std::promise<std::vector<pdf_engine::SearchResult>> prom;
    search.SearchAsync(L"Hello", false, false, [&](const std::vector<pdf_engine::SearchResult>& r) {
        prom.set_value(r);
    });
    
    auto results = prom.get_future().get();
    EXPECT_TRUE(results.size() > 0);
    EXPECT_EQ(results[0].pageIndex, 0);
}

TEST(Search_CaseSensitivity) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/text/basic_text.pdf");
    EXPECT_TRUE(res.has_value());
    
    std::shared_ptr<core::interfaces::dom::IDocument> sharedDoc = std::move(res.value);
    pdf_engine::SearchEngine search(sharedDoc);
    
    std::promise<std::vector<pdf_engine::SearchResult>> prom1;
    search.SearchAsync(L"hello", true, false, [&](const std::vector<pdf_engine::SearchResult>& r) {
        prom1.set_value(r);
    });
    auto resultsCaseSensitive = prom1.get_future().get();
    EXPECT_EQ((int)resultsCaseSensitive.size(), 0); // "Hello" vs "hello"
    
    std::promise<std::vector<pdf_engine::SearchResult>> prom2;
    search.SearchAsync(L"hello", false, false, [&](const std::vector<pdf_engine::SearchResult>& r) {
        prom2.set_value(r);
    });
    auto resultsCaseInsensitive = prom2.get_future().get();
    EXPECT_TRUE(resultsCaseInsensitive.size() > 0);
}

// ---------------------------------------------------------
// Registration of not-yet-implemented features
// ---------------------------------------------------------

// ---------------------------------------------------------
// Hot Reload Tests
// ---------------------------------------------------------

TEST(HotReload_ThemeValid) {
    std::error_code ec;
    std::filesystem::create_directories("tests/fixtures/basic", ec);
    // Write valid theme first so LoadTheme() on StartWatching() picks it up immediately
    const char theme1[] = "{\"bgPrimary\": \"#FF0000\"}";
    create_test_file(L"tests/fixtures/basic/theme.json", theme1, sizeof(theme1)-1);

    ThemeManager::Instance().SetDevMode(true);
    ThemeManager::Instance().Initialize(L"tests/fixtures/basic");
    ThemeManager::Instance().StartWatching();
    
    Sleep(100);
    
    auto colors = ThemeManager::Instance().GetColors();
    EXPECT_EQ(colors.bgPrimary.r, 1.0f);
    EXPECT_EQ(colors.bgPrimary.g, 0.0f);
    EXPECT_EQ(colors.bgPrimary.b, 0.0f);
    
    ThemeManager::Instance().StopWatching();
}

TEST(HotReload_ThemeInvalid) {
    ThemeManager::Instance().SetDevMode(true);
    ThemeManager::Instance().Initialize(L"tests/fixtures/basic");
    ThemeManager::Instance().StartWatching();

    // Write invalid theme
    const char theme_invalid[] = "{\"bgPrimary\": \"#ZZZZZZ\"}";
    create_test_file(L"tests/fixtures/basic/theme.json", theme_invalid, sizeof(theme_invalid)-1);
    
    Sleep(200); 
    
    // Colors should remain unchanged
    auto colors = ThemeManager::Instance().GetColors();
    EXPECT_EQ(colors.bgPrimary.r, 1.0f); // Remains the previous value

    ThemeManager::Instance().StopWatching();
}
class TestMockObject : public ui::interaction::ISelectableObject {
public:
    TestMockObject(std::string id, ui::interaction::Rect bounds) : m_id(std::move(id)), m_bounds(bounds) {}
    std::string GetId() const override { return m_id; }
    int GetPageIndex() const override { return 0; }
    ui::interaction::Rect GetBounds() const override { return m_bounds; }
    void SetBounds(const ui::interaction::Rect& bounds) override { m_bounds = bounds; }
    double GetRotation() const override { return 0; }
    void SetRotation(double degrees) override { (void)degrees; }
private:
    std::string m_id;
    ui::interaction::Rect m_bounds;
};

TEST(Interaction_Selection_Basic) {
    ui::interaction::SelectionModel sm;
    auto obj1 = std::make_shared<TestMockObject>("1", ui::interaction::Rect{0,0,10,10});
    auto obj2 = std::make_shared<TestMockObject>("2", ui::interaction::Rect{20,20,30,30});
    
    sm.Select(obj1);
    EXPECT_TRUE(sm.IsSelected("1"));
    EXPECT_FALSE(sm.IsSelected("2"));
    
    sm.ToggleSelect(obj2);
    EXPECT_TRUE(sm.IsSelected("1"));
    EXPECT_TRUE(sm.IsSelected("2"));
    
    sm.ToggleSelect(obj1);
    EXPECT_FALSE(sm.IsSelected("1"));
    EXPECT_TRUE(sm.IsSelected("2"));
    
    sm.Clear();
    EXPECT_TRUE(sm.GetSelected().empty());
}

TEST(Interaction_Selection_Marquee) {
    ui::interaction::InteractionManager im;
    std::vector<std::shared_ptr<ui::interaction::ISelectableObject>> mocks;
    mocks.push_back(std::make_shared<TestMockObject>("1", ui::interaction::Rect{10, 10, 50, 50}));
    mocks.push_back(std::make_shared<TestMockObject>("2", ui::interaction::Rect{100, 100, 150, 150}));
    im.SetObjects(mocks);
    
    // Set identity transform for testing
    im.pageToView = [](double px, double py, int, double& vx, double& vy) { vx = px; vy = py; };
    im.viewToPage = [](double vx, double vy, double& px, double& py, int& p) { px = vx; py = vy; p = 0; };
    
    // Simulate drag from (0,0) to (60,60)
    im.StartMarquee(0, 0, false);
    im.OnMouseMove(60, 60);
    im.OnLButtonUp(60, 60);
    
    EXPECT_TRUE(im.GetSelectionModel().IsSelected("1"));
    EXPECT_FALSE(im.GetSelectionModel().IsSelected("2"));
}

TEST(Interaction_Selection_TransformGeometry) {
    ui::interaction::InteractionManager im;
    std::vector<std::shared_ptr<ui::interaction::ISelectableObject>> mocks;
    auto obj = std::make_shared<TestMockObject>("1", ui::interaction::Rect{10, 10, 50, 50});
    mocks.push_back(obj);
    im.SetObjects(mocks);
    
    im.pageToView = [](double px, double py, int, double& vx, double& vy) { vx = px; vy = py; };
    im.viewToPage = [](double vx, double vy, double& px, double& py, int& p) { px = vx; py = vy; p = 0; };
    
    // Click on object to select and start drag
    im.OnLButtonDown(20, 20, false);
    EXPECT_TRUE(im.GetSelectionModel().IsSelected("1"));
    
    // Drag by +10, +20
    im.OnMouseMove(30, 40);
    im.OnLButtonUp(30, 40);
    
    auto bounds = obj->GetBounds();
    EXPECT_EQ(bounds.left, 20.0);
    EXPECT_EQ(bounds.top, 30.0);
    EXPECT_EQ(bounds.right, 60.0);
    EXPECT_EQ(bounds.bottom, 70.0);
}

TEST(Annotation_AddTextMarkups) {
    auto docResult = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    auto doc = std::move(docResult.value);
    EXPECT_TRUE(doc != nullptr);
    
    auto page = doc->GetPage(0);
    EXPECT_TRUE(page != nullptr);
    
    auto annots = page->GetAnnotations();
    size_t initialCount = annots.size();
    
    auto highlight = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    EXPECT_TRUE(highlight != nullptr);
    highlight->SetBounds({100, 200, 300, 400});
    
    std::vector<QuadF> quads;
    quads.push_back({{100,200}, {300,200}, {100,400}, {300,400}});
    highlight->SetQuadPoints(quads);
    
    auto underline = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Underline);
    EXPECT_TRUE(underline != nullptr);
    
    auto strikeout = page->CreateAnnotation(core::interfaces::dom::AnnotationType::StrikeOut);
    EXPECT_TRUE(strikeout != nullptr);
    
    auto finalAnnots = page->GetAnnotations();
    EXPECT_EQ(finalAnnots.size(), initialCount + 3);
    
    EXPECT_TRUE(doc->SaveAs(L"tests/fixtures/basic/annotated.pdf"));
}

TEST(Annotation_SaveReopen) {
    auto docResult = PdfDocument::LoadFromFile(L"tests/fixtures/basic/annotated.pdf");
    auto doc = std::move(docResult.value);
    EXPECT_TRUE(doc != nullptr);
    
    auto page = doc->GetPage(0);
    EXPECT_TRUE(page != nullptr);
    
    auto annots = page->GetAnnotations();
    bool foundHighlight = false;
    for (auto& a : annots) {
        if (a->GetType() == core::interfaces::dom::AnnotationType::Highlight) {
            foundHighlight = true;
            auto quads = a->GetQuadPoints();
            EXPECT_TRUE(quads.size() > 0);
            auto bounds = a->GetBounds();
            EXPECT_EQ((int)bounds.left, 100);
            EXPECT_EQ((int)bounds.top, 200);
            EXPECT_EQ((int)bounds.right, 300);
            EXPECT_EQ((int)bounds.bottom, 400);
        }
    }
    EXPECT_TRUE(foundHighlight);
}

TEST(Annotation_AddFreeTextAndNote) {
    auto docResult = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    auto doc = std::move(docResult.value);
    EXPECT_TRUE(doc != nullptr);
    
    auto page = doc->GetPage(0);
    EXPECT_TRUE(page != nullptr);
    
    auto ft = page->CreateAnnotation(core::interfaces::dom::AnnotationType::FreeText);
    EXPECT_TRUE(ft != nullptr);
    ft->SetBounds({50, 50, 200, 100});
    ft->SetContents("Hello FreeText!");
    
    auto note = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Text); // Text = Sticky Note
    EXPECT_TRUE(note != nullptr);
    note->SetBounds({10, 10, 30, 30});
    note->SetContents("This is a sticky note.");
    
    EXPECT_TRUE(doc->SaveAs(L"tests/fixtures/basic/annotated_freetext.pdf"));
}

TEST(Annotation_SaveReopenFreeText) {
    auto docResult = PdfDocument::LoadFromFile(L"tests/fixtures/basic/annotated_freetext.pdf");
    auto doc = std::move(docResult.value);
    EXPECT_TRUE(doc != nullptr);
    
    auto page = doc->GetPage(0);
    EXPECT_TRUE(page != nullptr);
    
    auto annots = page->GetAnnotations();
    bool foundFT = false;
    bool foundNote = false;
    for (auto& a : annots) {
        if (a->GetType() == core::interfaces::dom::AnnotationType::FreeText) {
            foundFT = true;
            EXPECT_EQ(a->GetContents(), "Hello FreeText!");
            auto bounds = a->GetBounds();
            EXPECT_EQ((int)bounds.left, 50);
        } else if (a->GetType() == core::interfaces::dom::AnnotationType::Text) {
            foundNote = true;
            EXPECT_EQ(a->GetContents(), "This is a sticky note.");
        }
    }
    EXPECT_TRUE(foundFT);
    EXPECT_TRUE(foundNote);
}

TEST(Annotation_Modify) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    auto page = doc->GetPage(0);
    EXPECT_TRUE(page != nullptr);

    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Highlight);
    EXPECT_TRUE(annot != nullptr);
    
    // Modify Bounds
    annot->SetBounds({ 50.0f, 50.0f, 150.0f, 150.0f });
    auto bounds = annot->GetBounds();
    EXPECT_EQ(bounds.left, 50.0f);
    EXPECT_EQ(bounds.right, 150.0f);

    // Modify Color
    annot->SetColor(255, 0, 0, 255);
}

TEST(Annotation_ExtendedProperties) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    auto page = doc->GetPage(0);
    EXPECT_TRUE(page != nullptr);

    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Square);
    EXPECT_TRUE(annot != nullptr);

    // Test Color
    annot->SetColor(255, 128, 64, 200);
    int r, g, b, a;
    EXPECT_TRUE(annot->GetColor(r, g, b, a));
    EXPECT_EQ(r, 255);
    EXPECT_EQ(g, 128);
    EXPECT_EQ(b, 64);
    EXPECT_EQ(a, 200);

    // Test FillColor
    annot->SetFillColor(10, 20, 30, 40);
    EXPECT_TRUE(annot->GetFillColor(r, g, b, a));
    EXPECT_EQ(r, 10);
    EXPECT_EQ(g, 20);
    EXPECT_EQ(b, 30);
    EXPECT_EQ(a, 40);

    // Test Author
    annot->SetAuthor("PDF Elite User");
    EXPECT_EQ(annot->GetAuthor(), "PDF Elite User");

    // Test Dates
    annot->SetCreationDate("D:20260826120000Z");
    EXPECT_EQ(annot->GetCreationDate(), "D:20260826120000Z");
    annot->SetModificationDate("D:20260826120500Z");
    EXPECT_EQ(annot->GetModificationDate(), "D:20260826120500Z");

    // Test Flags (e.g. 4 = Print, 8 = NoZoom)
    annot->SetFlags(12);
    EXPECT_EQ(annot->GetFlags(), 12);
    
    // Test BorderWidth
    annot->SetBorderWidth(3.5f);
    EXPECT_EQ(annot->GetBorderWidth(), 3.5f);
    
    auto& stack = doc->GetCommandStack();
    std::unique_ptr<core::interfaces::dom::ICommand> delCmd = std::make_unique<pdf_engine::commands::DeleteAnnotationCommand>(doc.get(), 0, annot);
    stack.ExecuteCommand(std::move(delCmd));
    
    // Annotation should be removed
    EXPECT_EQ(page->GetAnnotations().size(), 0);
    
    stack.Undo();
    // Annotation should be restored
    auto annots = page->GetAnnotations();
    EXPECT_EQ(annots.size(), 1);
    auto restored = annots[0];
    
    EXPECT_EQ(restored->GetAuthor(), "PDF Elite User");
    EXPECT_EQ(restored->GetCreationDate(), "D:20260826120000Z");
    EXPECT_EQ(restored->GetFlags(), 12);
    EXPECT_TRUE(restored->GetColor(r, g, b, a));
    EXPECT_EQ(r, 255);
    EXPECT_EQ(a, 200);
}

TEST(Annotation_Delete) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    auto page = doc->GetPage(0);
    EXPECT_TRUE(page != nullptr);

    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Underline);
    EXPECT_TRUE(annot != nullptr);
    
    size_t countBefore = page->GetAnnotations().size();
    
    bool removed = page->RemoveAnnotation(annot);
    EXPECT_TRUE(removed);
    
    size_t countAfter = page->GetAnnotations().size();
    EXPECT_EQ(countAfter, countBefore - 1);
}

TEST(Annotation_Ink_Strokes_And_Commands) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    auto page = doc->GetPage(0);
    EXPECT_TRUE(page != nullptr);

    std::vector<std::vector<PointF>> strokes = {
        { {10.0f, 10.0f}, {20.0f, 25.0f}, {30.0f, 30.0f} },
        { {50.0f, 50.0f}, {60.0f, 75.0f} }
    };
    RectF bounds{10.0f, 10.0f, 60.0f, 75.0f};

    auto& stack = doc->GetCommandStack();
    auto cmd = std::make_unique<pdf_engine::commands::AddInkAnnotationCommand>(
        doc.get(), 0, strokes, bounds, 0, 128, 255, 200, 3.0f
    );
    auto* rawCmd = cmd.get();
    bool executed = stack.ExecuteCommand(std::move(cmd));
    EXPECT_TRUE(executed);

    auto annot = rawCmd->GetAnnotation();
    EXPECT_TRUE(annot != nullptr);
    EXPECT_EQ(static_cast<int>(annot->GetType()), static_cast<int>(core::interfaces::dom::AnnotationType::Ink));

    auto fetchedStrokes = annot->GetInkList();
    EXPECT_EQ(fetchedStrokes.size(), 2);
    if (fetchedStrokes.size() == 2) {
        EXPECT_EQ(fetchedStrokes[0].size(), 3);
        EXPECT_EQ(fetchedStrokes[1].size(), 2);
    }

    // Delete and undo test
    auto delCmd = std::make_unique<pdf_engine::commands::DeleteAnnotationCommand>(doc.get(), 0, annot);
    stack.ExecuteCommand(std::move(delCmd));
    EXPECT_EQ(page->GetAnnotations().size(), 0);

    stack.Undo();
    auto restoredAnnots = page->GetAnnotations();
    EXPECT_EQ(restoredAnnots.size(), 1);
    if (!restoredAnnots.empty()) {
        auto restoredInks = restoredAnnots[0]->GetInkList();
        EXPECT_EQ(restoredInks.size(), 2);
        int r, g, b, a;
        EXPECT_TRUE(restoredAnnots[0]->GetColor(r, g, b, a));
        EXPECT_EQ(g, 128);
        EXPECT_EQ(b, 255);
    }
}

TEST(Annotation_Commands_Resize_And_ModifyProperties) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    auto page = doc->GetPage(0);
    EXPECT_TRUE(page != nullptr);

    auto annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Square);
    EXPECT_TRUE(annot != nullptr);
    RectF origBounds{50.0f, 50.0f, 150.0f, 150.0f};
    annot->SetBounds(origBounds);
    annot->SetColor(255, 0, 0, 255);

    auto& stack = doc->GetCommandStack();

    // 1. Resize command
    RectF newBounds{60.0f, 60.0f, 200.0f, 200.0f};
    auto resizeCmd = std::make_unique<pdf_engine::commands::ResizeAnnotationCommand>(annot, origBounds, newBounds);
    stack.ExecuteCommand(std::move(resizeCmd));
    EXPECT_EQ(annot->GetBounds().right, 200.0f);

    stack.Undo();
    EXPECT_EQ(annot->GetBounds().right, 150.0f);

    stack.Redo();
    EXPECT_EQ(annot->GetBounds().right, 200.0f);

    // 2. Modify Properties command
    pdf_engine::commands::AnnotationState oldState;
    oldState.hasColor = annot->GetColor(oldState.colorR, oldState.colorG, oldState.colorB, oldState.colorA);
    oldState.author = annot->GetAuthor();
    oldState.opacity = annot->GetOpacity();

    pdf_engine::commands::AnnotationState newState = oldState;
    newState.hasColor = true;
    newState.colorR = 0; newState.colorG = 255; newState.colorB = 0; newState.colorA = 255;
    newState.author = "Modified Author";
    newState.opacity = 0.5f;

    auto propCmd = std::make_unique<pdf_engine::commands::ModifyAnnotationPropertiesCommand>(annot, oldState, newState);
    stack.ExecuteCommand(std::move(propCmd));
    EXPECT_EQ(annot->GetAuthor(), "Modified Author");
    EXPECT_EQ(annot->GetOpacity(), 0.5f);

    stack.Undo();
    EXPECT_EQ(annot->GetAuthor(), oldState.author);

    stack.Redo();
    EXPECT_EQ(annot->GetAuthor(), "Modified Author");
}

TEST(Annotation_Selectable_Geometric_HitTest) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);
    auto page = doc->GetPage(0);
    EXPECT_TRUE(page != nullptr);

    // Line hit testing
    auto lineAnnot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Line);
    if (lineAnnot) {
        core::interfaces::dom::LineGeometry lg;
        lg.start = {10.0f, 10.0f};
        lg.end = {100.0f, 10.0f};
        lineAnnot->SetLineGeometry(lg);
        lineAnnot->SetBounds({10.0f, 10.0f, 100.0f, 10.0f});

        ui::interaction::AnnotationSelectableObject lineObj(lineAnnot, 0);
        EXPECT_TRUE(lineObj.HitTest(50.0, 10.0, 3.0));
        EXPECT_TRUE(lineObj.HitTest(50.0, 12.0, 3.0));
        EXPECT_FALSE(lineObj.HitTest(50.0, 20.0, 3.0));
    }

    // Circle hit testing
    auto circleAnnot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Circle);
    if (circleAnnot) {
        circleAnnot->SetBounds({100.0f, 100.0f, 200.0f, 200.0f}); // Center = (150, 150), rx=50, ry=50
        ui::interaction::AnnotationSelectableObject circleObj(circleAnnot, 0);
        EXPECT_TRUE(circleObj.HitTest(150.0, 150.0, 2.0)); // Center
        EXPECT_TRUE(circleObj.HitTest(150.0, 195.0, 2.0)); // Inside
        EXPECT_FALSE(circleObj.HitTest(105.0, 105.0, 2.0)); // Corner outside circle
    }

    // Ink hit testing
    auto inkAnnot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Ink);
    if (inkAnnot) {
        inkAnnot->SetBounds({0.0f, 0.0f, 100.0f, 100.0f});
        inkAnnot->AddInkStroke({{0.0f, 0.0f}, {100.0f, 100.0f}});
        ui::interaction::AnnotationSelectableObject inkObj(inkAnnot, 0);
        EXPECT_TRUE(inkObj.HitTest(50.0, 50.0, 2.0)); // On stroke
        EXPECT_FALSE(inkObj.HitTest(50.0, 80.0, 2.0)); // Far from stroke
    }
}

TEST(Annotation_Handlers_StateMachine_And_Factory) {
    ui::annotation::AnnotationHandlerContext ctx;
    ctx.viewToPage = [](double vx, double vy, double& px, double& py, int& pageIndex) {
        px = vx; py = vy; pageIndex = 0;
    };
    ctx.pageToView = [](double px, double py, int, double& vx, double& vy) {
        vx = px; vy = py;
    };

    auto handlers = ui::annotation::AnnotationHandlerFactory::CreateAllHandlers(ctx);
    EXPECT_TRUE(handlers.find(ToolMode::StickyNote) != handlers.end());
    EXPECT_TRUE(handlers.find(ToolMode::Highlight) != handlers.end());
    EXPECT_TRUE(handlers.find(ToolMode::Ink) != handlers.end());
    EXPECT_TRUE(handlers.find(ToolMode::Rectangle) != handlers.end());
    EXPECT_TRUE(handlers.find(ToolMode::Ellipse) != handlers.end());
    EXPECT_TRUE(handlers.find(ToolMode::Line) != handlers.end());
    EXPECT_TRUE(handlers.find(ToolMode::Arrow) != handlers.end());
    EXPECT_TRUE(handlers.find(ToolMode::FreeText) != handlers.end());

    // Test ShapeAnnotationHandler state machine
    auto shapeHandler = handlers[ToolMode::Rectangle];
    EXPECT_TRUE(shapeHandler != nullptr);
    EXPECT_EQ(static_cast<int>(shapeHandler->GetState()), static_cast<int>(ui::annotation::InteractionState::Idle));

    ui::annotation::MouseEvent me;
    me.pageIndex = 0;
    me.pdfX = 10.0; me.pdfY = 10.0;
    shapeHandler->OnMouseDown(me);
    EXPECT_EQ(static_cast<int>(shapeHandler->GetState()), static_cast<int>(ui::annotation::InteractionState::Creating));

    shapeHandler->OnKeyDown(VK_ESCAPE, false, false, false);
    EXPECT_EQ(static_cast<int>(shapeHandler->GetState()), static_cast<int>(ui::annotation::InteractionState::Idle));
}

void RegisterUnimplemented() {
    TestRunner::Instance().NotImplemented("Rendering_BasicPage_GoldenMatch");
    TestRunner::Instance().NotImplemented("Text_Multilingual_Extraction");
}



int main() {
    PdfiumLibrary::Instance().Initialize();
    
    std::filesystem::create_directories("tests/fixtures/malformed");
    std::filesystem::create_directories("tests/fixtures/basic");
    SetupFixtures();
    
    RegisterUnimplemented();

    // PERF BENCHMARK
    {
        const wchar_t* files[] = {
            L"tests/fixtures/perf/1_page.pdf",
            L"tests/fixtures/perf/10_page.pdf",
            L"tests/fixtures/perf/50_page.pdf",
            L"tests/fixtures/perf/100_page.pdf",
            L"tests/fixtures/perf/text_heavy.pdf",
            L"tests/fixtures/perf/image_heavy.pdf"
        };

        std::cout << "\n--- RENDERING PERF BASELINE ---\n" << std::flush;
        for (auto file : files) {
            std::cout << "File: perf file\n" << std::flush;
            std::cout << std::flush;
            auto t0 = std::chrono::high_resolution_clock::now();
            auto docResult = PdfDocument::LoadFromFile(file);
            auto t1 = std::chrono::high_resolution_clock::now();
            if (!docResult.has_value()) {
                std::cout << "  Failed to load.\n" << std::flush;
                continue;
            }
            auto doc = std::move(docResult.value);
            std::cout << "  Load Time: " << std::chrono::duration<double, std::milli>(t1 - t0).count() << " ms\n" << std::flush;
            
            auto page = doc->GetPage(0);
            auto pdfPage = std::dynamic_pointer_cast<PdfPage>(page);
            if (pdfPage) {
                auto size = doc->GetPageSize(0);
                auto t2 = std::chrono::high_resolution_clock::now();
                
                FPDF_BITMAP bmp = FPDFBitmap_CreateEx(static_cast<int>(size.width), static_cast<int>(size.height), 4, NULL, 0);
                FPDFBitmap_FillRect(bmp, 0, 0, static_cast<int>(size.width), static_cast<int>(size.height), 0xFFFFFFFF);
                FPDF_RenderPageBitmap(bmp, pdfPage->GetHandle(), 0, 0, static_cast<int>(size.width), static_cast<int>(size.height), 0, FPDF_LCD_TEXT);
                auto t3 = std::chrono::high_resolution_clock::now();
                std::cout << "  First Page Render Time: " << std::chrono::duration<double, std::milli>(t3 - t2).count() << " ms\n" << std::flush;
                FPDFBitmap_Destroy(bmp);
                
                auto t4 = std::chrono::high_resolution_clock::now();
                FPDF_BITMAP thumb = FPDFBitmap_CreateEx(200, 250, 4, NULL, 0);
                FPDFBitmap_FillRect(thumb, 0, 0, 200, 250, 0xFFFFFFFF);
                FPDF_RenderPageBitmap(thumb, pdfPage->GetHandle(), 0, 0, 200, 250, 0, FPDF_LCD_TEXT);
                auto t5 = std::chrono::high_resolution_clock::now();
                std::cout << "  Thumbnail Render Time: " << std::chrono::duration<double, std::milli>(t5 - t4).count() << " ms\n" << std::flush;
                FPDFBitmap_Destroy(thumb);
            }
        }
        std::cout << "-------------------------------\n" << std::flush;
    }
    
    int result = TestRunner::Instance().RunAll();
    

    
    return result;
}


