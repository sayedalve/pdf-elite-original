#include "TestFramework.h"
#include "../src/pdf_engine/src/PdfiumLibrary.h"
#include <iostream>
#include <fstream>
#include <filesystem>

extern void create_test_file(const wchar_t* path, const char* data, size_t size);
extern const char minimal_pdf[];

void create_test_file(const wchar_t* path, const char* data, size_t size) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }
    std::ofstream out(path, std::ios::binary);
    if (size > 0) out.write(data, size);
}

const char minimal_pdf[] = 
    "%PDF-1.4\n"
    "1 0 obj <</Type/Catalog/Pages 2 0 R>> endobj\n"
    "2 0 obj <</Type/Pages/Count 1/Kids[3 0 R]>> endobj\n"
    "3 0 obj <</Type/Page/Parent 2 0 R/MediaBox[0 0 612 792]/Contents 4 0 R/Resources<<>>>> endobj\n"
    "4 0 obj <</Length 0>> stream\nendstream\nendobj\n"
    "xref\n0 5\n0000000000 65535 f \n0000000009 00000 n \n0000000052 00000 n \n0000000101 00000 n \n0000000193 00000 n \n"
    "trailer <</Size 5/Root 1 0 R>>\nstartxref\n233\n%%EOF";

int main() {
    std::cout << "Starting Milestone 1 Empirical Stress Test Suite...\n";
    PdfiumLibrary::Instance().Initialize();

    std::error_code ec;
    std::filesystem::create_directories("tests/fixtures/basic", ec);
    std::filesystem::create_directories("tests/fixtures/output", ec);
    std::filesystem::create_directories("tests/fixtures/malformed", ec);

    create_test_file(L"tests/fixtures/basic/minimal.pdf", minimal_pdf, sizeof(minimal_pdf) - 1);

    int result = TestRunner::Instance().RunAll();
    return result;
}
