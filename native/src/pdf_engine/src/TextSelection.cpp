#include "TextSelection.h"

namespace pdf_engine {

TextSelection::TextSelection(std::shared_ptr<core::interfaces::dom::IPage> page) : m_page(page) {}

std::wstring TextSelection::ExtractText(double left, double top, double right, double bottom) {
    (void)left; (void)top; (void)right; (void)bottom;
    // Stub: In a full implementation, we would extract FPDF_TEXTPAGE, 
    // call FPDFText_GetBoundedText, and return it.
    return L""; 
}

std::vector<TextRect> TextSelection::GetSelectionRects(int startCharIndex, int endCharIndex) {
    (void)startCharIndex; (void)endCharIndex;
    std::vector<TextRect> rects;
    // Stub: use FPDFText_CountRects / FPDFText_GetRect
    return rects;
}

}
