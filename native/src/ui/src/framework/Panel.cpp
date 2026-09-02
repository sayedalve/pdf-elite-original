#include "Panel.h"

namespace framework {

void Panel::AddChild(std::shared_ptr<UIElement> child) {
    m_children.push_back(child);
}

void Panel::Render(ComPtr<ID2D1RenderTarget> target) {
    if (m_bg.a > 0.0f) {
        ComPtr<ID2D1SolidColorBrush> brush;
        target->CreateSolidColorBrush(m_bg, &brush);
        target->FillRectangle(m_bounds, brush.Get());
    }

    for (auto& child : m_children) {
        if (child && child->IsVisible()) {
            child->Render(target);
        }
    }
}

void Panel::Layout(const D2D1_RECT_F& bounds) {
    UIElement::Layout(bounds);
    
    if (m_children.empty()) return;

    float currentX = bounds.left + m_padding;
    float currentY = bounds.top + m_padding;

    float availableWidth = bounds.right - bounds.left - (m_padding * 2);
    float availableHeight = bounds.bottom - bounds.top - (m_padding * 2);

    // Simple distribution logic for demonstration
    float childWidth = m_direction == LayoutDirection::Horizontal ? 
        (availableWidth - (m_spacing * (m_children.size() - 1))) / m_children.size() : availableWidth;
        
    float childHeight = m_direction == LayoutDirection::Vertical ? 
        (availableHeight - (m_spacing * (m_children.size() - 1))) / m_children.size() : availableHeight;

    for (auto& child : m_children) {
        D2D1_RECT_F childBounds = {
            currentX,
            currentY,
            currentX + childWidth,
            currentY + childHeight
        };
        child->Layout(childBounds);

        if (m_direction == LayoutDirection::Horizontal) {
            currentX += childWidth + m_spacing;
        } else {
            currentY += childHeight + m_spacing;
        }
    }
}

bool Panel::HitTest(float x, float y) {
    return UIElement::HitTest(x, y);
}

void Panel::OnMouseMove(float x, float y) {
    std::shared_ptr<UIElement> currentHover = nullptr;
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        auto& child = *it;
        if (child && child->IsVisible() && child->HitTest(x, y)) {
            currentHover = child;
            break;
        }
    }

    if (m_hoveredChild != currentHover) {
        if (m_hoveredChild) m_hoveredChild->OnMouseLeave();
        m_hoveredChild = currentHover;
    }

    if (m_hoveredChild) {
        m_hoveredChild->OnMouseMove(x, y);
    }
}

void Panel::OnMouseDown(float x, float y) {
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        auto& child = *it;
        if (child && child->IsVisible() && child->HitTest(x, y)) {
            child->OnMouseDown(x, y);
            return;
        }
    }
}

void Panel::OnMouseUp(float x, float y) {
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        auto& child = *it;
        if (child && child->IsVisible() && child->HitTest(x, y)) {
            child->OnMouseUp(x, y);
            return;
        }
    }
}

void Panel::OnMouseLeave() {
    if (m_hoveredChild) {
        m_hoveredChild->OnMouseLeave();
        m_hoveredChild = nullptr;
    }
}

void Panel::OnMouseWheel(float delta) {
    if (m_hoveredChild) m_hoveredChild->OnMouseWheel(delta);
}



std::wstring Panel::GetTooltipText() const {
    if (m_hoveredChild) {
        std::wstring childTip = m_hoveredChild->GetTooltipText();
        if (!childTip.empty()) return childTip;
    }
    return L"";
}

} // namespace framework
