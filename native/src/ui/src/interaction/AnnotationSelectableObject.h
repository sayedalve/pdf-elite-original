#pragma once
#include "interaction/ISelectableObject.h"
#include "core/interfaces/dom/IAnnotation.h"
#include <memory>

namespace ui {
namespace interaction {

class AnnotationSelectableObject : public ISelectableObject {
public:
    AnnotationSelectableObject(std::shared_ptr<core::interfaces::dom::IAnnotation> annot, int pageIndex);

    std::string GetId() const override;
    int GetPageIndex() const override;
    
    Rect GetBounds() const override;
    void SetBounds(const Rect& bounds) override;
    
    double GetRotation() const override;
    void SetRotation(double degrees) override;

    bool HitTest(double px, double py, double tolerance = 5.0) const override;

    std::shared_ptr<core::interfaces::dom::IAnnotation> GetAnnotation() const { return m_annot; }

private:
    std::shared_ptr<core::interfaces::dom::IAnnotation> m_annot;
    int m_pageIndex;
    double m_rotation = 0.0;
};

} // namespace interaction
} // namespace ui
