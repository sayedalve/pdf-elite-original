#include <iostream>
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <fpdf_save.h>
#include <fstream>
struct SpikeFileWrite : public FPDF_FILEWRITE { std::ofstream* stream; };
static int WriteBlock(FPDF_FILEWRITE* pThis, const void* pData, unsigned long size) { static_cast<SpikeFileWrite*>(pThis)->stream->write(reinterpret_cast<const char*>(pData), size); return 1; }
int main() {
    FPDF_LIBRARY_CONFIG config; config.version = 2; config.m_pUserFontPaths = nullptr; config.m_pIsolate = nullptr; config.m_v8EmbedderSlot = 0;
    FPDF_InitLibraryWithConfig(&config);
    FPDF_DOCUMENT doc = FPDF_CreateNewDocument();
    FPDF_PAGE page = FPDFPage_New(doc, 0, 612, 792);
    FPDF_PAGEOBJECT text = FPDFPageObj_NewTextObj(doc, "Arial", 12);
    
    // Test \n inside FPDFText_SetText
    FPDFText_SetText(text, (FPDF_WIDESTRING)L"Line 1\nLine 2");
    
    FS_MATRIX mat = { 1, 0, 0, 1, 100, 700 };
    FPDFPageObj_SetMatrix(text, &mat);
    FPDFPage_InsertObject(page, text);
    FPDFPage_GenerateContent(page);
    std::ofstream file("NewlineTest.pdf", std::ios::binary);
    SpikeFileWrite writer; writer.version = 1; writer.stream = &file; writer.WriteBlock = WriteBlock;
    FPDF_SaveAsCopy(doc, &writer, 0);
    FPDF_ClosePage(page); FPDF_CloseDocument(doc); FPDF_DestroyLibrary(); return 0;
}
