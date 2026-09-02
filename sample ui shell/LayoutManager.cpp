// LayoutManager.cpp - Implements deterministic layout
#include "LayoutManager.h"

namespace PdfElite {

Layout LayoutManager::CalculateViewer(const D2D1_RECT_F& client, float zoom) {
    Layout l{};
    float w = client.right - client.left;
    float h = client.bottom - client.top;

    l.topBar = D2D1::RectF(client.left, client.top, client.right, client.top + Metrics::TOPBAR_H);
    l.toolbar = D2D1::RectF(client.left + Metrics::LEFT_RAIL_W, l.topBar.bottom, client.right - Metrics::RIGHT_RAIL_W, l.topBar.bottom + Metrics::TOOLBAR_H);
    l.leftRail = D2D1::RectF(client.left, l.topBar.bottom, client.left + Metrics::LEFT_RAIL_W, client.bottom - Metrics::STATUS_H);
    l.rightRail = D2D1::RectF(client.right - Metrics::RIGHT_RAIL_W, l.topBar.bottom, client.right, client.bottom - Metrics::STATUS_H);
    l.center = D2D1::RectF(l.leftRail.right, l.toolbar.bottom, l.rightRail.left, client.bottom - Metrics::STATUS_H);
    l.statusBar = D2D1::RectF(client.left, client.bottom - Metrics::STATUS_H, client.right, client.bottom);

    // PDF canvas is inset 40px top, centered - clip rect
    float canvasPadding = Metrics::SPACING_3XL;
    l.pdfCanvas = D2D1::RectF(l.center.left + canvasPadding, l.center.top + Metrics::PDF_MARGIN_TOP,
                              l.center.right - canvasPadding, l.center.bottom - canvasPadding);

    // Narrow responsive: hide right rail
    if (IsNarrow(w)) {
        l.rightRail = D2D1::RectF(0,0,0,0);
        l.toolbar.right = client.right;
        l.center.right = client.right;
        l.pdfCanvas.right = client.right - canvasPadding;
    }

    return l;
}

Layout LayoutManager::CalculateHome(const D2D1_RECT_F& client) {
    Layout l{};
    l.sidebarHome = D2D1::RectF(client.left, client.top, client.left + Metrics::SIDEBAR_HOME_W, client.bottom);
    l.mainHome = D2D1::RectF(l.sidebarHome.right, client.top, client.right, client.bottom);

    float contentLeft = l.mainHome.left + Metrics::HOME_PADDING;
    float contentRight = l.mainHome.right - Metrics::HOME_PADDING;
    float y = l.mainHome.top + Metrics::SPACING_XL;

    // Workspace header 80px
    y += 80.0f;

    // Quick Tools 260px
    l.quickTools = D2D1::RectF(contentLeft, y, contentRight, y + 260.0f);
    y += 260.0f + Metrics::HOME_SECTION_GAP;

    // Recent Files remaining
    l.recentFiles = D2D1::RectF(contentLeft, y, contentRight, client.bottom - Metrics::SPACING_XL);

    return l;
}

D2D1_RECT_F LayoutManager::CalculatePdfPage(const D2D1_RECT_F& canvas, float pageW, float pageH, float zoom) {
    float w = pageW * zoom;
    float h = pageH * zoom;
    float x = canvas.left + (canvas.right - canvas.left - w) * 0.5f;
    float y = canvas.top;
    return D2D1::RectF(x, y, x + w, y + h);
}

} // namespace PdfElite
