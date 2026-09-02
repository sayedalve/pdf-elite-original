// NativeDesignSystem.cpp - Native C++ implementation of better colors UI
// Translates exact HTML UI (#0f1117 / #151821 / #1c1f2b / #7c9cff) to Direct2D/DirectWrite
// No WebView, no HTML at runtime - pure native rendering

#include "NativeDesignSystem.h"

namespace PdfElite {

HRESULT DesignSystem::Initialize(ID2D1RenderTarget* rt, IDWriteFactory* dwFactory) {
    HRESULT hr = S_OK;

    // Create brushes from exact HTML tokens
    hr = rt->CreateSolidColorBrush(Color::FromHex(DesignTokens::APP_BG), &brushAppBg);
    if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(Color::FromHex(DesignTokens::SIDEBAR_BG), &brushSidebarBg);
    if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(Color::FromHex(DesignTokens::SURFACE), &brushSurface);
    if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(Color::FromHex(DesignTokens::ELEVATED), &brushElevated);
    if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(Color::FromHex(DesignTokens::CARD), &brushCard);
    if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(Color::FromHex(DesignTokens::CARD_HOVER), &brushCardHover);
    if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(DesignTokens::BORDER, &brushBorder);
    if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(DesignTokens::BORDER_STRONG, &brushBorderStrong);
    if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(Color::FromHex(DesignTokens::TEXT_PRIMARY), &brushTextPrimary);
    if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(Color::FromHex(DesignTokens::TEXT_SECONDARY), &brushTextSecondary);
    if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(Color::FromHex(DesignTokens::TEXT_TERTIARY), &brushTextTertiary);
    if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(Color::FromHex(DesignTokens::ACCENT), &brushAccent);
    if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(DesignTokens::ACCENT_SUBTLE, &brushAccentSubtle);
    if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(Color::FromHex(DesignTokens::SUCCESS), &brushSuccess);
    if (FAILED(hr)) return hr;

    // Typography - Inter / Segoe UI as in HTML
    // Small: 11px uppercase for labels
    hr = dwFactory->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        11.0f, L"en-us", &textFormatSmall);
    if (FAILED(hr)) return hr;

    // Body: 12.5px
    hr = dwFactory->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        12.5f, L"en-us", &textFormatBody);
    if (FAILED(hr)) return hr;

    // Medium: 13px
    hr = dwFactory->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        13.0f, L"en-us", &textFormatMedium);
    if (FAILED(hr)) return hr;

    // Title: 15px bold
    hr = dwFactory->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        15.0f, L"en-us", &textFormatTitle);
    if (FAILED(hr)) return hr;

    // Heading: 18px
    hr = dwFactory->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        18.0f, L"en-us", &textFormatHeading);
    if (FAILED(hr)) return hr;

    // Mono: 11px
    hr = dwFactory->CreateTextFormat(
        L"Cascadia Mono", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        11.0f, L"en-us", &textFormatMono);
    if (FAILED(hr)) {
        // Fallback to Consolas
        hr = dwFactory->CreateTextFormat(
            L"Consolas", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            11.0f, L"en-us", &textFormatMono);
    }

    return S_OK;
}

void DesignSystem::Release() {
    if (brushAppBg) { brushAppBg->Release(); brushAppBg = nullptr; }
    if (brushSidebarBg) { brushSidebarBg->Release(); brushSidebarBg = nullptr; }
    if (brushSurface) { brushSurface->Release(); brushSurface = nullptr; }
    if (brushElevated) { brushElevated->Release(); brushElevated = nullptr; }
    if (brushCard) { brushCard->Release(); brushCard = nullptr; }
    if (brushCardHover) { brushCardHover->Release(); brushCardHover = nullptr; }
    if (brushBorder) { brushBorder->Release(); brushBorder = nullptr; }
    if (brushBorderStrong) { brushBorderStrong->Release(); brushBorderStrong = nullptr; }
    if (brushTextPrimary) { brushTextPrimary->Release(); brushTextPrimary = nullptr; }
    if (brushTextSecondary) { brushTextSecondary->Release(); brushTextSecondary = nullptr; }
    if (brushTextTertiary) { brushTextTertiary->Release(); brushTextTertiary = nullptr; }
    if (brushAccent) { brushAccent->Release(); brushAccent = nullptr; }
    if (brushAccentSubtle) { brushAccentSubtle->Release(); brushAccentSubtle = nullptr; }
    if (brushSuccess) { brushSuccess->Release(); brushSuccess = nullptr; }

    if (textFormatSmall) { textFormatSmall->Release(); textFormatSmall = nullptr; }
    if (textFormatBody) { textFormatBody->Release(); textFormatBody = nullptr; }
    if (textFormatMedium) { textFormatMedium->Release(); textFormatMedium = nullptr; }
    if (textFormatTitle) { textFormatTitle->Release(); textFormatTitle = nullptr; }
    if (textFormatHeading) { textFormatHeading->Release(); textFormatHeading = nullptr; }
    if (textFormatMono) { textFormatMono->Release(); textFormatMono = nullptr; }
}

} // namespace PdfElite
