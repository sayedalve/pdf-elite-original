#pragma once
#include "../framework/UIElement.h"
#include <string>
#include <functional>
#include "IconRenderer.h"

namespace controls {

class QuickToolCard : public framework::UIElement {
public:
    QuickToolCard(const std::wstring& title, const std::wstring& desc, IconType icon);

    void SetOnClick(std::function<void()> onClick) { m_onClick = onClick; }
    void SetEnabled(bool enabled) { m_isEnabled = enabled; }
    void SetColors(D2D1_COLOR_F normal, D2D1_COLOR_F hover, D2D1_COLOR_F text, D2D1_COLOR_F iconTint);

    void Render(ComPtr<ID2D1RenderTarget> target) override;

    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    void OnMouseLeave() override;

private:
    std::wstring m_title;
    std::wstring m_desc;
    IconType m_icon;
    std::function<void()> m_onClick;
    
    bool m_isEnabled = true;
    bool m_isHovered = false;
    bool m_isPressed = false;

    D2D1_COLOR_F m_colorNormal;
    D2D1_COLOR_F m_colorHover;
    D2D1_COLOR_F m_colorText;
    D2D1_COLOR_F m_colorIconTint;
};

} // namespace controls
