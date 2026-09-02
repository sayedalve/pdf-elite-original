#include "TestFramework.h"
#include "../src/pdf_engine/src/PdfDocument.h"
#include "../src/pdf_engine/src/PdfPage.h"
#include "../src/pdf_engine/include/pdf_engine/commands/AddLinkCommand.h"
#include "../src/pdf_engine/include/pdf_engine/commands/AddBackgroundCommand.h"
#include "../src/pdf_engine/include/pdf_engine/commands/AddWatermarkCommand.h"
#include "../src/pdf_engine/include/pdf_engine/commands/AddHeaderFooterCommand.h"
#include "../src/pdf_engine/include/pdf_engine/operations/CreateBlankPdf.h"
#include "../src/pdf_engine/include/pdf_engine/operations/ExtractImagesFromPdf.h"
#include "../src/pdf_engine/include/pdf_engine/operations/CombinePdfs.h"
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <filesystem>
#include <fstream>

using namespace pdf_engine;
using namespace pdf_engine::commands;
using namespace pdf_engine::operations;

TEST(Batch2_AddLinkCommand_ExecuteUndoRedo) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);

    LinkParams params;
    params.pageIndex = 0;
    params.x = 50.0;
    params.y = 50.0;
    params.width = 200.0;
    params.height = 40.0;
    params.isUrl = true;
    params.url = L"https://pdfelite.app";
    params.drawBorder = true;
    params.borderColor = RGB(0, 102, 204);

    auto cmd = std::make_unique<AddLinkCommand>(doc.get(), params);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(cmd)));

    auto page = doc->GetPage(0);
    EXPECT_TRUE(page != nullptr);
    auto annots = page->GetAnnotations();
    EXPECT_TRUE(!annots.empty());

    // Undo
    EXPECT_TRUE(doc->GetCommandStack().Undo());

    // Redo
    EXPECT_TRUE(doc->GetCommandStack().Redo());
}

TEST(Batch2_AddBackgroundCommand_ExecuteUndoRedo) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);

    auto page = doc->GetPage(0);
    auto pdfPage = std::dynamic_pointer_cast<PdfPage>(page);
    EXPECT_TRUE(pdfPage != nullptr);

    int initialCount = FPDFPage_CountObjects(pdfPage->GetHandle());

    BackgroundParams params;
    params.isColor = true;
    params.color = RGB(220, 230, 242);
    params.opacity = 0.75;
    params.pageScope = 0; // All pages

    auto cmd = std::make_unique<AddBackgroundCommand>(doc.get(), params);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(cmd)));

    int afterCount = FPDFPage_CountObjects(pdfPage->GetHandle());
    EXPECT_EQ(afterCount, initialCount + 1);

    // Undo
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    int undoCount = FPDFPage_CountObjects(pdfPage->GetHandle());
    EXPECT_EQ(undoCount, initialCount);

    // Redo
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    int redoCount = FPDFPage_CountObjects(pdfPage->GetHandle());
    EXPECT_EQ(redoCount, initialCount + 1);
}

TEST(Batch2_AddWatermarkCommand_ExecuteUndoRedo) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);

    auto page = doc->GetPage(0);
    auto pdfPage = std::dynamic_pointer_cast<PdfPage>(page);
    EXPECT_TRUE(pdfPage != nullptr);

    int initialCount = FPDFPage_CountObjects(pdfPage->GetHandle());

    WatermarkParams params;
    params.text = L"CONFIDENTIAL";
    params.fontName = L"Helvetica";
    params.fontSize = 40.0f;
    params.color = RGB(255, 0, 0);
    params.opacity = 0.4f;
    params.rotation = 45.0f;
    params.positionIndex = 0; // Center
    params.layerOver = true;
    params.pageScope = 0;

    auto cmd = std::make_unique<AddWatermarkCommand>(doc.get(), params);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(cmd)));

    int afterCount = FPDFPage_CountObjects(pdfPage->GetHandle());
    EXPECT_EQ(afterCount, initialCount + 1);

    // Verify the added object is a text object
    FPDF_PAGEOBJECT lastObj = FPDFPage_GetObject(pdfPage->GetHandle(), afterCount - 1);
    EXPECT_TRUE(lastObj != nullptr);
    EXPECT_EQ(FPDFPageObj_GetType(lastObj), FPDF_PAGEOBJ_TEXT);

    // Undo
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    int undoCount = FPDFPage_CountObjects(pdfPage->GetHandle());
    EXPECT_EQ(undoCount, initialCount);

    // Redo
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    int redoCount = FPDFPage_CountObjects(pdfPage->GetHandle());
    EXPECT_EQ(redoCount, initialCount + 1);
}

