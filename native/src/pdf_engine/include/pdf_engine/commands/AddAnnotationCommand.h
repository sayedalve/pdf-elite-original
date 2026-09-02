#pragma once
#include "core/CommandStack.h"
#include <fpdfview.h>
#include <fpdf_annot.h>
#include <string>

namespace pdf_engine {
namespace commands {

class AddAnnotationCommand : public core::Command {
public:
    AddAnnotationCommand(FPDF_DOCUMENT doc, int pageIndex, FPDF_ANNOTATION_SUBTYPE subtype, const FS_RECTF& rect)
        : m_doc(doc), m_pageIndex(pageIndex), m_subtype(subtype), m_rect(rect), m_annotIndex(-1) {}

    bool Execute() override {
        if (!m_doc) return false;
        FPDF_PAGE page = FPDF_LoadPage(m_doc, m_pageIndex);
        if (!page) return false;

        // If we already created it (this is a Redo), we can't just create it again without tracking.
        // Actually, FPDFPage_CreateAnnot appends it to the end.
        FPDF_ANNOTATION annot = FPDFPage_CreateAnnot(page, m_subtype);
        if (!annot) {
            FPDF_ClosePage(page);
            return false;
        }

        FPDFAnnot_SetRect(annot, &m_rect);
        
        // Find the index so we can delete it later
        m_annotIndex = FPDFPage_GetAnnotCount(page) - 1;

        FPDFPage_CloseAnnot(annot);
        FPDF_ClosePage(page);
        return true;
    }

    bool Undo() override {
        if (!m_doc || m_annotIndex < 0) return false;
        FPDF_PAGE page = FPDF_LoadPage(m_doc, m_pageIndex);
        if (!page) return false;

        bool success = FPDFPage_RemoveAnnot(page, m_annotIndex);
        if (success) {
            m_annotIndex = -1; // Reset since it's removed
        }

        FPDF_ClosePage(page);
        return success;
    }

    std::wstring GetName() const override {
        return L"Add Annotation";
    }

private:
    FPDF_DOCUMENT m_doc;
    int m_pageIndex;
    FPDF_ANNOTATION_SUBTYPE m_subtype;
    FS_RECTF m_rect;
    int m_annotIndex;
};

} // namespace commands
} // namespace pdf_engine
