#include "pdf_engine/LineAnnotationAdapter.h"
#include <fpdf_annot.h>

namespace pdf_engine {

bool LineAnnotationAdapter::GetGeometry(FPDF_ANNOTATION annot, core::interfaces::dom::LineGeometry& outGeometry) {
    if (!annot) return false;
    
    // There is actually a public API for this! FPDFAnnot_GetLine
    FS_POINTF start, end;
    if (FPDFAnnot_GetLine(annot, &start, &end)) {
        outGeometry.start.x = start.x;
        outGeometry.start.y = start.y;
        outGeometry.end.x = end.x;
        outGeometry.end.y = end.y;
        return true;
    }
    return false;
}

void LineAnnotationAdapter::SetGeometry(FPDF_ANNOTATION annot, const core::interfaces::dom::LineGeometry& geometry) {
    if (!annot) return;
    
    FS_RECTF rect;
    rect.left = geometry.start.x;
    rect.top = geometry.start.y;
    rect.right = geometry.end.x;
    rect.bottom = geometry.end.y;
    FPDFAnnot_SetRect(annot, &rect);
}

} // namespace pdf_engine