TEST(Batch2_AddHeaderFooterCommand_ExecuteUndoRedo) {
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);

    auto page = doc->GetPage(0);
    auto pdfPage = std::dynamic_pointer_cast<PdfPage>(page);
    EXPECT_TRUE(pdfPage != nullptr);

    int initialCount = FPDFPage_CountObjects(pdfPage->GetHandle());

    HeaderFooterParams params;
    params.leftHeader = L"PDF Elite Document";
    params.centerHeader = L"Page {page} of {total}";
    params.rightFooter = L"Confidential";
    params.fontSize = 10.0f;
    params.pageScope = 0;

    auto cmd = std::make_unique<AddHeaderFooterCommand>(doc.get(), params);
    EXPECT_TRUE(doc->GetCommandStack().ExecuteCommand(std::move(cmd)));

    int afterCount = FPDFPage_CountObjects(pdfPage->GetHandle());
    EXPECT_EQ(afterCount, initialCount + 3);

    // Undo
    EXPECT_TRUE(doc->GetCommandStack().Undo());
    int undoCount = FPDFPage_CountObjects(pdfPage->GetHandle());
    EXPECT_EQ(undoCount, initialCount);

    // Redo
    EXPECT_TRUE(doc->GetCommandStack().Redo());
    int redoCount = FPDFPage_CountObjects(pdfPage->GetHandle());
    EXPECT_EQ(redoCount, initialCount + 3);
}

TEST(Batch2_CreateBlankPdf_DocumentCreation) {
    CreateBlankParams params;
    params.pageSizeIndex = 0; // Letter
    params.widthPt = 612.0;
    params.heightPt = 792.0;
    params.pageCount = 3;
    params.isPortrait = true;
    params.outputPath = L"tests/fixtures/basic/blank_test_created.pdf";

    auto doc = CreateBlankDocument(params);
    EXPECT_TRUE(doc != nullptr);
    EXPECT_EQ(doc->PageCount(), 3);

    // Verify on disk
    EXPECT_TRUE(std::filesystem::exists(params.outputPath));

    // Reload
    auto reloadRes = PdfDocument::LoadFromFile(params.outputPath.c_str());
    EXPECT_TRUE(reloadRes.has_value());
    EXPECT_EQ(reloadRes.value->PageCount(), 3);
}

TEST(Batch2_CombinePdfs_MergeMultipleDocuments) {
    // Create source 1 (2 pages)
    CreateBlankParams p1;
    p1.pageCount = 2;
    p1.outputPath = L"tests/fixtures/basic/combine_source_1.pdf";
    EXPECT_TRUE(CreateBlankPdfFile(p1));

    // Create source 2 (3 pages)
    CreateBlankParams p2;
    p2.pageCount = 3;
    p2.outputPath = L"tests/fixtures/basic/combine_source_2.pdf";
    EXPECT_TRUE(CreateBlankPdfFile(p2));

    CombineParams combParams;
    combParams.sourceFiles = {
        L"tests/fixtures/basic/combine_source_1.pdf",
        L"tests/fixtures/basic/combine_source_2.pdf"
    };
    combParams.outputFile = L"tests/fixtures/basic/combine_merged_result.pdf";

    bool success = CombinePdfDocuments(combParams);
    EXPECT_TRUE(success);
    EXPECT_TRUE(std::filesystem::exists(combParams.outputFile));

    // Verify merged page count = 2 + 3 = 5
    auto mergedDocRes = PdfDocument::LoadFromFile(combParams.outputFile.c_str());
    EXPECT_TRUE(mergedDocRes.has_value());
    EXPECT_EQ(mergedDocRes.value->PageCount(), 5);
}

TEST(Batch2_ExtractImagesFromPdf_ExtractionWorkflow) {
    // Create a document with an image
    auto res = PdfDocument::LoadFromFile(L"tests/fixtures/basic/minimal.pdf");
    EXPECT_TRUE(res.has_value());
    auto doc = std::move(res.value);

    auto page = doc->GetPage(0);
    EXPECT_TRUE(page != nullptr);

    // Insert raw BGRA image (32x32)
    std::vector<uint8_t> rawBgra(32 * 32 * 4, 200);
    RectF imgBounds = { 50, 100, 150, 200 };
    auto img = page->InsertImageFromMemory(rawBgra, 32, 32, imgBounds);
    EXPECT_TRUE(img != nullptr);

    ExtractImagesParams extParams;
    extParams.outputDir = L"tests/fixtures/images/extracted_batch2";
    extParams.format = L"PNG";
    extParams.prefix = L"batch2_img_";
    extParams.pageScope = 0;

    int extracted = ExtractImagesFromDocument(doc.get(), extParams);
    EXPECT_TRUE(extracted >= 1);
}
