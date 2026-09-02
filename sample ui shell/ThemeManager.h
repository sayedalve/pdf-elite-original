// ThemeManager.h - Dark theme with better colors #0f1117 / #151821 / #7c9cff
#pragma once
#include "NativeDesignSystem.h"
#include <d2d1.h>
#include <wrl/client.h>

namespace PdfElite {

class ThemeManager {
public:
    ThemeManager();
    ~ThemeManager();

    HRESULT Initialize(ID2D1RenderTarget* rt, IDWriteFactory* dwFactory);
    void Release();

    // Theme accessors - exact HTML better colors
    DesignSystem& GetDesignSystem() { return m_designSystem; }
    const DesignSystem& GetDesignSystem() const { return m_designSystem; }

    // Helper for creating rounded rectangle geometry
    HRESULT CreateRoundedRectGeometry(
        ID2D1Factory* factory,
        const D2D1_RECT_F& rect,
        float radius,
        ID2D1RoundedRectangleGeometry** geometry);

    // Draw helpers - translate HTML visual to Direct2D
    void DrawCard(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect, bool hovered, bool active);
    void DrawPill(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect, bool active);
    void DrawPageShadow(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect);

    // Exact colors from HTML
    D2D1_COLOR_F AppBg() const { return Color::FromHex(DesignTokens::APP_BG); }
    D2D1_COLOR_F SidebarBg() const { return Color::FromHex(DesignTokens::SIDEBAR_BG); }
    D2D1_COLOR_F Surface() const { return Color::FromHex(DesignTokens::SURFACE); }
    D2D1_COLOR_F Elevated() const { return Color::FromHex(DesignTokens::ELEVATED); }
    D2D1_COLOR_F Accent() const { return Color::FromHex(DesignTokens::ACCENT); }

private:
    DesignSystem m_designSystem;
    Microsoft::WRL::ComPtr<ID2D1Factory> m_d2dFactory;
};

} // namespace PdfElite
