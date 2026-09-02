#include "pdf_engine/commands/AddLinkCommand.h"
#include "CommandUtils.h"
#include "PdfDocument.h"
#include "PdfPage.h"
#include "PdfAnnotation.h"
#include <fpdfview.h>
#include <fpdf_annot.h>
#include <fpdf_doc.h>
#include <fpdf_edit.h>
#include <algorithm>
#include <cmath>

namespace pdf_engine {
namespace commands {

AddLinkCommand::AddLinkCommand(std::shared_ptr<core::interfaces::dom::IDocument> doc, const LinkParams& params)
    : m_docShared(doc), m_docRaw(nullptr), m_params(params) {
}

AddLinkCommand::AddLinkCommand(core::interfaces::dom::IDocument* doc, const LinkParams& params)
    : m_docShared(nullptr), m_docRaw(doc), m_params(params) {
}

core::interfaces::dom::IDocument* AddLinkCommand::GetDoc() const {
    return m_docShared ? m_docShared.get() : m_docRaw;
}

bool AddLinkCommand::Execute() {
    auto doc = GetDoc();
    if (!doc) return false;

    PdfDocument* pdfDoc = dynamic_cast<PdfDocument*>(doc);
    std::unique_lock<std::recursive_mutex> lock;
    if (pdfDoc) {
        lock = std::unique_lock<std::recursive_mutex>(pdfDoc->GetMutex());
    }

    auto page = doc->GetPage(m_params.pageIndex);
    if (!page) return false;

    PdfPage* pdfPage = dynamic_cast<PdfPage*>(page.get());
    if (!pdfPage) return false;

    // In PDF coordinates (Y-up), bottom is y, top is y + height
    RectF bounds = {
        static_cast<float>(m_params.x),
        static_cast<float>(m_params.y + m_params.height),
        static_cast<float>(m_params.x + m_params.width),
        static_cast<float>(m_params.y)
    };

    m_annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Link);
    if (!m_annot) return false;

    m_annot->SetBounds(bounds);

    if (m_params.drawBorder) {
        uint8_t r = GetRValue(m_params.borderColor);
        uint8_t g = GetGValue(m_params.borderColor);
        uint8_t b = GetBValue(m_params.borderColor);
        m_annot->SetColor(r, g, b, 255);
        m_annot->SetBorderWidth(1.0f);
    }

    auto pdfAnnot = std::dynamic_pointer_cast<PdfAnnotation>(m_annot);
    if (pdfAnnot && pdfAnnot->GetHandle()) {
        FPDF_ANNOTATION annotHandle = pdfAnnot->GetHandle();
        if (m_params.isUrl) {
            std::string asciiUrl = WideToNarrow(m_params.url);
            FPDFAnnot_SetURI(annotHandle, asciiUrl.c_str());
            m_annot->SetContents(asciiUrl);
        } else {
            std::wstring destStr = L"page=" + std::to_wstring(m_params.targetPage);
            m_annot->SetContents(WideToNarrow(destStr));
            FPDFAnnot_SetStringValue(annotHandle, "Dest", reinterpret_cast<FPDF_WIDESTRING>(destStr.c_str()));
        }
    }

    FPDFPage_GenerateContent(pdfPage->GetHandle());
    page->InvalidateTextIndex();
    return true;
}

bool AddLinkCommand::Undo() {
    auto doc = GetDoc();
    if (!doc) return false;

    PdfDocument* pdfDoc = dynamic_cast<PdfDocument*>(doc);
    std::unique_lock<std::recursive_mutex> lock;
    if (pdfDoc) {
        lock = std::unique_lock<std::recursive_mutex>(pdfDoc->GetMutex());
    }

    auto page = doc->GetPage(m_params.pageIndex);
    if (!page) return false;

    PdfPage* pdfPage = dynamic_cast<PdfPage*>(page.get());
    if (!pdfPage) return false;

    FPDF_PAGE pageHandle = pdfPage->GetHandle();

    // In PDF coordinates (Y-up), bottom is y, top is y + height
    RectF bounds = {
        static_cast<float>(m_params.x),
        static_cast<float>(m_params.y + m_params.height),
        static_cast<float>(m_params.x + m_params.width),
        static_cast<float>(m_params.y)
    };

    if (m_annot && page->RemoveAnnotation(m_annot)) {
        FPDFPage_GenerateContent(pageHandle);
        page->InvalidateTextIndex();
        m_annot = nullptr;
        return true;
    }

    // If m_annot handle was invalidated or page reloaded, search for matching annot
    int annotCount = FPDFPage_GetAnnotCount(pageHandle);
    for (int i = 0; i < annotCount; ++i) {
        FPDF_ANNOTATION a = FPDFPage_GetAnnot(pageHandle, i);
        if (a) {
            int subtype = FPDFAnnot_GetSubtype(a);
            if (subtype == static_cast<int>(core::interfaces::dom::AnnotationType::Link)) {
                FS_RECTF r;
                if (FPDFAnnot_GetRect(a, &r)) {
                    if (std::abs(r.left - bounds.left) < 2.0f && std::abs(r.bottom - bounds.bottom) < 2.0f) {
                        FPDFPage_CloseAnnot(a);
                        FPDFPage_RemoveAnnot(pageHandle, i);
                        FPDFPage_GenerateContent(pageHandle);
                        page->InvalidateTextIndex();
                        m_annot = nullptr;
                        return true;
                    }
                }
            }
            FPDFPage_CloseAnnot(a);
        }
    }

    return false;
}

} // namespace commands
} // namespace pdf_engine
