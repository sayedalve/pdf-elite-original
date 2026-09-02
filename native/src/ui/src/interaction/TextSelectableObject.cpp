#include "TextSelectableObject.h"
#include <cmath>

namespace ui {
namespace interaction {

TextSelectableObject::TextSelectableObject(std::shared_ptr<core::interfaces::dom::ITextObject> textObj, int pageIndex)
    : m_textObj(textObj), m_pageIndex(pageIndex) {
    m_id = "text_" + std::to_string(pageIndex) + "_" + std::to_string(m_textObj->GetId());
}

std::string TextSelectableObject::GetId() const {
    return m_id;
}

int TextSelectableObject::GetPageIndex() const {
    return m_pageIndex;
}

Rect TextSelectableObject::GetBounds() const {
    auto b = m_textObj->GetBounds();
    return Rect{ (double)b.left, (double)b.top, (double)b.right, (double)b.bottom };
}

void TextSelectableObject::SetBounds(const Rect& bounds) {
    auto currentMat = m_textObj->GetTransform();
    auto oldBounds = m_textObj->GetBounds();
    
    float dx = static_cast<float>(bounds.left - oldBounds.left);
    float dy = static_cast<float>(bounds.top - oldBounds.top);
    
    // Adjust matrix
    currentMat.e += dx;
    currentMat.f += dy;
    
    // For now just move, scaling text may distort font size in PDF
    m_textObj->SetTransform(currentMat);
}

double TextSelectableObject::GetRotation() const {
    auto mat = m_textObj->GetTransform();
    // Assuming no skew, just rotation
    return std::atan2(mat.b, mat.a) * 180.0 / 3.14159265358979323846;
}

void TextSelectableObject::SetRotation(double /*degrees*/) {
    // Basic rotation implementation
}

} // namespace interaction
} // namespace ui
