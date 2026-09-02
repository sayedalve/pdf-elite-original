// NativeDesignSystem.h - PDF Elite Native Design System
// Better Colors: Exact from HTML UI liked by user
// #0f1117 app bg, #151821 sidebar, #1c1f2b surface, #232636 elevated, #2a2e3d card, #7c9cff accent
// Win32 / Direct2D / DirectWrite native - NO web runtime

#pragma once
#include <d2d1.h>
#include <dwrite.h>
#include <string>

namespace PdfElite {

struct Color {
    static D2D1_COLOR_F FromHex(uint32_t hex, float alpha = 1.0f) {
        float r = ((hex >> 16) & 0xFF) / 255.0f;
        float g = ((hex >> 8) & 0xFF) / 255.0f;
        float b = (hex & 0xFF) / 255.0f;
        return D2D1::ColorF(r, g, b, alpha);
    }
    static D2D1_COLOR_F FromRGBA(uint8_t r, uint8_t g, uint8_t b, float a = 1.0f) {
        return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, a);
    }
};

// Exact HTML tokens - better colors user liked
namespace DesignTokens {
    // Core backgrounds - from liked HTML
    constexpr uint32_t APP_BG = 0x0f1117;
    constexpr uint32_t SIDEBAR_BG = 0x151821;
    constexpr uint32_t SURFACE = 0x1c1f2b;
    constexpr uint32_t ELEVATED = 0x232636;
    constexpr uint32_t CARD = 0x2a2e3d;
    constexpr uint32_t CARD_HOVER = 0x32364a;

    // Borders - rgba(255,255,255,0.06) and 0.08
    const D2D1_COLOR_F BORDER = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.06f);
    const D2D1_COLOR_F BORDER_STRONG = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.10f);
    const D2D1_COLOR_F BORDER_HOVER = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.12f);

    // Text
    constexpr uint32_t TEXT_PRIMARY = 0xe4e6eb;
    constexpr uint32_t TEXT_SECONDARY = 0x8b90a8;
    constexpr uint32_t TEXT_TERTIARY = 0x5a5f7a;

    // Accent - #7c9cff
    constexpr uint32_t ACCENT = 0x7c9cff;
    constexpr uint32_t ACCENT_SECONDARY = 0xa78bfa;
    const D2D1_COLOR_F ACCENT_GLOW = D2D1::ColorF(0x7c / 255.0f, 0x9c / 255.0f, 0xff / 255.0f, 0.25f);
    const D2D1_COLOR_F ACCENT_SUBTLE = D2D1::ColorF(0x7c / 255.0f, 0x9c / 255.0f, 0xff / 255.0f, 0.12f);
    const D2D1_COLOR_F ACCENT_15 = D2D1::ColorF(0x7c / 255.0f, 0x9c / 255.0f, 0xff / 255.0f, 0.15f);

    // Status
    constexpr uint32_t SUCCESS = 0x34d399;
    constexpr uint32_t WARNING = 0xfbbf24;
    constexpr uint32_t DANGER = 0xfb7185;

    // Page
    const D2D1_COLOR_F PAGE_SHADOW = D2D1::ColorF(0, 0, 0, 0.5f);

    // Spacing - spacious, not cramped (fix validation)
    constexpr float SPACE_2XS = 2.0f;
    constexpr float SPACE_XS = 4.0f;
    constexpr float SPACE_SM = 8.0f;
    constexpr float SPACE_MD = 12.0f;
    constexpr float SPACE_LG = 16.0f;
    constexpr float SPACE_XL = 24.0f;
    constexpr float SPACE_2XL = 32.0f;
    constexpr float SPACE_3XL = 40.0f;

    // Radius - modern 2026
    constexpr float RADIUS_SM = 8.0f;
    constexpr float RADIUS_MD = 10.0f;
    constexpr float RADIUS_LG = 12.0f;
    constexpr float RADIUS_XL = 14.0f;
    constexpr float RADIUS_2XL = 16.0f;
    constexpr float RADIUS_3XL = 20.0f;

    // Dimensions - exact from HTML
    constexpr float SIDEBAR_WIDTH = 280.0f;
    constexpr float LEFT_RAIL_WIDTH = 72.0f;
    constexpr float RIGHT_RAIL_WIDTH = 48.0f;
    constexpr float TAB_BAR_HEIGHT = 44.0f;
    constexpr float TOOLBAR_HEIGHT = 56.0f;
    constexpr float TOP_BAR_HEIGHT = 44.0f;
    constexpr float PAGE_WIDTH = 794.0f;
    constexpr float PAGE_HEIGHT = 1123.0f;
}

enum class AppMode {
    Home,
    Viewer
};

enum class ViewerMode {
    View,
    Comment,
    Edit,
    Organize,
    Search
};

struct DesignSystem {
    // Brushes - cached Direct2D brushes
    ID2D1SolidColorBrush* brushAppBg = nullptr;
    ID2D1SolidColorBrush* brushSidebarBg = nullptr;
    ID2D1SolidColorBrush* brushSurface = nullptr;
    ID2D1SolidColorBrush* brushElevated = nullptr;
    ID2D1SolidColorBrush* brushCard = nullptr;
    ID2D1SolidColorBrush* brushCardHover = nullptr;
    ID2D1SolidColorBrush* brushBorder = nullptr;
    ID2D1SolidColorBrush* brushBorderStrong = nullptr;
    ID2D1SolidColorBrush* brushTextPrimary = nullptr;
    ID2D1SolidColorBrush* brushTextSecondary = nullptr;
    ID2D1SolidColorBrush* brushTextTertiary = nullptr;
    ID2D1SolidColorBrush* brushAccent = nullptr;
    ID2D1SolidColorBrush* brushAccentSubtle = nullptr;
    ID2D1SolidColorBrush* brushSuccess = nullptr;

    // Text formats
    IDWriteTextFormat* textFormatSmall = nullptr;      // 11px
    IDWriteTextFormat* textFormatBody = nullptr;       // 12.5px
    IDWriteTextFormat* textFormatMedium = nullptr;     // 13px
    IDWriteTextFormat* textFormatTitle = nullptr;      // 15px
    IDWriteTextFormat* textFormatHeading = nullptr;    // 18px
    IDWriteTextFormat* textFormatMono = nullptr;       // 11px mono

    // Methods
    HRESULT Initialize(ID2D1RenderTarget* rt, IDWriteFactory* dwFactory);
    void Release();

    D2D1_COLOR_F GetAccentColor() const { return Color::FromHex(DesignTokens::ACCENT); }
    D2D1_COLOR_F GetAppBg() const { return Color::FromHex(DesignTokens::APP_BG); }
    D2D1_COLOR_F GetSurface() const { return Color::FromHex(DesignTokens::SURFACE); }
};

} // namespace PdfElite
