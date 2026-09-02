#pragma once
#include "core/Geometry.h"
#include <string>
#include <vector>


namespace core {
namespace interfaces {
namespace dom {
class ITextPage {
public:
    virtual ~ITextPage() = default;

    virtual int GetCharCount() const = 0;
    int CountChars() const { return GetCharCount(); }
    virtual std::wstring GetText(int startCharIndex, int charCount) const = 0;
    virtual RectF GetCharBox(int charIndex) const = 0;
    virtual int GetCharIndexAtPos(double x, double y, double xTolerance, double yTolerance) const = 0;
    virtual std::vector<RectF> GetRects(int startCharIndex, int charCount) const = 0;
};

} // namespace dom
} // namespace interfaces
} // namespace core
