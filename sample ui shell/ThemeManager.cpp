// ThemeManager.cpp - Implements better colors theme #0f1117
#include "ThemeManager.h"

namespace PdfElite {

ThemeManager::ThemeManager() {}
ThemeManager::~ThemeManager() { Release(); }

HRESULT ThemeManager::Initialize(ID2D1RenderTarget* rt, IDWriteFactory* dwFactory) {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf());
    if (FAILED(hr)) return hr;
    return m_designSystem.Initialize(rt, dwFactory);
}

void ThemeManager::Release() {
    m_designSystem.Release();
}

HRESULT ThemeManager::CreateRoundedRectGeometry(
    ID2D1Factory* factory,
    const D2D1_RECT_F& rect,
    float radius,
    ID2D1RoundedRectangleGeometry** geometry) {
    D2D1_ROUNDED_RECT rr;
    rr.rect = rect;
    rr.radiusX = radius;
    rr.radiusY = radius;
    return factory->CreateRoundedRectangleGeometry(rr, geometry);
}

void ThemeManager::DrawCard(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect, bool hovered, bool active) {
    // Rounded rect 16px as in HTML .rounded-[16px]
    auto brush = hovered ? m_designSystem.brushCardHover : m_designSystem.brushCard;
    if (!brush) return;

    // Create rounded rect path
    D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(rect, 16.0f, 16.0f);
    
    rt->FillRoundedRectangle(rr, brush);

    // Border rgba(255,255,255,0.06)
    if (m_designSystem.brushBorder) {
        rt->DrawRoundedRectangle(rr, m_designSystem.brushBorder, 1.0f);
    }

    // Active glow with accent #7c9cff
    if (active && m_designSystem.brushAccent) {
        // Outer glow simulation - draw larger rounded rect with low alpha
        D2D1_ROUNDED_RECT glowRect = D2D1::RoundedRect(
            D2D1::RectF(rect.left - 1, rect.top - 1, rect.right + 1, rect.bottom + 1), 17.0f, 17.0f);
        rt->DrawRoundedRectangle(glowRect, m_designSystem.brushAccent, 1.0f);
    }
}

void ThemeManager::DrawPill(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect, bool active) {
    float radius = (rect.bottom - rect.top) / 2.0f;
    D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(rect, radius, radius);
    
    auto brush = active ? m_designSystem.brushElevated : m_designSystem.brushSurface;
    if (brush) rt->FillRoundedRectangle(rr, brush);
    
    if (m_designSystem.brushBorder) {
        rt->DrawRoundedRectangle(rr, m_designSystem.brushBorder, 1.0f);
    }
}

void ThemeManager::DrawPageShadow(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect) {
    // Shadow 0 8px 40px rgba(0,0,0,0.5) + 0 0 0 1px rgba(0,0,0,0.08)
    // In Direct2D we simulate with multiple draws
    for (int i = 1; i <= 4; ++i) {
        float alpha = 0.08f / i;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> shadowBrush;
        rt->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, alpha), &shadowBrush);
        D2D1_RECT_F shadowRect = D2D1::RectF(rect.left + i, rect.top + i, rect.right + i, rect.bottom + i);
        rt->FillRectangle(shadowRect, shadowBrush.Get());
    }
}

} // namespace PdfElite
