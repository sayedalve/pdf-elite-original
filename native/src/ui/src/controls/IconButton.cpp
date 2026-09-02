#include "IconButton.h"
#include "../GraphicsDevice.h"
#include "../NativeDesignSystem.h"

namespace controls {

IconButton::IconButton(const std::wstring& text, IconType icon) : m_text(text), m_icon(icon) {
    m_colorNormal = design::Colors::Control;
    m_colorHover = design::Colors::ControlHover;
    m_colorText = design::Colors::TextPrimary;
}

void IconButton::SetColors(D2D1_COLOR_F normal, D2D1_COLOR_F hover, D2D1_COLOR_F text) {
    m_colorNormal = normal;
    m_colorHover = hover;
    m_colorText = text;
}

void IconButton::Render(ComPtr<ID2D1RenderTarget> target) {
    ComPtr<ID2D1SolidColorBrush> brush;
    
    D2D1_COLOR_F bg = m_colorNormal;
    if (!m_isEnabled) {
        bg = m_colorNormal;
    } else if (m_isPressed) {
        bg = design::Colors::ControlPressed;
    } else if (m_isActive) {
        bg = design::Colors::ControlActive;
    } else if (m_isHovered) {
        bg = m_colorHover;
    }
    
    target->CreateSolidColorBrush(bg, &brush);
    D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(m_bounds, design::Radius::R6, design::Radius::R6);
    target->FillRoundedRectangle(roundedRect, brush.Get());

    if (m_isActive) {
        ComPtr<ID2D1SolidColorBrush> activeBorder;
        target->CreateSolidColorBrush(design::Colors::Focus, &activeBorder);
        target->DrawRoundedRectangle(roundedRect, activeBorder.Get(), 1.0f);
    }

    D2D1_COLOR_F currentTextColor = m_colorText;
    if (!m_isEnabled) {
        currentTextColor = design::Colors::TextDisabled;
    } else if (m_isActive) {
        currentTextColor = design::Colors::AccentPrimary;
    }

    if (m_showText && !m_text.empty()) {
        auto textFormat = design::FontManager::Instance().GetToolbar();
        target->CreateSolidColorBrush(currentTextColor, &brush);
        D2D1_RECT_F textBounds = m_bounds;
        if (m_icon != IconType::None) {
            if (m_isVertical) {
                textBounds.bottom -= design::Spacing::Medium1;
            } else {
                textBounds.left += 36.0f;
            }
        }
        if (textFormat) {
            target->DrawText(m_text.c_str(), static_cast<UINT32>(m_text.length()), 
                textFormat, textBounds, brush.Get());
        }
    }
    
    if (m_icon != IconType::None) {
        float w = m_bounds.right - m_bounds.left;
        float h = m_bounds.bottom - m_bounds.top;
        float iconSize = m_isVertical ? 24.0f : 24.0f;
        D2D1_RECT_F iconBounds;
        if (m_isVertical) {
            iconBounds = {
                m_bounds.left + (w - iconSize) / 2.0f,
                m_bounds.top + 20.0f,
                m_bounds.left + (w + iconSize) / 2.0f,
                m_bounds.top + 20.0f + iconSize
            };
        } else {
            iconBounds = {
                m_bounds.left + 8.0f,
                m_bounds.top + (h - iconSize) / 2.0f,
                m_bounds.left + 8.0f + iconSize,
                m_bounds.top + (h + iconSize) / 2.0f
            };
        }
        IconRenderer::DrawIcon(target, m_icon, iconBounds, currentTextColor);
    }
}

void IconButton::OnMouseMove(float x, float y) {
    (void)x; (void)y;
    if (m_isEnabled && !m_isHovered) {
        m_isHovered = true;
        if (onHoverStateChanged) onHoverStateChanged(true);
    }
}

void IconButton::OnMouseDown(float x, float y) {
    (void)x; (void)y;
    if (m_isEnabled) m_isPressed = true;
}

void IconButton::OnMouseUp(float x, float y) {
    (void)x; (void)y;
    if (m_isEnabled && m_isPressed && m_isHovered && m_onClick) {
        m_onClick();
    }
    m_isPressed = false;
}

void IconButton::OnMouseLeave() {
    if (m_isHovered) {
        m_isHovered = false;
        if (onHoverStateChanged) onHoverStateChanged(false);
    }
    m_isPressed = false;
}



std::wstring IconButton::GetTooltipText() const {
    if (!m_tooltip.empty()) return m_tooltip;
    if (m_showText && !m_text.empty()) {
        return L""; // No tooltip if text is already visibly drawn on the button
    }
    return m_text;
}

} // namespace controls

