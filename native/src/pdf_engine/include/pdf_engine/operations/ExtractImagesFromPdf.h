#pragma once

#include "core/interfaces/dom/IDocument.h"
#include <windows.h>
#include <string>
#include <memory>

namespace pdf_engine {
namespace operations {

struct ExtractImagesParams {
    std::wstring srcPdfPath;                // Source PDF file path
    std::wstring outputDir;                 // Target folder path
    std::wstring format = L"PNG";           // Image format: PNG, JPEG, BMP
    std::wstring prefix = L"img_p";         // Filename prefix
    int pageScope = 0;                      // 0 = All pages, 1 = Custom range
    std::wstring pageRange = L"1";
    int currentPage = 1;
    int totalPages = 1;
};

int ExtractImagesFromDocument(std::shared_ptr<core::interfaces::dom::IDocument> doc, const ExtractImagesParams& params);
int ExtractImagesFromDocument(core::interfaces::dom::IDocument* doc, const ExtractImagesParams& params);
int ExtractImagesFromPdf(const std::wstring& pdfPath, const ExtractImagesParams& params);

} // namespace operations
} // namespace pdf_engine
