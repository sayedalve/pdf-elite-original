#include "pdf_engine/commands/AddWatermarkCommand.h"
#include "CommandUtils.h"
#include "PdfDocument.h"
#include "PdfPage.h"
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <fpdf_text.h>
#include <algorithm>
#include <cmath>

namespace pdf_engine {
namespace commands {

AddWatermarkCommand::AddWatermarkCommand(std::shared_ptr<core::interfaces::dom::IDocument> doc, const WatermarkParams& params)
    : m_docShared(doc), m_docRaw(nullptr), m_params(params) {
}

AddWatermarkCommand::AddWatermarkCommand(core::interfaces::dom::IDocument* doc, const WatermarkParams& params)
    : m_docShared(nullptr), m_docRaw(doc), m_params(params) {
}

core::interfaces::dom::IDocument* AddWatermarkCommand::GetDoc() const {
    return m_docShared ? m_docShared.get() : m_docRaw;
}

bool AddWatermarkCommand::Execute() {
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

    // Determine font name
    std::string fontName = "Helvetica";
    if (!m_params.fontName.empty()) {
        std::string fn = WideToNarrow(m_params.fontName);
        if (fn.find("Times") != std::string::npos) {
            if (m_params.bold && m_params.italic) fontName = "Times-BoldItalic";
            else if (m_params.bold) fontName = "Times-Bold";
            else if (m_params.italic) fontName = "Times-Italic";
            else fontName = "Times-Roman";
        } else if (fn.find("Courier") != std::string::npos) {
            if (m_params.bold && m_params.italic) fontName = "Courier-BoldOblique";
            else if (m_params.bold) fontName = "Courier-Bold";
            else if (m_params.italic) fontName = "Courier-Oblique";
            else fontName = "Courier";
        } else {
            if (m_params.bold && m_params.italic) fontName = "Helvetica-BoldOblique";
            else if (m_params.bold) fontName = "Helvetica-Bold";
            else if (m_params.italic) fontName = "Helvetica-Oblique";
            else fontName = "Helvetica";
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

        FPDF_DOCUMENT docHandle = pdfDoc ? pdfDoc->GetHandle() : nullptr;
        FPDF_PAGEOBJECT textObj = FPDFPageObj_NewTextObj(docHandle, fontName.c_str(), m_params.fontSize);
        if (!textObj) continue;

        FPDFText_SetText(textObj, reinterpret_cast<FPDF_WIDESTRING>(m_params.text.c_str()));

        uint8_t r = GetRValue(m_params.color);
        uint8_t g = GetGValue(m_params.color);
        uint8_t b = GetBValue(m_params.color);
        uint8_t a = (m_params.opacity <= 1.0f) ?
            static_cast<uint8_t>(m_params.opacity * 255.0f) :
            static_cast<uint8_t>(std::clamp(m_params.opacity, 0.0f, 255.0f));

        FPDFPageObj_SetFillColor(textObj, r, g, b, a);

        float approxWidth = static_cast<float>(m_params.text.length()) * m_params.fontSize * 0.5f;
        float approxHeight = m_params.fontSize;

        float cx = static_cast<float>(pageWidth) / 2.0f;
        float cy = static_cast<float>(pageHeight) / 2.0f;

        switch (m_params.positionIndex) {
            case 1: // Top-Left
                cx = 50.0f + approxWidth / 2.0f;
                cy = static_cast<float>(pageHeight) - 50.0f - approxHeight / 2.0f;
                break;
            case 2: // Top-Center
                cx = static_cast<float>(pageWidth) / 2.0f;
                cy = static_cast<float>(pageHeight) - 50.0f - approxHeight / 2.0f;
                break;
            case 3: // Top-Right
                cx = static_cast<float>(pageWidth) - 50.0f - approxWidth / 2.0f;
                cy = static_cast<float>(pageHeight) - 50.0f - approxHeight / 2.0f;
                break;
            case 4: // Bottom-Left
                cx = 50.0f + approxWidth / 2.0f;
                cy = 50.0f + approxHeight / 2.0f;
                break;
            case 5: // Bottom-Center
                cx = static_cast<float>(pageWidth) / 2.0f;
                cy = 50.0f + approxHeight / 2.0f;
                break;
            case 6: // Bottom-Right
                cx = static_cast<float>(pageWidth) - 50.0f - approxWidth / 2.0f;
                cy = 50.0f + approxHeight / 2.0f;
                break;
            case 0: // Center
            default:
                cx = static_cast<float>(pageWidth) / 2.0f;
                cy = static_cast<float>(pageHeight) / 2.0f;
                break;
        }

        float rad = m_params.rotation * 3.14159265358979323846f / 180.0f;
        float cosA = std::cos(rad);
        float sinA = std::sin(rad);

        float halfW = approxWidth / 2.0f;
        float halfH = approxHeight / 2.0f;
        float tx = cx - (halfW * cosA - halfH * sinA);
        float ty = cy - (halfW * sinA + halfH * cosA);

        FS_MATRIX mat = { cosA, sinA, -sinA, cosA, tx, ty };
        FPDFPageObj_SetMatrix(textObj, &mat);

        if (!m_params.layerOver) {
            FPDFPage_InsertObjectAtIndex(pageHandle, textObj, 0); // Background
        } else {
            FPDFPage_InsertObject(pageHandle, textObj); // Foreground
        }

        FPDFPage_GenerateContent(pageHandle);
        page->InvalidateTextIndex();
        m_createdObjects[pageIdx] = textObj;
    }

    m_applied = !m_createdObjects.empty();
    return m_applied;
}

bool AddWatermarkCommand::Undo() {
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
