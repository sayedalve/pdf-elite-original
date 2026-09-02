// DocumentView.cpp - Implements thumbnail grid and canvas integration
#include "DocumentView.h"

namespace PdfElite {

HRESULT DocumentView::Initialize(ID2D1RenderTarget* rt, IDWriteFactory* dwFactory, Theme* theme, CommandManager* cmdMgr) {
    m_theme = theme;
    m_cmdMgr = cmdMgr;

    HRESULT hr = m_canvas.Initialize(rt, dwFactory, theme);
    if (FAILED(hr)) return hr;

    hr = rt->CreateSolidColorBrush(D2D1::ColorF(1,1,1,0.06f), &m_thumbBorder);

    // Exact titles from screenshot 4
    m_thumbs = {
        {1, L"Introduction to Prestressed Concrete & Materials", true},
        {2, L"Introduction", false},
        {3, L"Normal Reinforced Concrete vs Prestressed", false},
        {4, L"Advantages of PC over RC", false},
        {5, L"Disadvantages of PC", false},
        {6, L"Example of PC structures", false},
        {7, L"Example of PC structures", false},
        {8, L"Example of PC structures", false},
        {9, L"Example of PC structures", false},
    };

    return hr;
}

void DocumentView::Release() {
    m_canvas.Release();
    m_thumbBorder.Reset();
}

void DocumentView::Render(ID2D1RenderTarget* rt, const Layout& layout, PdfDocument* doc, DocViewMode mode, int selectedThumb) {
    if (mode == DocViewMode::SinglePage) {
        m_canvas.Render(rt, layout, doc, 0.77f, 4);
    } else {
        RenderThumbnailGrid(rt, layout);
    }
}

void DocumentView::RenderThumbnailGrid(ID2D1RenderTarget* rt, const Layout& layout) {
    // 3 cols gap 24px thumb radius 10 bg #1c1f2b border 0.06 selected border 2px #7c9cff
    float gap = Metrics::THUMB_GAP;
    float cols = Metrics::THUMB_COLS;
    float availableW = layout.center.right - layout.center.left - 80.0f;
    float thumbW = (availableW - gap * (cols - 1)) / cols;
    float thumbH = thumbW * 1.4f;

    float x0 = layout.center.left + 40.0f;
    float y0 = layout.center.top + 40.0f;

    for (size_t i = 0; i < m_thumbs.size(); ++i) {
        int col = i % 3;
        int row = i / 3;
        float x = x0 + col * (thumbW + gap);
        float y = y0 + row * (thumbH + 60.0f + gap);

        D2D1_RECT_F thumbRect = D2D1::RectF(x, y, x + thumbW, y + thumbH);
        bool selected = (int)i == 0; // Page 1 selected as in screenshot

        D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(thumbRect, Metrics::THUMB_RADIUS, Metrics::THUMB_RADIUS);
        rt->FillRoundedRectangle(rr, m_theme->brushSurface);
        if (selected) {
            rt->DrawRoundedRectangle(rr, m_theme->brushAccent, 2.0f);
            // Top actions rotate/delete
            D2D1_RECT_F actions = D2D1::RectF(x + thumbW - 80, y - 28, x + thumbW, y - 4);
            D2D1_ROUNDED_RECT actRR = D2D1::RoundedRect(actions, 6, 6);
            rt->FillRoundedRectangle(actRR, m_theme->brushElevated);
        } else {
            rt->DrawRoundedRectangle(rr, m_thumbBorder.Get(), 1.0f);
        }

        // Page number
        D2D1_RECT_F numRect = D2D1::RectF(x, y + thumbH + 8, x + thumbW, y + thumbH + 24);
        std::wstring num = std::to_wstring(m_thumbs[i].pageNumber);
        auto brush = selected ? m_theme->brushTextPrimary : m_theme->brushTextTertiary;
        if (m_theme->fmtSmallUpper) rt->DrawText(num.c_str(), (UINT32)num.length(), m_theme->fmtSmallUpper, numRect, brush);
    }
}

bool DocumentView::HitTestThumbnail(int x, int y, const Layout& layout, int& outIndex) {
    float gap = Metrics::THUMB_GAP;
    float cols = Metrics::THUMB_COLS;
    float availableW = layout.center.right - layout.center.left - 80.0f;
    float thumbW = (availableW - gap * (cols - 1)) / cols;
    float thumbH = thumbW * 1.4f;
    float x0 = layout.center.left + 40.0f;
    float y0 = layout.center.top + 40.0f;

    for (size_t i = 0; i < m_thumbs.size(); ++i) {
        int col = i % 3;
        int row = i / 3;
        float tx = x0 + col * (thumbW + gap);
        float ty = y0 + row * (thumbH + 60.0f + gap);
        if (x >= tx && x <= tx + thumbW && y >= ty && y <= ty + thumbH) {
            outIndex = (int)i;
            return true;
        }
    }
    return false;
}

} // namespace PdfElite
