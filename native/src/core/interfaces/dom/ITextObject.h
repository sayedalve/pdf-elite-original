#pragma once
#include "core/Geometry.h"
#include <string>
#include <vector>

namespace core {
namespace interfaces {
namespace dom {

struct TextLineData {
    std::wstring text;
    float x;
    float y;
    float width;
    float height;
};

class ITextObject {
public:
    virtual ~ITextObject() = default;

    virtual std::wstring GetText() const = 0;
    virtual bool SetText(const std::wstring& text) = 0;
    virtual bool SetLines(const std::vector<TextLineData>& lines) = 0;

    virtual float GetFontSize() const = 0;
    virtual bool SetFontSize(float size) = 0;

    virtual RectF GetBounds() const = 0;
    
    virtual Matrix3x2F GetTransform() const = 0;
    virtual bool SetTransform(const Matrix3x2F& matrix) = 0;
    
    virtual std::string GetFontName() const = 0;
    
    virtual void GetColor(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const = 0;
    virtual bool SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) = 0;
    
    // Stable identity
    virtual uint64_t GetId() const = 0;
};

} // namespace dom
} // namespace interfaces
} // namespace core
