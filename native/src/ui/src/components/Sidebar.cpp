#include "Sidebar.h"
#include "../NativeDesignSystem.h"

namespace components {

Sidebar::Sidebar() {
    SetBackgroundColor(design::Colors::SurfaceElevated);
    
    m_newDocBtn = std::make_shared<controls::IconButton>(L"New Document");
    // Gradient accent color setup will be in Render or styling
    m_newDocBtn->SetColors(design::Colors::AccentPrimary, design::Colors::AccentHover, D2D1::ColorF(1.0f, 1.0f, 1.0f));
    m_newDocBtn->SetOnClick([this]() { if (onNewDocument) onNewDocument(); });
    AddChild(m_newDocBtn);
    
    m_navHome = std::make_shared<SidebarItem>(L"Home", controls::IconType::Home);
    m_navHome->SetSelected(true);
    AddChild(m_navHome);
    
    m_navTools = std::make_shared<SidebarItem>(L"Tools", controls::IconType::Tools);
    AddChild(m_navTools);
    
    m_navRecent = std::make_shared<SidebarItem>(L"Recent", controls::IconType::Recent);
    AddChild(m_navRecent);
    
    m_navStarred = std::make_shared<SidebarItem>(L"Starred", controls::IconType::Star);
    AddChild(m_navStarred);
}

void Sidebar::Layout(const D2D1_RECT_F& bounds) {
    framework::Panel::Layout(bounds);
    
    // Top Header: 72px
    // Action Area: 76px (New doc button is 44px)
    // Navigation List starts at 72 + 76 = 148px
    
    float headerHeight = 72.0f;
    float actionAreaHeight = 76.0f;
    
    // New doc btn: 44px height, centered vertically in the 76px action area, padded
    float btnY = bounds.top + headerHeight + (actionAreaHeight - 44.0f) / 2.0f;
    m_newDocBtn->Layout(D2D1::RectF(bounds.left + 20.0f, btnY, bounds.right - 20.0f, btnY + 44.0f));
    
    // Navigation List (px-3 -> 12px padding left/right)
    float navY = bounds.top + headerHeight + actionAreaHeight + 16.0f; // some padding before items
    float itemHeight = 32.0f;
    
    m_navHome->Layout(D2D1::RectF(bounds.left, navY, bounds.right, navY + itemHeight));
    navY += itemHeight + 4.0f;
    
    m_navTools->Layout(D2D1::RectF(bounds.left, navY, bounds.right, navY + itemHeight));
    navY += itemHeight + 4.0f;
    
    // Label "Quick Access" could go here (skip for brevity in layout logic for now)
    navY += 16.0f;
    
    m_navRecent->Layout(D2D1::RectF(bounds.left, navY, bounds.right, navY + itemHeight));
    navY += itemHeight + 4.0f;
    
    m_navStarred->Layout(D2D1::RectF(bounds.left, navY, bounds.right, navY + itemHeight));
}

void Sidebar::Render(ComPtr<ID2D1RenderTarget> target) {
    framework::Panel::Render(target);
    
    // Draw Logo / Header area
    ComPtr<ID2D1SolidColorBrush> brush;
    target->CreateSolidColorBrush(design::Colors::TextPrimary, &brush);
    
    // Draw "PDF Elite" text
    D2D1_RECT_F logoRect = D2D1::RectF(m_bounds.left + 64.0f, m_bounds.top + 24.0f, m_bounds.right, m_bounds.top + 50.0f);
    auto format = design::FontManager::Instance().GetSectionHeading();
    if (format) {
        target->DrawTextW(L"PDF Elite", 9, format, logoRect, brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP, DWRITE_MEASURING_MODE_NATURAL);
    }
    
    // Draw PRO badge (approximate)
    target->CreateSolidColorBrush(design::Colors::Surface, &brush);
    D2D1_RECT_F badgeRect = D2D1::RectF(m_bounds.left + 150.0f, m_bounds.top + 26.0f, m_bounds.left + 190.0f, m_bounds.top + 46.0f);
    ComPtr<ID2D1RoundedRectangleGeometry> geo;
    auto factory = GraphicsDevice::Instance().GetD2DFactory();
    factory->CreateRoundedRectangleGeometry(D2D1::RoundedRect(badgeRect, 4.0f, 4.0f), &geo);
    target->FillGeometry(geo.Get(), brush.Get());
    
    target->CreateSolidColorBrush(design::Colors::AccentPrimary, &brush);
    target->DrawTextW(L"PRO", 3, design::FontManager::Instance().GetCaption(), badgeRect, brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP, DWRITE_MEASURING_MODE_NATURAL);

    // Draw right border
    if (SUCCEEDED(target->CreateSolidColorBrush(design::Colors::BorderSubtle, &brush))) {
        target->DrawLine(
            D2D1::Point2F(m_bounds.right, m_bounds.top),
            D2D1::Point2F(m_bounds.right, m_bounds.bottom),
            brush.Get(),
            1.0f
        );
    }
}

} // namespace components
