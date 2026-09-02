#include <iostream>
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <fpdf_text.h>

int main() {
    FPDF_InitLibrary();
    FPDF_DOCUMENT doc = FPDF_CreateNewDocument();
    FPDF_PAGE page = FPDFPage_New(doc, 0, 600, 800);
    
    // CALL IT BEFORE
    FPDF_TEXTPAGE textPageFirst = FPDFText_LoadPage(page);
    FPDFText_ClosePage(textPageFirst);
    
    FPDF_FONT font = FPDFText_LoadStandardFont(doc, "Arial");
    FPDF_PAGEOBJECT textObj = FPDFPageObj_NewTextObj(doc, "Arial", 12.0f);
    
    const wchar_t* wstr = L"Hello PDF";
    FPDFText_SetText(textObj, (FPDF_WIDESTRING)wstr);
    
    FPDFPage_InsertObject(page, textObj);
    FPDFPage_GenerateContent(page);
    
    // CALL IT AFTER
    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
    
    unsigned long len = FPDFTextObj_GetText(textObj, textPage, nullptr, 0);
    std::cout << "Len: " << len << std::endl;
    
    FPDFText_ClosePage(textPage);
    FPDF_ClosePage(page);
    FPDF_CloseDocument(doc);
    FPDF_DestroyLibrary();
    return 0;
}
