#pragma once
#include <string>
#include <algorithm>

namespace ui {
namespace interaction {

struct Rect {
    double left, top, right, bottom;
};

class ISelectableObject {
public:
    virtual ~ISelectableObject() = default;
    virtual std::string GetId() const = 0;
    virtual int GetPageIndex() const = 0;
    virtual Rect GetBounds() const = 0; // In page coordinates
    virtual void SetBounds(const Rect& bounds) = 0;
    virtual double GetRotation() const = 0;
    virtual void SetRotation(double degrees) = 0;
    
    virtual bool HitTest(double px, double py, double tolerance = 5.0) const {
        Rect b = GetBounds();
        return px >= (b.left - tolerance) && px <= (b.right + tolerance) && 
               py >= (std::min(b.top, b.bottom) - tolerance) && 
               py <= (std::max(b.top, b.bottom) + tolerance); 
    }
};

} // namespace interaction
} // namespace ui
