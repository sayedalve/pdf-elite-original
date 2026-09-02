// PdfCanvas.h - PDF rendering boundary, NEVER draws outside clip
// Critical: PDF renderer must never overwrite TopBar, Tabs, Toolbar, Sidebar, StatusBar
#pragma once
#include "Theme.h"
#include "LayoutManager.h"
#include <d2d1.h>
#include <wrl/client.h>

namespace PdfElite {

class PdfDocument; // Forward - engine interface

class PdfCanvas {
public:
    HRESULT Initialize(ID2D1RenderTarget* rt, IDWriteFactory* dwFactory, Theme* theme);
    void Release();

    // Render with strict clipping - boundary safety
    void Render(ID2D1RenderTarget* rt, const Layout& layout, PdfDocument* doc, float zoom, int currentPage);

    // Input
    void SetZoom(float z) { m_zoom = z; }
    float GetZoom() const { return m_zoom; }

private:
    void RenderSinglePage(ID2D1RenderTarget* rt, const D2D1_RECT_F& canvasRect, PdfDocument* doc, int page);
    void RenderGrid(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect);

    Theme* m_theme = nullptr;
    ID2D1RenderTarget* m_rt = nullptr;
    float m_zoom = 0.77f; // 77% as in screenshots

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_gridBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_whiteBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_blackBrush;
};

} // namespace PdfElite
