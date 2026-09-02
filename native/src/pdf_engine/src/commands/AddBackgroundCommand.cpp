#include "pdf_engine/commands/AddBackgroundCommand.h"
#include "CommandUtils.h"
#include "PdfDocument.h"
#include "PdfPage.h"
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <algorithm>

namespace pdf_engine {
namespace commands {

AddBackgroundCommand::AddBackgroundCommand(std::shared_ptr<core::interfaces::dom::IDocument> doc, const BackgroundParams& params)
    : m_docShared(doc), m_docRaw(nullptr), m_params(params) {
}

AddBackgroundCommand::AddBackgroundCommand(core::interfaces::dom::IDocument* doc, const BackgroundParams& params)
    : m_docShared(nullptr), m_docRaw(doc), m_params(params) {
}

AddBackgroundCommand::~AddBackgroundCommand() {
    // If not applied (undone), but handles still present, they have already been destroyed in Undo.
}

core::interfaces::dom::IDocument* AddBackgroundCommand::GetDoc() const {
    return m_docShared ? m_docShared.get() : m_docRaw;
}

bool AddBackgroundCommand::Execute() {
    auto doc = GetDoc();
    if (!doc) return false;

    PdfDocument* pdfDoc = dynamic_cast<PdfDocument*>(doc);
    std::unique_lock<std::recursive_mutex> lock;
    if (pdfDoc) {
        lock = std::unique_lock<std::recursive_mutex>(pdfDoc->GetMutex());
    }

    int totalPages = doc->PageCount();
    if (totalPages <= 0) return false;

    std::vector<int> targetPages = ParsePageRange(m_params.pageScope, m_params.pageRange, m_params.currentPage, totalPages);
    if (targetPages.empty()) return false;

    std::vector<uint8_t> imgData;
    int imgW = 0, imgH = 0;
    if (!m_params.isColor) {
        if (!LoadImageToBgra(m_params.imagePath, imgData, imgW, imgH) || imgData.empty()) {
            return false;
        }
    }

    m_createdObjects.clear();

    for (int pageIdx : targetPages) {
        auto page = doc->GetPage(pageIdx);
        if (!page) continue;

        PdfPage* pdfPage = dynamic_cast<PdfPage*>(page.get());
        if (!pdfPage) continue;

        FPDF_PAGE pageHandle = pdfPage->GetHandle();
        double pageWidth = FPDF_GetPageWidth(pageHandle);
        double pageHeight = FPDF_GetPageHeight(pageHandle);

        if (m_params.isColor) {
            uint8_t r = GetRValue(m_params.color);
            uint8_t g = GetGValue(m_params.color);
            uint8_t b = GetBValue(m_params.color);
            uint8_t a = (m_params.opacity <= 1.0) ?
                static_cast<uint8_t>(m_params.opacity * 255.0) :
                static_cast<uint8_t>(std::clamp(m_params.opacity, 0.0, 255.0));

            FPDF_PAGEOBJECT rectObj = FPDFPageObj_CreateNewRect(0.0f, 0.0f,
                                                                static_cast<float>(pageWidth),
                                                                static_cast<float>(pageHeight));
            if (!rectObj) continue;

            FPDFPageObj_SetFillColor(rectObj, r, g, b, a);
            FPDFPath_SetDrawMode(rectObj, FPDF_FILLMODE_ALTERNATE, 0); // Fill only

            FPDFPage_InsertObjectAtIndex(pageHandle, rectObj, 0); // Insert at index 0 (behind existing content)
            FPDFPage_GenerateContent(pageHandle);
            m_createdObjects[pageIdx] = rectObj;
        } else {
            FPDF_DOCUMENT docHandle = pdfDoc ? pdfDoc->GetHandle() : nullptr;
            if (!docHandle) continue;

            FPDF_PAGEOBJECT imgObj = FPDFPageObj_NewImageObj(docHandle);
            if (!imgObj) continue;

            FPDF_BITMAP bmp = FPDFBitmap_CreateEx(imgW, imgH, FPDFBitmap_BGRA, imgData.data(), imgW * 4);
            if (!bmp) {
                FPDFPageObj_Destroy(imgObj);
                continue;
            }

            if (!FPDFImageObj_SetBitmap(&pageHandle, 1, imgObj, bmp)) {
                FPDFBitmap_Destroy(bmp);
                FPDFPageObj_Destroy(imgObj);
                continue;
            }
            FPDFBitmap_Destroy(bmp);

            FS_MATRIX matrix = {
                static_cast<float>(pageWidth), 0.0f,
                0.0f, static_cast<float>(pageHeight),
                0.0f, 0.0f
            };
            FPDFPageObj_SetMatrix(imgObj, &matrix);

            FPDFPage_InsertObjectAtIndex(pageHandle, imgObj, 0); // Behind existing content
            FPDFPage_GenerateContent(pageHandle);
            m_createdObjects[pageIdx] = imgObj;
        }

        page->InvalidateTextIndex();
    }

    m_applied = !m_createdObjects.empty();
    return m_applied;
}

bool AddBackgroundCommand::Undo() {
    auto doc = GetDoc();
    if (!doc || m_createdObjects.empty()) return false;

    PdfDocument* pdfDoc = dynamic_cast<PdfDocument*>(doc);
    std::unique_lock<std::recursive_mutex> lock;
    if (pdfDoc) {
        lock = std::unique_lock<std::recursive_mutex>(pdfDoc->GetMutex());
    }

    for (const auto& [pageIdx, obj] : m_createdObjects) {
        auto page = doc->GetPage(pageIdx);
        if (!page) continue;

        PdfPage* pdfPage = dynamic_cast<PdfPage*>(page.get());
        if (!pdfPage) continue;

        FPDF_PAGE pageHandle = pdfPage->GetHandle();
        FPDFPage_RemoveObject(pageHandle, obj);
        FPDFPageObj_Destroy(obj);
        FPDFPage_GenerateContent(pageHandle);
        page->InvalidateTextIndex();
    }

    m_createdObjects.clear();
    m_applied = false;
    return true;
}

} // namespace commands
} // namespace pdf_engine
