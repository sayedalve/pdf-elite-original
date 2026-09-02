#pragma once
#include "framework/UIElement.h"
#include <string>
#include <functional>
#include "IconRenderer.h"

namespace controls {

class IconButton : public framework::UIElement {
public:
    IconButton(const std::wstring& text = L"", IconType icon = IconType::None);

    void SetOnClick(std::function<void()> onClick) { m_onClick = onClick; }
    void SetVertical(bool v) { m_isVertical = v; }
    void SetColors(D2D1_COLOR_F normal, D2D1_COLOR_F hover, D2D1_COLOR_F text);
    void SetEnabled(bool enabled) { m_isEnabled = enabled; }
    void SetActive(bool active) { m_isActive = active; }
    void SetShowText(bool show) { m_showText = show; }

    void Render(ComPtr<ID2D1RenderTarget> target) override;

    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    void OnMouseLeave() override;
    
    std::function<void(bool)> onHoverStateChanged;
    void SetTooltipText(const std::wstring& t) { m_tooltip = t; }
    std::wstring GetTooltipText() const override;

private:
    std::wstring m_text;
    std::wstring m_tooltip;
    IconType m_icon = IconType::None;
    std::function<void()> m_onClick;
    bool m_isEnabled = true;
    bool m_isHovered = false;
    bool m_isVertical = false;
    bool m_isPressed = false;
    bool m_isActive = false;
    bool m_showText = true;

    D2D1_COLOR_F m_colorNormal = {30/255.0f, 34/255.0f, 47/255.0f, 1.0f};
    D2D1_COLOR_F m_colorHover = {40/255.0f, 45/255.0f, 60/255.0f, 1.0f};
    D2D1_COLOR_F m_colorText = {245/255.0f, 246/255.0f, 247/255.0f, 1.0f};
};

} // namespace controls

