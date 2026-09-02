#include "pdf_engine/operations/CombinePdfs.h"
#include "PdfDocument.h"
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <fpdf_ppo.h>
#include <fpdf_save.h>
#include <fstream>
#include <filesystem>

namespace pdf_engine {
namespace operations {

namespace {

struct FileWriter : public FPDF_FILEWRITE {
    std::ofstream file;
    FileWriter(const std::wstring& path) : file(path, std::ios::binary) {
        version = 1;
        WriteBlock = [](FPDF_FILEWRITE* pThis, const void* pData, unsigned long size) -> int {
            auto* writer = static_cast<FileWriter*>(pThis);
            if (writer->file.write(static_cast<const char*>(pData), size)) {
                return 1;
            }
            return 0;
        };
    }
};

} // anonymous namespace

bool CombinePdfDocuments(const CombineParams& params) {
    if (params.sourceFiles.empty() || params.outputFile.empty()) {
        return false;
    }

    FPDF_DOCUMENT destDoc = FPDF_CreateNewDocument();
    if (!destDoc) return false;

    int currentDestIndex = 0;

    for (const auto& srcPath : params.sourceFiles) {
        auto srcDocRes = PdfDocument::LoadFromFile(srcPath.c_str());
        if (!srcDocRes.has_value() || !srcDocRes.value) {
            continue;
        }

        FPDF_DOCUMENT srcHandle = srcDocRes.value->GetHandle();
        if (!srcHandle) continue;

        int srcPageCount = FPDF_GetPageCount(srcHandle);
        if (srcPageCount <= 0) continue;

        if (FPDF_ImportPages(destDoc, srcHandle, nullptr, currentDestIndex)) {
            currentDestIndex += srcPageCount;
        }
    }

    if (currentDestIndex == 0) {
        FPDF_CloseDocument(destDoc);
        return false;
    }

    std::error_code ec;
    auto parentPath = std::filesystem::path(params.outputFile).parent_path();
    if (!parentPath.empty()) {
        std::filesystem::create_directories(parentPath, ec);
    }

    FileWriter writer(params.outputFile);
    if (!writer.file.is_open()) {
        FPDF_CloseDocument(destDoc);
        return false;
    }

    bool success = FPDF_SaveAsCopy(destDoc, &writer, 0);
    writer.file.close();
    FPDF_CloseDocument(destDoc);

    return success;
}

std::shared_ptr<core::interfaces::dom::IDocument> CombinePdfDocumentsToMemory(const CombineParams& params) {
    if (params.sourceFiles.empty()) {
        return nullptr;
    }

    FPDF_DOCUMENT destDoc = FPDF_CreateNewDocument();
    if (!destDoc) return nullptr;

    int currentDestIndex = 0;

    for (const auto& srcPath : params.sourceFiles) {
        auto srcDocRes = PdfDocument::LoadFromFile(srcPath.c_str());
        if (!srcDocRes.has_value() || !srcDocRes.value) {
            continue;
        }

        FPDF_DOCUMENT srcHandle = srcDocRes.value->GetHandle();
        if (!srcHandle) continue;

        int srcPageCount = FPDF_GetPageCount(srcHandle);
        if (srcPageCount <= 0) continue;

        if (FPDF_ImportPages(destDoc, srcHandle, nullptr, currentDestIndex)) {
            currentDestIndex += srcPageCount;
        }
    }

    if (currentDestIndex == 0) {
        FPDF_CloseDocument(destDoc);
        return nullptr;
    }

    auto resultDoc = std::make_shared<PdfDocument>(destDoc);

    if (!params.outputFile.empty()) {
        resultDoc->SaveAs(params.outputFile);
    }

    return resultDoc;
}

} // namespace operations
} // namespace pdf_engine
