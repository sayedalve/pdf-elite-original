#include <fpdfview.h>
#include <fpdf_edit.h>
#include <fpdf_save.h>
#include <fpdf_text.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

std::vector<uint8_t> ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size))
        return buffer;
    return {};
}

struct CustomFileWrite : public FPDF_FILEWRITE {
    std::ofstream* stream;
};

int BlockWrite(FPDF_FILEWRITE* pThis, const void* pData, unsigned long size) {
    auto* custom = static_cast<CustomFileWrite*>(pThis);
    custom->stream->write(reinterpret_cast<const char*>(pData), size);
    return 1;
}

void TestTextInsertion(FPDF_DOCUMENT doc, FPDF_PAGE page, FPDF_FONT font, const std::wstring& text, float x, float y) {
    FPDF_PAGEOBJECT textObj = FPDFPageObj_CreateTextObj(doc, font, 12.0f);
    
    // Set text (UTF-16LE encoded)
    std::vector<uint16_t> utf16;
    for (wchar_t c : text) {
        utf16.push_back(static_cast<uint16_t>(c));
    }
    utf16.push_back(0); // Null terminator

    FPDFText_SetText(textObj, reinterpret_cast<FPDF_WIDESTRING>(utf16.data()));
    
    // Position
    FS_MATRIX matrix = {1.0f, 0.0f, 0.0f, 1.0f, x, y};
    FPDFPageObj_Transform(textObj, matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f);
    
    FPDFPage_InsertObject(page, textObj);
}

int main() {
    FPDF_InitLibrary();

    FPDF_DOCUMENT doc = FPDF_CreateNewDocument();
    FPDF_PAGE page = FPDFPage_New(doc, 0, 612, 792);

    // Load fonts
    auto arialData = ReadFile("C:\\Windows\\Fonts\\arial.ttf");
    auto nirmalaData = ReadFile("C:\\Windows\\Fonts\\Nirmala.ttc");
    
    std::cout << "Arial size: " << arialData.size() << "\n";
    std::cout << "Nirmala size: " << nirmalaData.size() << "\n";

    FPDF_FONT arialFont = FPDFText_LoadFont(doc, arialData.data(), static_cast<uint32_t>(arialData.size()), FPDF_FONT_TRUETYPE, false);
    FPDF_FONT nirmalaFont = FPDFText_LoadFont(doc, nirmalaData.data(), static_cast<uint32_t>(nirmalaData.size()), FPDF_FONT_TRUETYPE, true); // cid = true for Unicode

    if (!arialFont || !nirmalaFont) {
        std::cerr << "Failed to load fonts\n";
        return 1;
    }

    // Insert English (Arial)
    TestTextInsertion(doc, page, arialFont, L"Hello World (Arial)", 50, 700);
    
    // Insert Bangla (Nirmala)
    TestTextInsertion(doc, page, nirmalaFont, L"বাংলাদেশ (Nirmala)", 50, 650);

    // Insert Mixed (Nirmala)
    TestTextInsertion(doc, page, nirmalaFont, L"Hello বাংলাদেশ PDF (Nirmala)", 50, 600);
    
    // Try Bangla in Arial (Expected: blocks or missing glyphs, but does it crash/fail to save?)
    TestTextInsertion(doc, page, arialFont, L"বাংলাদেশ (Arial)", 50, 550);

    FPDFPage_GenerateContent(page);

    std::ofstream out("C:\\Users\\sayed\\Downloads\\PDF-Elite\\native\\tests\\unicode_prototype.pdf", std::ios::binary);
    CustomFileWrite writer;
    writer.version = 1;
    writer.stream = &out;
    writer.WriteBlock = BlockWrite;
    FPDF_SaveAsCopy(doc, &writer, 0);

    FPDF_ClosePage(page);
    FPDF_CloseDocument(doc);
    FPDF_DestroyLibrary();

    std::cout << "Generated unicode_prototype.pdf\n";
    return 0;
}
