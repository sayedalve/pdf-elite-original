#pragma once
#include "UIElement.h"
#include <string>

namespace framework {

enum class LayoutDirection { Horizontal, Vertical };

class Panel : public UIElement {
public:
    void AddChild(std::shared_ptr<UIElement> child);
    const std::vector<std::shared_ptr<UIElement>>& GetChildren() const { return m_children; }
    void ClearChildren() { m_children.clear(); m_hoveredChild.reset(); }
    
    void SetLayoutDirection(LayoutDirection dir) { m_direction = dir; }
    void SetPadding(float padding) { m_padding = padding; }
    void SetSpacing(float spacing) { m_spacing = spacing; }
    void SetBackgroundColor(D2D1_COLOR_F color) { m_bg = color; }

    void Render(ComPtr<ID2D1RenderTarget> target) override;
    void Layout(const D2D1_RECT_F& bounds) override;

    bool HitTest(float x, float y) override;
    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    void OnMouseLeave() override;
    void OnMouseWheel(float delta) override;
    
    std::wstring GetTooltipText() const override;

protected:
    std::vector<std::shared_ptr<UIElement>> m_children;
    LayoutDirection m_direction = LayoutDirection::Vertical;
    float m_padding = 0.0f;
    float m_spacing = 0.0f;
    D2D1_COLOR_F m_bg = D2D1::ColorF(0, 0.0f); // Transparent by default
    
    std::shared_ptr<UIElement> m_hoveredChild;
};

} // namespace framework
