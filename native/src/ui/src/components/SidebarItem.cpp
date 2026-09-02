#include "SidebarItem.h"
#include "../NativeDesignSystem.h"
#include "../GraphicsDevice.h"

namespace components {

SidebarItem::SidebarItem(const std::wstring& text, controls::IconType icon) 
    : m_text(text), m_icon(icon) {}

void SidebarItem::Render(ComPtr<ID2D1RenderTarget> target) {
    if (!m_visible) return;

    D2D1_RECT_F bgRect = m_bounds;
    bgRect.left += 12.0f;
    bgRect.right -= 12.0f;
    bgRect.top += 2.0f;
    bgRect.bottom -= 2.0f;

    ComPtr<ID2D1SolidColorBrush> brush;
    
    if (m_isSelected) {
        target->CreateSolidColorBrush(design::Colors::SurfaceActiveTab, &brush);
        ComPtr<ID2D1RoundedRectangleGeometry> geo;
        auto factory = GraphicsDevice::Instance().GetD2DFactory();
        factory->CreateRoundedRectangleGeometry(D2D1::RoundedRect(bgRect, 8.0f, 8.0f), &geo);
        target->FillGeometry(geo.Get(), brush.Get());
        
        target->CreateSolidColorBrush(design::Colors::BorderSubtle, &brush);
        target->DrawGeometry(geo.Get(), brush.Get(), 1.0f);
    } else if (m_isHovered) {
        target->CreateSolidColorBrush(design::Colors::Surface, &brush);
        ComPtr<ID2D1RoundedRectangleGeometry> geo;
        auto factory = GraphicsDevice::Instance().GetD2DFactory();
        factory->CreateRoundedRectangleGeometry(D2D1::RoundedRect(bgRect, 8.0f, 8.0f), &geo);
        target->FillGeometry(geo.Get(), brush.Get());
    }

    // Draw Icon (placeholder for now, or use IconRenderer)
    
    // Draw Text
    target->CreateSolidColorBrush(m_isSelected ? design::Colors::TextPrimary : design::Colors::TextSecondary, &brush);
    D2D1_RECT_F textRect = bgRect;
    textRect.left += 32.0f; // space for icon
    
    auto format = design::FontManager::Instance().GetNavigation();
    if (format) {
        target->DrawTextW(m_text.c_str(), static_cast<UINT32>(m_text.length()), format, textRect, brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP, DWRITE_MEASURING_MODE_NATURAL);
    }
}

void SidebarItem::OnMouseMove(float x, float y) {
    (void)x; (void)y;
    m_isHovered = true;
}
void SidebarItem::OnMouseLeave() {
    m_isHovered = false;
    m_isPressed = false;
}
void SidebarItem::OnMouseDown(float x, float y) {
    (void)x; (void)y;
    m_isPressed = true;
}
void SidebarItem::OnMouseUp(float x, float y) {
    (void)x; (void)y;
    if (m_isPressed && m_onClick) m_onClick();
    m_isPressed = false;
}

} // namespace components
