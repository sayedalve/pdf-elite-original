#pragma once

#include "core/interfaces/dom/IDocument.h"
#include <windows.h>
#include <string>
#include <memory>

namespace pdf_engine {
namespace operations {

struct CreateBlankParams {
    int pageSizeIndex = 0;                  // 0=Letter, 1=A4, 2=Legal, 3=A3, 4=Custom
    double widthPt = 612.0;                 // Width in points (default: Letter 8.5" = 612 pt)
    double heightPt = 792.0;                // Height in points (default: Letter 11" = 792 pt)
    int unitIndex = 0;                      // 0=Points, 1=Inches, 2=Millimeters
    bool isPortrait = true;                 // true = Portrait, false = Landscape
    int pageCount = 1;                      // Number of blank pages to initialize (1-500)
    std::wstring outputPath;                // Optional output file path
};

std::shared_ptr<core::interfaces::dom::IDocument> CreateBlankDocument(const CreateBlankParams& params);
bool CreateBlankPdfFile(const CreateBlankParams& params);

} // namespace operations
} // namespace pdf_engine
