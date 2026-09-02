#pragma once
#include "core/CommandStack.h"
#include <fpdfview.h>
#include <fpdf_annot.h>
#include <string>

namespace pdf_engine {
namespace commands {

class TransformAnnotationCommand : public core::Command {
public:
    TransformAnnotationCommand(FPDF_DOCUMENT doc, int pageIndex, int annotIndex, const FS_RECTF& oldRect, const FS_RECTF& newRect)
        : m_doc(doc), m_pageIndex(pageIndex), m_annotIndex(annotIndex), m_oldRect(oldRect), m_newRect(newRect) {}

    bool Execute() override {
        return ApplyRect(m_newRect);
    }

    bool Undo() override {
        return ApplyRect(m_oldRect);
    }

    std::wstring GetName() const override {
        return L"Transform Annotation";
    }

private:
    FPDF_DOCUMENT m_doc;
    int m_pageIndex;
    int m_annotIndex;
    FS_RECTF m_oldRect;
    FS_RECTF m_newRect;

    bool ApplyRect(const FS_RECTF& rect) {
        if (!m_doc) return false;
        FPDF_PAGE page = FPDF_LoadPage(m_doc, m_pageIndex);
        if (!page) return false;

        FPDF_ANNOTATION annot = FPDFPage_GetAnnot(page, m_annotIndex);
        if (!annot) {
            FPDF_ClosePage(page);
            return false;
        }

        bool success = FPDFAnnot_SetRect(annot, &rect);
        
        FPDFPage_CloseAnnot(annot);
        FPDF_ClosePage(page);
        return success;
    }
};

} // namespace commands
} // namespace pdf_engine
