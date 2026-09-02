// Theme.cpp - Implements exact better colors
#include "Theme.h"

namespace PdfElite {

HRESULT Theme::Initialize(ID2D1RenderTarget* rt, IDWriteFactory* dwFactory) {
    HRESULT hr;
    hr = rt->CreateSolidColorBrush(FromHex(Colors::APP_BG), &brushAppBg); if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(FromHex(Colors::SIDEBAR_BG), &brushTopBar);
    hr = rt->CreateSolidColorBrush(FromHex(Colors::SURFACE), &brushToolbar);
    hr = rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &brushWhite);
    hr = rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.5f), &brushWhiteSubtle);
    hr = rt->CreateSolidColorBrush(FromHex(Colors::SIDEBAR_BG), &brushSidebarBg); if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(FromHex(Colors::SURFACE), &brushSurface); if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(FromHex(Colors::ELEVATED), &brushElevated); if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(FromHex(Colors::CARD), &brushCard); if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(FromHex(Colors::CARD_HOVER), &brushCardHover); if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(D2D1::ColorF(1,1,1,0.06f), &brushBorder); if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(D2D1::ColorF(1,1,1,0.10f), &brushBorderStrong); if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &brushTextPrimary); if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.85f, 0.85f, 1.0f), &brushTextSecondary); if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(D2D1::ColorF(0.70f, 0.70f, 0.70f, 1.0f), &brushTextTertiary); if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(FromHex(Colors::ACCENT), &brushAccent); if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(D2D1::ColorF(0x7c/255.0f,0x9c/255.0f,1,0.12f), &brushAccentSubtle); if (FAILED(hr)) return hr;

    hr = dwFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, Typography::SIZE_11, L"en-us", &fmtSmallUpper);
    if (FAILED(hr)) return hr;
    hr = dwFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, Typography::SIZE_11, L"en-us", &fmtSmall);
    hr = dwFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, Typography::SIZE_13, L"en-us", &fmtBold);
    hr = dwFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, Typography::SIZE_12, L"en-us", &fmtBody); if (FAILED(hr)) return hr;
    hr = dwFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, Typography::SIZE_13, L"en-us", &fmtMedium); if (FAILED(hr)) return hr;
    hr = dwFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, Typography::SIZE_15, L"en-us", &fmtTitle); if (FAILED(hr)) return hr;
    hr = dwFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, Typography::SIZE_18, L"en-us", &fmtHeading);
    return hr;
}

void Theme::Release() {
#define REL(x) if(x){x->Release(); x=nullptr;}
    REL(brushAppBg); REL(brushSidebarBg); REL(brushSurface); REL(brushElevated); REL(brushCard); REL(brushCardHover); REL(brushTopBar); REL(brushToolbar); REL(brushWhite); REL(brushWhiteSubtle);
    REL(brushBorder); REL(brushBorderStrong); REL(brushTextPrimary); REL(brushTextSecondary); REL(brushTextTertiary);
    REL(brushAccent); REL(brushAccentSubtle);
    REL(fmtSmallUpper); REL(fmtSmall); REL(fmtBold); REL(fmtBody); REL(fmtMedium); REL(fmtTitle); REL(fmtHeading);
#undef REL
}

} // namespace PdfElite
