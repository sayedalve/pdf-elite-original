#include "TestFramework.h"
#include "../src/pdf_engine/src/PdfiumLibrary.h"
#include <iostream>
#include <fstream>
#include <filesystem>

static const char kMinimalPdf[] = 
    "%PDF-1.4\n"
    "1 0 obj <</Type/Catalog/Pages 2 0 R>> endobj\n"
    "2 0 obj <</Type/Pages/Count 1/Kids[3 0 R]>> endobj\n"
    "3 0 obj <</Type/Page/Parent 2 0 R/MediaBox[0 0 612 792]/Contents 4 0 R/Resources<<>>>> endobj\n"
    "4 0 obj <</Length 0>> stream\nendstream\nendobj\n"
    "xref\n0 5\n0000000000 65535 f \n0000000009 00000 n \n0000000052 00000 n \n0000000101 00000 n \n0000000193 00000 n \n"
    "trailer <</Size 5/Root 1 0 R>>\nstartxref\n233\n%%EOF";

static void WriteTestFile(const wchar_t* path, const char* data, size_t size) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }
    std::ofstream out(path, std::ios::binary);
    if (size > 0) out.write(data, size);
}

int main() {
    PdfiumLibrary::Instance().Initialize();

    std::error_code ec;
    std::filesystem::create_directories("tests/fixtures/basic", ec);
    std::filesystem::create_directories("tests/fixtures/images", ec);
    std::filesystem::create_directories("tests/fixtures/malformed", ec);
    std::filesystem::create_directories("tests/fixtures/output", ec);

    WriteTestFile(L"tests/fixtures/basic/minimal.pdf", kMinimalPdf, sizeof(kMinimalPdf) - 1);
    WriteTestFile(L"tests/fixtures/malformed/empty.pdf", "", 0);
    WriteTestFile(L"tests/fixtures/malformed/truncated.pdf", "%PDF-", 5);

    std::cout << "==============================================\n";
    std::cout << " PDF Elite E2E & UI Interaction Test Suite\n";
    std::cout << "==============================================\n\n";

    int result = TestRunner::Instance().RunAll();

    return result;
}
