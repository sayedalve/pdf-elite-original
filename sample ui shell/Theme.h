// Theme.h - Exact Wondershare PDFelement target colors from your 5 reference images
#pragma once
#include <d2d1.h>
#include <dwrite.h>
#include <cstdint>

namespace PdfElite {

namespace Colors {
    // Exact from target: dark purple-blue, not black #0f1117
    constexpr uint32_t APP_BG = 0x1e1e2f;          // Main workspace #1e1e2f
    constexpr uint32_t SIDEBAR_BG = 0x171729;       // Left nav #171729
    constexpr uint32_t SURFACE = 0x28283e;         // Cards, toolbar #28283e
    constexpr uint32_t ELEVATED = 0x32324e;        // Hover #32324e
    constexpr uint32_t CARD = 0x2a2a40;             // Quick Tools bg
    constexpr uint32_t CARD_HOVER = 0x33334f;
    constexpr uint32_t TOPBAR_BG = 0x1c1c2e;        // Top bar
    constexpr uint32_t TOOLBAR_BG = 0x2b2b42;       // Toolbar
    constexpr uint32_t TEXT_PRIMARY = 0xe8e8f0;
    constexpr uint32_t TEXT_SECONDARY = 0x9a9ab0;
    constexpr uint32_t TEXT_TERTIARY = 0x6b6b80;
    constexpr uint32_t ACCENT = 0x6b8cff;           // Blue avatar, selection
    constexpr uint32_t ACCENT_HOVER = 0x7d9bff;
    constexpr uint32_t WHITE = 0xffffff;
    constexpr uint32_t SUCCESS = 0x4ade80;
    // Quick Tools icon colors - exact from target
    constexpr uint32_t ICON_EDIT = 0xf59e0b;        // Orange
    constexpr uint32_t ICON_CONVERT = 0x10b981;     // Green teal
    constexpr uint32_t ICON_OCR = 0x14b8a6;
    constexpr uint32_t ICON_COMMENT = 0xf87171;     // Red
    constexpr uint32_t ICON_TRANSLATE = 0x8b5cf6;   // Purple blue
    constexpr uint32_t ICON_COMBINE = 0x3b82f6;     // Blue
    constexpr uint32_t ICON_COMPRESS = 0x22c55e;    // Green
    constexpr uint32_t ICON_BATCH = 0x10b981;
}

namespace Typography {
    constexpr float SIZE_11 = 11.0f;
    constexpr float SIZE_12 = 12.0f;
    constexpr float SIZE_13 = 13.0f;
    constexpr float SIZE_15 = 15.0f;
    constexpr float SIZE_18 = 18.0f;
}
namespace Metrics {
    constexpr float WINDOW_DEFAULT_W = 1440.0f;
    constexpr float WINDOW_DEFAULT_H = 900.0f;
    constexpr float SIDEBAR_HOME_W = 240.0f;
    constexpr float TOPBAR_H = 48.0f;
    constexpr float TAB_H = 30.0f;
    constexpr float TOOLBAR_H = 48.0f;
    constexpr float LEFT_RAIL_W = 64.0f;
    constexpr float RIGHT_RAIL_W = 48.0f;
    constexpr float STATUS_H = 24.0f;
    constexpr float RADIUS_10 = 10.0f;
    constexpr float RADIUS_12 = 12.0f;
    constexpr float SPACING_3XL = 40.0f;
    constexpr float SPACING_XL = 24.0f;
    constexpr float PDF_MARGIN_TOP = 40.0f;
    constexpr float HOME_SECTION_GAP = 32.0f;
    constexpr float PDF_PAGE_W = 794.0f;
    constexpr float PDF_PAGE_H = 1123.0f;
    constexpr float THUMB_GAP = 24.0f;
    constexpr float THUMB_COLS = 3.0f;
    constexpr float THUMB_RADIUS = 10.0f;
    constexpr float QUICKTOOLS_CARD_H = 110.0f;
    constexpr float QUICKTOOLS_RADIUS = 16.0f;
    constexpr float HOME_PADDING = 32.0f;
    constexpr float QUICKTOOLS_GAP = 16.0f;
    constexpr float RECENT_ROW_H = 48.0f;
    constexpr float RADIUS_8 = 8.0f;
}

struct Theme {
    ID2D1SolidColorBrush* brushAppBg = nullptr;
    ID2D1SolidColorBrush* brushSidebarBg = nullptr;
    ID2D1SolidColorBrush* brushSurface = nullptr;
    ID2D1SolidColorBrush* brushElevated = nullptr;
    ID2D1SolidColorBrush* brushCard = nullptr;
    ID2D1SolidColorBrush* brushTopBar = nullptr;
    ID2D1SolidColorBrush* brushToolbar = nullptr;
    ID2D1SolidColorBrush* brushBorder = nullptr;
    ID2D1SolidColorBrush* brushBorderStrong = nullptr;
    ID2D1SolidColorBrush* brushCardHover = nullptr;
    ID2D1SolidColorBrush* brushAccentSubtle = nullptr;
    ID2D1SolidColorBrush* brushTextPrimary = nullptr;
    ID2D1SolidColorBrush* brushTextSecondary = nullptr;
    ID2D1SolidColorBrush* brushTextTertiary = nullptr;
    ID2D1SolidColorBrush* brushAccent = nullptr;
    ID2D1SolidColorBrush* brushWhite = nullptr;
    ID2D1SolidColorBrush* brushWhiteSubtle = nullptr;

    // Quick Tools brushes
    ID2D1SolidColorBrush* brushIconEdit = nullptr;
    ID2D1SolidColorBrush* brushIconConvert = nullptr;
    ID2D1SolidColorBrush* brushIconOcr = nullptr;
    ID2D1SolidColorBrush* brushIconComment = nullptr;

    IDWriteTextFormat* fmtSmallUpper = nullptr;
    IDWriteTextFormat* fmtSmall = nullptr;      // 11px
    IDWriteTextFormat* fmtBody = nullptr;       // 13px
    IDWriteTextFormat* fmtMedium = nullptr;     // 14px medium
    IDWriteTextFormat* fmtBold = nullptr;       // 14px bold
    IDWriteTextFormat* fmtTitle = nullptr;      // 16px bold
    IDWriteTextFormat* fmtHeading = nullptr;    // 20px bold

    static D2D1_COLOR_F FromHex(uint32_t hex, float a = 1.0f) {
        return D2D1::ColorF(((hex>>16)&0xFF)/255.0f, ((hex>>8)&0xFF)/255.0f, (hex&0xFF)/255.0f, a);
    }

    HRESULT Initialize(ID2D1RenderTarget* rt, IDWriteFactory* dwFactory);
    void Release();
};

} // namespace PdfElite
