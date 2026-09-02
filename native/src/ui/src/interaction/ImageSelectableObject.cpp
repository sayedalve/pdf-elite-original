#include "ImageSelectableObject.h"
#include <cmath>

namespace ui {
namespace interaction {

ImageSelectableObject::ImageSelectableObject(std::shared_ptr<core::interfaces::dom::IImage> image, int pageIndex)
    : m_image(image), m_pageIndex(pageIndex) {
}

std::string ImageSelectableObject::GetId() const {
    return m_image->GetId();
}

int ImageSelectableObject::GetPageIndex() const {
    return m_pageIndex;
}

Rect ImageSelectableObject::GetBounds() const {
    auto b = m_image->GetBounds();
    return { b.left, b.top, b.right, b.bottom };
}

void ImageSelectableObject::SetBounds(const Rect& bounds) {
    m_image->SetBounds({ static_cast<float>(bounds.left), static_cast<float>(bounds.top), static_cast<float>(bounds.right), static_cast<float>(bounds.bottom) });
}

double ImageSelectableObject::GetRotation() const {
    auto mat = m_image->GetTransform();
    // atan2(b, a) gives rotation in radians
    return atan2(mat.b, mat.a) * 180.0 / 3.14159265358979323846;
}

void ImageSelectableObject::SetRotation(double degrees) {
    auto mat = m_image->GetTransform();
    
    // We need to rotate around the center of the image.
    // However, in our simple model, the matrix represents the full transform.
    // Let's compute current center from GetBounds()
    RectF bounds = m_image->GetBounds();
    float cx = (bounds.left + bounds.right) / 2.0f;
    float cy = (bounds.top + bounds.bottom) / 2.0f;
    
    double currentRot = GetRotation();
    double deltaRot = degrees - currentRot;
    double rad = deltaRot * 3.14159265358979323846 / 180.0;
    
    float cosA = static_cast<float>(cos(rad));
    float sinA = static_cast<float>(sin(rad));
    
    // Translate to origin, rotate, translate back
    Matrix3x2F transformMatrix;
    transformMatrix.a = cosA;
    transformMatrix.b = sinA;
    transformMatrix.c = -sinA;
    transformMatrix.d = cosA;
    transformMatrix.e = cx - cx * cosA + cy * sinA;
    transformMatrix.f = cy - cx * sinA - cy * cosA;
    
    // Combine matrices (mat = mat * transformMatrix)
    Matrix3x2F newMat;
    newMat.a = mat.a * transformMatrix.a + mat.b * transformMatrix.c;
    newMat.b = mat.a * transformMatrix.b + mat.b * transformMatrix.d;
    newMat.c = mat.c * transformMatrix.a + mat.d * transformMatrix.c;
    newMat.d = mat.c * transformMatrix.b + mat.d * transformMatrix.d;
    newMat.e = mat.e * transformMatrix.a + mat.f * transformMatrix.c + transformMatrix.e;
    newMat.f = mat.e * transformMatrix.b + mat.f * transformMatrix.d + transformMatrix.f;

    m_image->SetTransform(newMat);
}

} // namespace interaction
} // namespace ui
