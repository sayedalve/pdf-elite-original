#pragma once
#include "core/Geometry.h"
#include <string>
#include <vector>
#include <cstdint>

namespace core {
namespace interfaces {
namespace dom {

class IImage {
public:
    virtual ~IImage() = default;

    virtual std::string GetId() const = 0;

    // The bounding box of the image on the PDF page in PDF coordinates
    virtual RectF GetBounds() const = 0;
    virtual void SetBounds(const RectF& bounds) = 0;

    // In a full implementation, we might expose the transformation matrix
    virtual Matrix3x2F GetTransform() const = 0;
    virtual void SetTransform(const Matrix3x2F& matrix) = 0;

    // Retrieve pixel dimensions of the underlying image
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;

    // Optional: get raw bitmap for Copy Image to Clipboard
    virtual std::vector<uint8_t> GetBitmapData() const = 0;
};

} // namespace dom
} // namespace interfaces
} // namespace core
