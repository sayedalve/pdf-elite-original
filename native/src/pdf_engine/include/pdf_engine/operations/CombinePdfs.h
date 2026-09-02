#pragma once

#include "core/interfaces/dom/IDocument.h"
#include <windows.h>
#include <string>
#include <vector>
#include <memory>

namespace pdf_engine {
namespace operations {

struct CombineParams {
    std::vector<std::wstring> sourceFiles;   // Ordered list of filepaths to combine
    std::wstring outputFile;                 // Target merged PDF filepath
    bool openAfterMerge = true;              // Automatically open combined PDF
};

bool CombinePdfDocuments(const CombineParams& params);
std::shared_ptr<core::interfaces::dom::IDocument> CombinePdfDocumentsToMemory(const CombineParams& params);

} // namespace operations
} // namespace pdf_engine
