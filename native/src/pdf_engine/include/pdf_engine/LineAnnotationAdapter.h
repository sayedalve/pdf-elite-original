#pragma once
#include "core/interfaces/dom/IAnnotation.h"
#include <fpdfview.h>
#include <fpdf_annot.h>

namespace pdf_engine {

class LineAnnotationAdapter {
public:
    static bool GetGeometry(FPDF_ANNOTATION annot, core::interfaces::dom::LineGeometry& outGeometry);
    static void SetGeometry(FPDF_ANNOTATION annot, const core::interfaces::dom::LineGeometry& geometry);
};

} // namespace pdf_engine
