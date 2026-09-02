#include <iostream>
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <fpdf_text.h>
#include <vector>
#include <string>

int main() {
    FPDF_InitLibrary();
    FPDF_DOCUMENT doc = FPDF_LoadDocument("basic_text.pdf", nullptr);
    
    // First page handle
    FPDF_PAGE page1 = FPDF_LoadPage(doc, 0);
    FPDFPage_CountObjects(page1); // parse it
    
    // Second page handle (like in AddTextCommand)
    FPDF_PAGE page2 = FPDF_LoadPage(doc, 0);
    
    FPDF_PAGEOBJECT textObj = FPDFPageObj_NewTextObj(doc, "Arial", 14.0f);
    FPDFText_SetText(textObj, (FPDF_WIDESTRING)L"New Added Text");
    FPDFPage_InsertObject(page2, textObj);
    FPDFPage_GenerateContent(page2);
    
    // Close second handle
    FPDF_ClosePage(page2);
    
    // Check first page handle!
    int count = FPDFPage_CountObjects(page1);
    std::cout << "Count on page1: " << count << std::endl;
    
    FPDF_ClosePage(page1);
    FPDF_CloseDocument(doc);
    FPDF_DestroyLibrary();
    return 0;
}
