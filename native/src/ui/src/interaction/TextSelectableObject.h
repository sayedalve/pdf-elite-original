#pragma once
#include "ISelectableObject.h"
#include "core/interfaces/dom/ITextObject.h"
#include <memory>
#include <string>

namespace ui {
namespace interaction {

class TextSelectableObject : public ISelectableObject {
public:
    TextSelectableObject(std::shared_ptr<core::interfaces::dom::ITextObject> textObj, int pageIndex);

    std::string GetId() const override;
    int GetPageIndex() const override;
    Rect GetBounds() const override;
    void SetBounds(const Rect& bounds) override;
    
    double GetRotation() const override;
    void SetRotation(double degrees) override;
    
    std::shared_ptr<core::interfaces::dom::ITextObject> GetTextObject() const { return m_textObj; }

private:
    std::shared_ptr<core::interfaces::dom::ITextObject> m_textObj;
    int m_pageIndex;
    std::string m_id;
};

} // namespace interaction
} // namespace ui
