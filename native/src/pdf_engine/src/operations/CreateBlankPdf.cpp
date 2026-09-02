#include "pdf_engine/operations/CreateBlankPdf.h"
#include "PdfDocument.h"
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <fpdf_save.h>
#include <fstream>
#include <algorithm>
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

void ComputeDimensions(const CreateBlankParams& params, double& outWidth, double& outHeight) {
    double w = 612.0;
    double h = 792.0;

    switch (params.pageSizeIndex) {
        case 0: // Letter
            w = 612.0;
            h = 792.0;
            break;
        case 1: // A4
            w = 595.276;
            h = 841.89;
            break;
        case 2: // Legal
            w = 612.0;
            h = 1008.0;
            break;
        case 3: // A3
            w = 841.89;
            h = 1190.55;
            break;
        case 4: // Custom
        default:
            w = params.widthPt;
            h = params.heightPt;
            if (params.unitIndex == 1) { // Inches
                w *= 72.0;
                h *= 72.0;
            } else if (params.unitIndex == 2) { // Millimeters
                w = w * 72.0 / 25.4;
                h = h * 72.0 / 25.4;
            }
            break;
    }

    if (w <= 0.0) w = 612.0;
    if (h <= 0.0) h = 792.0;

    if (!params.isPortrait && w < h) {
        std::swap(w, h);
    } else if (params.isPortrait && w > h) {
        std::swap(w, h);
    }

    outWidth = w;
    outHeight = h;
}

} // anonymous namespace

std::shared_ptr<core::interfaces::dom::IDocument> CreateBlankDocument(const CreateBlankParams& params) {
    FPDF_DOCUMENT doc = FPDF_CreateNewDocument();
    if (!doc) return nullptr;

    double w = 0.0, h = 0.0;
    ComputeDimensions(params, w, h);

    int count = std::clamp(params.pageCount, 1, 500);
    for (int i = 0; i < count; ++i) {
        FPDF_PAGE page = FPDFPage_New(doc, i, w, h);
        if (page) {
            FPDF_ClosePage(page);
        }
    }

    auto pdfDoc = std::make_shared<PdfDocument>(doc);

    if (!params.outputPath.empty()) {
        pdfDoc->SaveAs(params.outputPath);
    }

    return pdfDoc;
}

bool CreateBlankPdfFile(const CreateBlankParams& params) {
    if (params.outputPath.empty()) return false;

    FPDF_DOCUMENT doc = FPDF_CreateNewDocument();
    if (!doc) return false;

    double w = 0.0, h = 0.0;
    ComputeDimensions(params, w, h);

    int count = std::clamp(params.pageCount, 1, 500);
    for (int i = 0; i < count; ++i) {
        FPDF_PAGE page = FPDFPage_New(doc, i, w, h);
        if (page) {
            FPDF_ClosePage(page);
        }
    }

    FileWriter writer(params.outputPath);
    if (!writer.file.is_open()) {
        FPDF_CloseDocument(doc);
        return false;
    }

    bool success = FPDF_SaveAsCopy(doc, &writer, 0);
    writer.file.close();
    FPDF_CloseDocument(doc);
    return success;
}

} // namespace operations
} // namespace pdf_engine
