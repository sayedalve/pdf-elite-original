#include "TestFramework.h"
#include <fstream>
#include <filesystem>
#include <future>
#include "../src/pdf_engine/src/PdfDocument.h"
#include "../src/pdf_engine/src/SearchEngine.h"
#include "../src/core/interfaces/dom/CommandStack.h"
#include "../src/pdf_engine/src/commands/TextCommands.h"

// ---------------------------------------------------------
// Task 15B Validation Tests
// ---------------------------------------------------------

TEST(TextEditing_BasicModification_SaveReopen) {
    std::cout << "Loading file..." << std::endl; auto res = PdfDocument::LoadFromFile(L"tests/fixtures/text/basic_text.pdf"); std::cout << "Loaded!" << std::endl;
    std::cout << "Checking res.has_value()..." << std::endl; EXPECT_TRUE(res.has_value()); std::cout << "Checked!" << std::endl;
    std::cout << "Moving doc..." << std::endl; auto doc = std::move(res.value); std::cout << "Moved!" << std::endl;

    std::cout << "Getting page..." << std::endl; auto page = doc->GetPage(0); std::cout << "Got page!" << std::endl;
    EXPECT_TRUE(page != nullptr);

    // 1. Find text to edit
    auto textObjs = page->GetTextObjects();
    EXPECT_TRUE(!textObjs.empty());

    // We will just grab the first text object for now to see what it is
    auto firstObj = textObjs.front();
    std::wstring originalText = firstObj->GetText();

    // 2. Perform Edit
    std::wstring newText = L"Hello PDF Elite";
    auto editCmd = std::make_unique<pdf_engine::commands::EditTextCommand>(
        firstObj, originalText, newText);
        
    auto& stack = doc->GetCommandStack();
    EXPECT_TRUE(stack.ExecuteCommand(std::move(editCmd)));

    // 3. Save
    std::wstring savePath = L"tests/fixtures/text/basic_text_edited.pdf";
    EXPECT_TRUE(doc->SaveAs(savePath));

    // 4. Reopen and verify
    auto res2 = PdfDocument::LoadFromFile(savePath.c_str());
    EXPECT_TRUE(res2.has_value());
    auto doc2 = std::move(res2.value);
    {
        auto page2 = doc2->GetPage(0);
        auto textObjs2 = page2->GetTextObjects();
        EXPECT_TRUE(!textObjs2.empty());
        
        // Look for our new text
        bool foundNewText = false;
        for (auto obj : textObjs2) {
            if (obj->GetText() == newText) {
                foundNewText = true;
                break;
            }
        }
        
        EXPECT_TRUE(foundNewText);
    }
    
    // 5. Search test
    std::shared_ptr<core::interfaces::dom::IDocument> sharedDoc = std::move(doc2);
    pdf_engine::SearchEngine searchEngine(sharedDoc);
    std::promise<std::vector<pdf_engine::SearchResult>> searchPromise;
    searchEngine.SearchAsync(L"PDF Elite", false, false, [&](const std::vector<pdf_engine::SearchResult>& results) {
        searchPromise.set_value(results);
    });
    
    auto searchRes = searchPromise.get_future().get();
    EXPECT_TRUE(!searchRes.empty());
}

TEST(TextEditing_UnicodeModification) {
    std::cout << "Loading file..." << std::endl; auto res = PdfDocument::LoadFromFile(L"tests/fixtures/text/basic_text.pdf"); std::cout << "Loaded!" << std::endl;
    std::cout << "Checking res.has_value()..." << std::endl; EXPECT_TRUE(res.has_value()); std::cout << "Checked!" << std::endl;
    std::cout << "Moving doc..." << std::endl; auto doc = std::move(res.value); std::cout << "Moved!" << std::endl;
    std::cout << "Getting page..." << std::endl; auto page = doc->GetPage(0); std::cout << "Got page!" << std::endl;
    
    auto textObjs = page->GetTextObjects();
    auto firstObj = textObjs.front();
    std::wstring originalText = firstObj->GetText();

    std::wstring newText = L"বাংলা লেখা";
    auto editCmd = std::make_unique<pdf_engine::commands::EditTextCommand>(
        firstObj, originalText, newText);
        
    doc->GetCommandStack().ExecuteCommand(std::move(editCmd));

    std::wstring savePath = L"tests/fixtures/text/unicode_text_edited.pdf";
    EXPECT_TRUE(doc->SaveAs(savePath));

    auto res2 = PdfDocument::LoadFromFile(savePath.c_str());
    EXPECT_TRUE(res2.has_value());
    auto doc2 = std::move(res2.value);
    auto page2 = doc2->GetPage(0);
    
    auto textObjs2 = page2->GetTextObjects();
    bool foundNewText = false;
    for (auto obj : textObjs2) {
        if (obj->GetText() == newText) {
            foundNewText = true;
            break;
        }
    }
    
    printf("Checking foundNewText: %d\n", foundNewText); fflush(stdout); EXPECT_TRUE(foundNewText);
    printf("Returning from test!\n"); fflush(stdout);
}

TEST(TextEditing_AddText_Move_Delete) {
    std::cout << "Loading file..." << std::endl; auto res = PdfDocument::LoadFromFile(L"tests/fixtures/text/basic_text.pdf"); std::cout << "Loaded!" << std::endl;
    std::cout << "Checking res.has_value()..." << std::endl; EXPECT_TRUE(res.has_value()); std::cout << "Checked!" << std::endl;
    std::cout << "Moving doc..." << std::endl; auto doc = std::move(res.value); std::cout << "Moved!" << std::endl;
    std::cout << "Getting page..." << std::endl; auto page = doc->GetPage(0); std::cout << "Got page!" << std::endl;
    
    // 1. Add Text
    RectF bounds = { 100, 700, 300, 680 };
    auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
        doc.get(), 0, L"New Added Text", bounds, "Arial", 14.0f, 255, 0, 0, 255);
        
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));
    
    // Verify added
    auto page2 = doc->GetPage(0);
    auto textObjs = page2->GetTextObjects();
    bool found = false;
    std::shared_ptr<core::interfaces::dom::ITextObject> addedObj = nullptr;
    for (auto obj : textObjs) {
        if (obj->GetText() == L"New Added Text") {
            found = true;
            addedObj = obj;
            break;
        }
    }
    EXPECT_TRUE(found);
    EXPECT_TRUE(addedObj != nullptr);
    
    // 2. Move Text
    RectF newBounds = { 150, 750, 350, 730 };
    auto moveCmd = std::make_unique<pdf_engine::commands::MoveTextCommand>(
        addedObj, bounds, newBounds);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(moveCmd)));
    
    // 3. Delete Text
    auto delCmd = std::make_unique<pdf_engine::commands::DeleteTextCommand>(
        doc.get(), 0, addedObj);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(delCmd)));
    
    // Verify deleted
    textObjs = page->GetTextObjects();
    found = false;
    for (auto obj : textObjs) {
        if (obj->GetText() == L"New Added Text") {
            found = true;
            break;
        }
    }
    EXPECT_FALSE(found);
    
    // 4. Undo Delete
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    
    textObjs = page->GetTextObjects();
    found = false;
    for (auto obj : textObjs) {
        if (obj->GetText() == L"New Added Text") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}
