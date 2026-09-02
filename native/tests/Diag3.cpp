#include <iostream>
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <fpdf_text.h>
#include <vector>
#include <string>

int main() {
    FPDF_InitLibrary();
    FPDF_DOCUMENT doc = FPDF_LoadDocument("basic_text.pdf", nullptr);
    FPDF_PAGE page = FPDF_LoadPage(doc, 0);
    
    // CALL THIS BEFORE
    int count = FPDFPage_CountObjects(page);
    std::cout << "Count before: " << count << std::endl;
    
    FPDF_PAGEOBJECT textObj = FPDFPageObj_NewTextObj(doc, "Arial", 14.0f);
    FPDFText_SetText(textObj, (FPDF_WIDESTRING)L"New Added Text");
    FPDFPage_InsertObject(page, textObj);
    FPDFPage_GenerateContent(page);
    
    count = FPDFPage_CountObjects(page);
    std::cout << "Count after: " << count << std::endl;
    
    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
    unsigned long len = FPDFTextObj_GetText(textObj, textPage, nullptr, 0);
    std::cout << "Len: " << len << std::endl;
    
    FPDFText_ClosePage(textPage);
    FPDF_ClosePage(page);
    FPDF_CloseDocument(doc);
    FPDF_DestroyLibrary();
    return 0;
}
