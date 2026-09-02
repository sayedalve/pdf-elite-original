#pragma once
#include "ISelectableObject.h"
#include "core/interfaces/dom/IImage.h"
#include <memory>

namespace ui {
namespace interaction {

class ImageSelectableObject : public ISelectableObject {
public:
    ImageSelectableObject(std::shared_ptr<core::interfaces::dom::IImage> image, int pageIndex);

    std::string GetId() const override;
    int GetPageIndex() const override;
    Rect GetBounds() const override;
    void SetBounds(const Rect& bounds) override;
    
    double GetRotation() const override;
    void SetRotation(double degrees) override;
    
    // We could expose this specific object wrapper
    std::shared_ptr<core::interfaces::dom::IImage> GetImage() const { return m_image; }

private:
    std::shared_ptr<core::interfaces::dom::IImage> m_image;
    int m_pageIndex;
};

} // namespace interaction
} // namespace ui
