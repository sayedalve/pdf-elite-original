#include "TestFramework.h"
#include <fstream>
#include <filesystem>
#include <future>
#include "../src/pdf_engine/src/PdfDocument.h"
#include "../src/pdf_engine/src/SearchEngine.h"
#include "../src/core/interfaces/dom/CommandStack.h"
#include "../src/pdf_engine/src/commands/TextCommands.h"

using namespace pdf_engine;

TEST(MultilineText_Validation_Workflow) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/text/basic_text.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);

    {
        auto page = doc->GetPage(0);
        EXPECT_TRUE(page != nullptr);

        // 1. Create a new text block.
        RectF bounds = { 100, 700, 300, 600 };
        auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
            doc.get(), 0, L"Initial", bounds, "Arial", 12.0f, 0, 0, 0, 255);
        EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(addCmd)));

        auto textObjs = page->GetTextObjects();
        std::shared_ptr<core::interfaces::dom::ITextObject> addedObj = nullptr;
        for (auto obj : textObjs) {
            if (obj->GetText() == L"Initial") {
                addedObj = obj;
                break;
            }
        }
        EXPECT_TRUE(addedObj != nullptr);

        // 2. Set Lines
        std::vector<core::interfaces::dom::TextLineData> multilineData = {
            { L"This is a long paragraph that", 0, 0, 200, 15 },
            { L"should wrap automatically into", 0, 15, 200, 15 },
            { L"multiple lines based on the", 0, 30, 200, 15 },
            { L"width of the text box.", 0, 45, 200, 15 }
        };
        EXPECT_TRUE(addedObj->SetLines(multilineData));
        
        addedObj->SetText(L"This is a long paragraph that\r\nshould wrap automatically into\r\nmultiple lines based on the\r\nwidth of the text box.");
        
        // Save As
        std::wstring savePath = L"tests/fixtures/text/multiline_text_edited.pdf";
        EXPECT_TRUE(doc->SaveAs(savePath));
    }

    // Reopen
    doc.reset();
    std::wstring savePath = L"tests/fixtures/text/multiline_text_edited.pdf";
    auto res2 = PdfDocument::LoadFromFile(savePath.c_str());
    EXPECT_TRUE(res2.has_value());
    auto doc2 = std::move(res2.value);
    auto page2 = doc2->GetPage(0);
    EXPECT_TRUE(page2 != nullptr);
    
    auto textObjs2 = page2->GetTextObjects();
    std::shared_ptr<core::interfaces::dom::ITextObject> reloadedObj = nullptr;
    for (auto obj : textObjs2) {
        if (obj->GetText().find(L"paragraph") != std::wstring::npos) {
            reloadedObj = obj;
            break;
        }
    }
    EXPECT_TRUE(reloadedObj != nullptr);
    
    // Resize test
    std::vector<core::interfaces::dom::TextLineData> reflowedData = {
        { L"This is a long paragraph that should wrap", 0, 0, 250, 15 },
        { L"automatically into multiple lines based", 0, 15, 250, 15 },
        { L"on the width of the text box.", 0, 30, 250, 15 }
    };
    EXPECT_TRUE(reloadedObj->SetLines(reflowedData));
    
    std::wstring savePath2 = L"tests/fixtures/text/multiline_text_reflowed.pdf";
    EXPECT_TRUE(doc2->SaveAs(savePath2));
}
