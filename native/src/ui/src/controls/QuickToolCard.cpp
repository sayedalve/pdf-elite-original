#include "QuickToolCard.h"
#include "../GraphicsDevice.h"
#include "../NativeDesignSystem.h"

namespace controls {

QuickToolCard::QuickToolCard(const std::wstring& title, const std::wstring& desc, IconType icon) 
    : m_title(title), m_desc(desc), m_icon(icon) {
    
    m_colorNormal = design::Colors::SurfaceActiveTab; // Maps to CARD
    m_colorHover = design::Colors::SurfacePressed;    // Maps to CARD_HOVER
    m_colorText = design::Colors::TextPrimary;
    m_colorIconTint = design::Colors::AccentPrimary;
}

void QuickToolCard::SetColors(D2D1_COLOR_F normal, D2D1_COLOR_F hover, D2D1_COLOR_F text, D2D1_COLOR_F iconTint) {
    m_colorNormal = normal;
    m_colorHover = hover;
    m_colorText = text;
    m_colorIconTint = iconTint;
}

void QuickToolCard::Render(ComPtr<ID2D1RenderTarget> target) {
    ComPtr<ID2D1SolidColorBrush> brush;
    D2D1_COLOR_F bg = m_colorNormal;
    if (m_isEnabled && m_isPressed) bg = design::Colors::SurfacePressed;
    else if (m_isEnabled && m_isHovered) bg = m_colorHover;
    
    target->CreateSolidColorBrush(bg, &brush);
    D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(m_bounds, 16.0f, 16.0f);
    target->FillRoundedRectangle(roundedRect, brush.Get());

    ComPtr<ID2D1SolidColorBrush> borderBrush;
    target->CreateSolidColorBrush(m_isHovered ? design::Colors::BorderStrong : design::Colors::Border, &borderBrush);
    target->DrawRoundedRectangle(roundedRect, borderBrush.Get(), 1.0f);

    float alpha = m_isEnabled ? 1.0f : 0.4f;

    D2D1_RECT_F iconBgBounds = {m_bounds.left + 16.0f, m_bounds.top + 16.0f, m_bounds.left + 52.0f, m_bounds.top + 52.0f};
    ComPtr<ID2D1SolidColorBrush> iconBgBrush;
    D2D1_COLOR_F iconBgColor = m_colorIconTint;
    iconBgColor.a = alpha; // Full solid color
    target->CreateSolidColorBrush(iconBgColor, &iconBgBrush);
    target->FillRoundedRectangle(D2D1::RoundedRect(iconBgBounds, 10.0f, 10.0f), iconBgBrush.Get());

    D2D1_COLOR_F iconColor = D2D1::ColorF(D2D1::ColorF::White); // White icon
    iconColor.a = alpha;
    D2D1_RECT_F iconBounds = {iconBgBounds.left + 8.0f, iconBgBounds.top + 8.0f, iconBgBounds.right - 8.0f, iconBgBounds.bottom - 8.0f};
    IconRenderer::DrawIcon(target, m_icon, iconBounds, iconColor);

    auto titleFormat = design::FontManager::Instance().GetSectionHeading();
    auto descFormat = design::FontManager::Instance().GetSecondary();

    if (titleFormat && descFormat) {
        ComPtr<ID2D1SolidColorBrush> titleBrush, descBrush;
        target->CreateSolidColorBrush(m_colorText, &titleBrush);
        
        D2D1_COLOR_F descColor = m_colorText;
        descColor.a = 0.6f * alpha;
        target->CreateSolidColorBrush(descColor, &descBrush);

        titleBrush->SetOpacity(alpha);

        D2D1_RECT_F titleRect = {m_bounds.left + 64.0f, m_bounds.top + 16.0f, m_bounds.right - 16.0f, m_bounds.top + 36.0f};
        target->DrawText(m_title.c_str(), static_cast<UINT32>(m_title.length()), titleFormat, titleRect, titleBrush.Get());

        D2D1_RECT_F descRect = {titleRect.left, titleRect.bottom + 2.0f, titleRect.right, m_bounds.bottom - 12.0f};
        target->DrawText(m_desc.c_str(), static_cast<UINT32>(m_desc.length()), descFormat, descRect, descBrush.Get());
    }
}

void QuickToolCard::OnMouseMove(float /*x*/, float /*y*/) {
    if (!m_isEnabled) return;
    m_isHovered = true;
}

void QuickToolCard::OnMouseDown(float /*x*/, float /*y*/) {
    if (!m_isEnabled) return;
    m_isPressed = true;
}

void QuickToolCard::OnMouseUp(float /*x*/, float /*y*/) {
    if (!m_isEnabled) return;
    if (m_isPressed) {
        m_isPressed = false;
        if (m_isHovered && m_onClick) {
            m_onClick();
        }
    }
}

void QuickToolCard::OnMouseLeave() {
    m_isHovered = false;
    m_isPressed = false;
}

} // namespace controls
