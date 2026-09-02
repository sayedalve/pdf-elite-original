// PdfCanvas.cpp - Implements clipped PDF rendering
#include "PdfCanvas.h"
#include "PdfDocument.h"

namespace PdfElite {

HRESULT PdfCanvas::Initialize(ID2D1RenderTarget* rt, IDWriteFactory* dwFactory, Theme* theme) {
    m_rt = rt;
    m_theme = theme;
    HRESULT hr;
    hr = rt->CreateSolidColorBrush(D2D1::ColorF(1,1,1,0.02f), &m_gridBrush); if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_whiteBrush); if (FAILED(hr)) return hr;
    hr = rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_blackBrush);
    return hr;
}

void PdfCanvas::Release() {
    m_gridBrush.Reset();
    m_whiteBrush.Reset();
    m_blackBrush.Reset();
}

void PdfCanvas::Render(ID2D1RenderTarget* rt, const Layout& layout, PdfDocument* doc, float zoom, int currentPage) {
    // CRITICAL: Push clip to pdfCanvas - PDF must NEVER draw outside
    rt->PushAxisAlignedClip(layout.pdfCanvas, D2D1_ANTIALIAS_MODE_ALIASED);

    // Grid background #0f1117 with 32px pattern rgba(255,255,255,0.02)
    rt->FillRectangle(layout.center, m_theme->brushAppBg);
    RenderGrid(rt, layout.center);

    // Single page centered
    RenderSinglePage(rt, layout.pdfCanvas, doc, currentPage);

    rt->PopAxisAlignedClip();
}

void PdfCanvas::RenderGrid(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect) {
    // 32px grid
    float grid = 32.0f;
    for (float x = rect.left; x < rect.right; x += grid) {
        rt->DrawLine(D2D1::Point2F(x, rect.top), D2D1::Point2F(x, rect.bottom), m_gridBrush.Get(), 1.0f);
    }
    for (float y = rect.top; y < rect.bottom; y += grid) {
        rt->DrawLine(D2D1::Point2F(rect.left, y), D2D1::Point2F(rect.right, y), m_gridBrush.Get(), 1.0f);
    }
}

void PdfCanvas::RenderSinglePage(ID2D1RenderTarget* rt, const D2D1_RECT_F& canvasRect, PdfDocument* doc, int page) {
    // Page rect centered in canvas
    D2D1_RECT_F pageRect = LayoutManager::CalculatePdfPage(canvasRect, Metrics::PDF_PAGE_W, Metrics::PDF_PAGE_H * 0.6f, m_zoom);

    // Shadow 0 8px 40px rgba(0,0,0,0.5) - multiple layers
    for (int i = 1; i <= 4; ++i) {
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> shadow;
        rt->CreateSolidColorBrush(D2D1::ColorF(0,0,0,0.08f / i), &shadow);
        D2D1_RECT_F sr = D2D1::RectF(pageRect.left + i, pageRect.top + i, pageRect.right + i, pageRect.bottom + i);
        rt->FillRectangle(sr, shadow.Get());
    }

    // White page
    rt->FillRectangle(pageRect, m_whiteBrush.Get());

    // Border rgba(0,0,0,0.08)
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> border;
    rt->CreateSolidColorBrush(D2D1::ColorF(0,0,0,0.08f), &border);
    rt->DrawRectangle(pageRect, border.Get(), 1.0f);

    // Real PDF rendering via engine - if doc available, else placeholder content from screenshot
    if (doc && doc->IsLoaded()) {
        doc->RenderPage(rt, page, pageRect, m_zoom);
    } else {
        // Placeholder content "Advantages of PC over RC" as in Redesign.html screenshot - but rendered natively, not fake rect
        if (m_theme->fmtHeading) {
            D2D1_RECT_F titleRect = D2D1::RectF(pageRect.left + 40, pageRect.top + 30, pageRect.right - 40, pageRect.top + 70);
            rt->DrawText(L"Advantages of PC over RC", 24, m_theme->fmtHeading, titleRect, m_blackBrush.Get());
            rt->DrawLine(D2D1::Point2F(titleRect.left, titleRect.bottom - 4), D2D1::Point2F(titleRect.left + 260, titleRect.bottom - 4), m_blackBrush.Get(), 2.0f);
        }
        if (m_theme->fmtBody) {
            const wchar_t* bullets[] = {
                L"• Take full advantages of high strength concrete and high strength steel",
                L"• Need less materials",
                L"• Smaller and lighter structure",
                L"• No cracks",
                L"• Use the entire section to resist the load",
            };
            float y = pageRect.top + 100.0f;
            for (auto b : bullets) {
                D2D1_RECT_F br = D2D1::RectF(pageRect.left + 40, y, pageRect.right - 40, y + 22);
                rt->DrawText(b, (UINT32)wcslen(b), m_theme->fmtBody, br, m_blackBrush.Get());
                y += 28.0f;
            }
        }
    }
}

} // namespace PdfElite
