#include <gtest/gtest.h>
#include <fpdfview.h>
#include <fpdf_annot.h>
#include <fpdf_edit.h>
#include <fpdf_save.h>
#include <fstream>
#include <iostream>

class LineFeasibilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        FPDF_LIBRARY_CONFIG config;
        config.version = 2;
        config.m_pUserFontPaths = nullptr;
        config.m_pIsolate = nullptr;
        config.m_v8EmbedderSlot = 0;
        FPDF_InitLibraryWithConfig(&config);
    }
    void TearDown() override {
        FPDF_DestroyLibrary();
    }
};

static int SaveBlock(void* param, const void* data, unsigned long size) {
    std::ofstream* file = static_cast<std::ofstream*>(param);
    file->write(static_cast<const char*>(data), size);
    return 1;
}

TEST_F(LineFeasibilityTest, EvaluateOptions) {
    // Create new document
    FPDF_DOCUMENT doc = FPDF_CreateNewDocument();
    FPDF_PAGE page = FPDFPage_New(doc, 0, 600, 800);

    // Option A: Try setting Line with public APIs
    FPDF_ANNOTATION lineAnnot = FPDFPage_CreateAnnot(page, FPDF_ANNOT_LINE);
    FS_RECTF rect = { 100, 100, 200, 200 };
    FPDFAnnot_SetRect(lineAnnot, &rect);
    
    // Try setting string value for /L (will result in string type, not array)
    const wchar_t* l_str = L"[100 100 200 200]";
    FPDFAnnot_SetStringValue(lineAnnot, "L", (FPDF_WIDESTRING)l_str);

    // Option C: AP stream polyfill
    FPDF_ANNOTATION apLine = FPDFPage_CreateAnnot(page, FPDF_ANNOT_LINE);
    FS_RECTF apRect = { 300, 300, 400, 400 };
    FPDFAnnot_SetRect(apLine, &apRect);
    // Draw a diagonal line using AP
    const wchar_t* ap_str = L"0.0 0.0 m 100.0 100.0 l S";
    FPDFAnnot_SetAP(apLine, FPDF_ANNOT_APPEARANCEMODE_NORMAL, (FPDF_WIDESTRING)ap_str);

    // Option E: Custom keys
    FPDF_ANNOTATION customLine = FPDFPage_CreateAnnot(page, FPDF_ANNOT_LINE);
    FS_RECTF customRect = { 500, 500, 600, 600 };
    FPDFAnnot_SetRect(customLine, &customRect);
    FPDFAnnot_SetStringValue(customLine, "PDFElite_StartX", (FPDF_WIDESTRING)L"500");
    FPDFAnnot_SetStringValue(customLine, "PDFElite_StartY", (FPDF_WIDESTRING)L"500");
    FPDFAnnot_SetStringValue(customLine, "PDFElite_EndX", (FPDF_WIDESTRING)L"600");
    FPDFAnnot_SetStringValue(customLine, "PDFElite_EndY", (FPDF_WIDESTRING)L"600");

    // Option D: Use Ink instead
    FPDF_ANNOTATION inkLine = FPDFPage_CreateAnnot(page, FPDF_ANNOT_INK);
    FS_POINTF points[2] = { {100, 700}, {200, 800} };
    FPDFAnnot_AddInkStroke(inkLine, points, 2);

    // Save PDF
    std::ofstream file("LineFeasibilityTest.pdf", std::ios::binary);
    FPDF_FILEWRITE writer;
    writer.version = 1;
    writer.WriteBlock = SaveBlock;
    FPDF_SaveAsCopy(doc, &writer, 0);
    file.close();

    FPDFPage_CloseAnnot(lineAnnot);
    FPDFPage_CloseAnnot(apLine);
    FPDFPage_CloseAnnot(customLine);
    FPDFPage_CloseAnnot(inkLine);

    FPDF_ClosePage(page);
    FPDF_CloseDocument(doc);
}
