#pragma once
#include "core/CommandStack.h"
#include <fpdfview.h>
#include <fpdf_annot.h>
#include <vector>

namespace pdf_engine {
namespace commands {

class AddInkAnnotationCommand : public core::Command {
public:
    using Stroke = std::vector<FS_POINTF>;

    AddInkAnnotationCommand(FPDF_DOCUMENT doc, int pageIndex, const std::vector<Stroke>& strokes, const FS_RECTF& rect, unsigned int colorARGB, float lineWidth)
        : m_doc(doc), m_pageIndex(pageIndex), m_strokes(strokes), m_rect(rect), m_color(colorARGB), m_lineWidth(lineWidth), m_annotIndex(-1) {}

    bool Execute() override {
        if (!m_doc) return false;
        FPDF_PAGE page = FPDF_LoadPage(m_doc, m_pageIndex);
        if (!page) return false;

        FPDF_ANNOTATION annot = FPDFPage_CreateAnnot(page, FPDF_ANNOT_INK);
        if (!annot) {
            FPDF_ClosePage(page);
            return false;
        }

        // Set geometry
        FPDFAnnot_SetRect(annot, &m_rect);

        // Set visual properties
        unsigned int a = (m_color >> 24) & 0xFF;
        unsigned int r = (m_color >> 16) & 0xFF;
        unsigned int g = (m_color >> 8) & 0xFF;
        unsigned int b = m_color & 0xFF;
        FPDFAnnot_SetColor(annot, FPDFANNOT_COLORTYPE_Color, r, g, b, a);
        
        // Add strokes
        for (const auto& stroke : m_strokes) {
            if (!stroke.empty()) {
                FPDFAnnot_AddInkStroke(annot, stroke.data(), stroke.size());
            }
        }
        
        // Find the index so we can delete it later on Undo
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
            m_annotIndex = -1;
        }

        FPDF_ClosePage(page);
        return success;
    }

    std::wstring GetName() const override {
        return L"Add Ink Annotation";
    }

private:
    FPDF_DOCUMENT m_doc;
    int m_pageIndex;
    std::vector<Stroke> m_strokes;
    FS_RECTF m_rect;
    unsigned int m_color;
    float m_lineWidth;
    int m_annotIndex;
};

} // namespace commands
} // namespace pdf_engine
