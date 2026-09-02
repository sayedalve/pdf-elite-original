#include "viewport/ViewportEngine.h"
#include <cmath>
#include <algorithm>

namespace ui::viewport {

const std::vector<double> ViewportEngine::PRESET_ZOOM_LEVELS = {
    0.01, 0.05, 0.10, 0.25, 0.333, 0.50, 0.667, 0.75, 1.00,
    1.25, 1.50, 2.00, 3.00, 4.00, 6.00, 8.00, 12.00, 16.00,
    24.00, 32.00, 48.00, 64.00
};

ViewportEngine::ViewportEngine()
    : m_zoom(DEFAULT_ZOOM),
      m_fitMode(FitMode::None),
      m_scrollX(0.0),
      m_scrollY(0.0),
      m_viewportWidth(800.0),
      m_viewportHeight(600.0),
      m_totalContentWidth(0.0),
      m_totalContentHeight(0.0) {}

bool ViewportEngine::SetZoom(double zoom) {
    double clamped = std::clamp(zoom, MIN_ZOOM, MAX_ZOOM);
    if (std::abs(m_zoom - clamped) < 0.0001) return false;
    m_zoom = clamped;
    return true;
}

double ViewportEngine::ZoomIn(int steps) {
    if (steps <= 0) return m_zoom;
    double factor = std::pow(ZOOM_STEP_FACTOR, steps);
    SetZoom(m_zoom * factor);
    return m_zoom;
}

double ViewportEngine::ZoomOut(int steps) {
    if (steps <= 0) return m_zoom;
    double factor = std::pow(ZOOM_STEP_FACTOR, steps);
    SetZoom(m_zoom / factor);
    return m_zoom;
}

double ViewportEngine::GetNextPresetZoom(bool zoomIn) const {
    if (zoomIn) {
        for (double preset : PRESET_ZOOM_LEVELS) {
            if (preset > m_zoom + 0.001) {
                return preset;
            }
        }
        return MAX_ZOOM;
    } else {
        for (auto it = PRESET_ZOOM_LEVELS.rbegin(); it != PRESET_ZOOM_LEVELS.rend(); ++it) {
            if (*it < m_zoom - 0.001) {
                return *it;
            }
        }
        return MIN_ZOOM;
    }
}

void ViewportEngine::CalculateFocalZoomOffsets(double currentZoom, double newZoom,
                                              double focalScreenX, double focalScreenY,
                                              double currentScrollX, double currentScrollY,
                                              double& outScrollX, double& outScrollY,
                                              double pageOffsetX, double pageOffsetY) {
    if (currentZoom <= 0.0001) {
        outScrollX = currentScrollX;
        outScrollY = currentScrollY;
        return;
    }

    double ratio = newZoom / currentZoom;
    outScrollX = (focalScreenX + currentScrollX - pageOffsetX) * ratio - (focalScreenX - pageOffsetX);
    outScrollY = (focalScreenY + currentScrollY - pageOffsetY) * ratio - (focalScreenY - pageOffsetY);
}

bool ViewportEngine::ApplyFocalZoom(double newZoom, double focalScreenX, double focalScreenY,
                                   double pageOffsetX, double pageOffsetY) {
    double targetZoom = std::clamp(newZoom, MIN_ZOOM, MAX_ZOOM);
    if (std::abs(m_zoom - targetZoom) < 0.0001) return false;

    double newScrollX = m_scrollX;
    double newScrollY = m_scrollY;
    CalculateFocalZoomOffsets(m_zoom, targetZoom, focalScreenX, focalScreenY,
                              m_scrollX, m_scrollY, newScrollX, newScrollY,
                              pageOffsetX, pageOffsetY);

    m_zoom = targetZoom;
    ClampScroll(newScrollX, newScrollY);
    m_scrollX = newScrollX;
    m_scrollY = newScrollY;
    return true;
}

double ViewportEngine::CalculateFitWidth(double contentWidth, double viewportWidth, double padding) const {
    if (contentWidth <= 0.0) return DEFAULT_ZOOM;
    double avail = std::max(10.0, viewportWidth - padding);
    return std::clamp(avail / contentWidth, MIN_ZOOM, MAX_ZOOM);
}

double ViewportEngine::CalculateFitPage(double contentWidth, double contentHeight,
                                      double viewportWidth, double viewportHeight,
                                      double padding) const {
    if (contentWidth <= 0.0 || contentHeight <= 0.0) return DEFAULT_ZOOM;
    double availW = std::max(10.0, viewportWidth - padding);
    double availH = std::max(10.0, viewportHeight - padding);
    double zw = availW / contentWidth;
    double zh = availH / contentHeight;
    return std::clamp(std::min(zw, zh), MIN_ZOOM, MAX_ZOOM);
}

double ViewportEngine::CalculateFitHeight(double contentHeight, double viewportHeight, double padding) const {
    if (contentHeight <= 0.0) return DEFAULT_ZOOM;
    double availH = std::max(10.0, viewportHeight - padding);
    return std::clamp(availH / contentHeight, MIN_ZOOM, MAX_ZOOM);
}

void ViewportEngine::SetScroll(double scrollX, double scrollY) {
    double sx = scrollX;
    double sy = scrollY;
    ClampScroll(sx, sy);
    m_scrollX = sx;
    m_scrollY = sy;
}

void ViewportEngine::ScrollBy(double deltaX, double deltaY) {
    SetScroll(m_scrollX + deltaX, m_scrollY + deltaY);
}

void ViewportEngine::SetViewportSize(double width, double height) {
    m_viewportWidth = std::max(1.0, width);
    m_viewportHeight = std::max(1.0, height);
    ClampScroll(m_scrollX, m_scrollY);
}

void ViewportEngine::ClampScroll(double& inOutScrollX, double& inOutScrollY) const {
    double maxScrollX = std::max(0.0, m_totalContentWidth - m_viewportWidth);
    double maxScrollY = std::max(0.0, m_totalContentHeight - m_viewportHeight);
    inOutScrollX = std::clamp(inOutScrollX, 0.0, maxScrollX);
    inOutScrollY = std::clamp(inOutScrollY, 0.0, maxScrollY);
}

void ViewportEngine::UpdateContinuousLayout(const std::vector<std::pair<double, double>>& pageSizes,
                                            const std::vector<int>& rotations,
                                            double pageGap) {
    m_pageLayouts.clear();
    m_pageLayouts.reserve(pageSizes.size());

    double currentY = pageGap;
    double maxWidth = 0.0;

    for (size_t i = 0; i < pageSizes.size(); ++i) {
        int rot = (i < rotations.size()) ? rotations[i] : 0;
        bool isLandscape = (rot == 90 || rot == 270);

        double unscaledW = isLandscape ? pageSizes[i].second : pageSizes[i].first;
        double unscaledH = isLandscape ? pageSizes[i].first : pageSizes[i].second;

        double scaledW = unscaledW * m_zoom;
        double scaledH = unscaledH * m_zoom;

        double xOffset = 0.0;
        if (scaledW < m_viewportWidth) {
            xOffset = (m_viewportWidth - scaledW) / 2.0;
        }

        ViewportPageInfo info;
        info.index = static_cast<int>(i);
        info.unscaledWidth = pageSizes[i].first;
        info.unscaledHeight = pageSizes[i].second;
        info.rotation = rot;
        info.xOffset = xOffset;
        info.yOffset = currentY;
        info.scaledWidth = scaledW;
        info.scaledHeight = scaledH;

        m_pageLayouts.push_back(info);

        currentY += scaledH + pageGap;
        maxWidth = std::max(maxWidth, scaledW);
    }

    m_totalContentWidth = maxWidth;
    m_totalContentHeight = currentY;
    ClampScroll(m_scrollX, m_scrollY);
}

bool ViewportEngine::GetVisiblePageRange(int& outStartPage, int& outEndPage) const {
    if (m_pageLayouts.empty()) {
        outStartPage = -1;
        outEndPage = -1;
        return false;
    }

    outStartPage = -1;
    outEndPage = -1;

    double viewTop = m_scrollY;
    double viewBottom = m_scrollY + m_viewportHeight;

    for (const auto& layout : m_pageLayouts) {
        double pageTop = layout.yOffset;
        double pageBottom = layout.yOffset + layout.scaledHeight;

        if (pageBottom >= viewTop && pageTop <= viewBottom) {
            if (outStartPage == -1) {
                outStartPage = layout.index;
            }
            outEndPage = layout.index;
        }
    }

    if (outStartPage == -1) {
        outStartPage = 0;
        outEndPage = 0;
    }

    return true;
}

int ViewportEngine::GetPageAtOffset(double offsetY) const {
    if (m_pageLayouts.empty()) return 0;
    if (offsetY < m_pageLayouts.front().yOffset) {
        return m_pageLayouts.front().index;
    }
    for (size_t i = 0; i < m_pageLayouts.size(); ++i) {
        double pageTop = m_pageLayouts[i].yOffset;
        double nextTop = (i + 1 < m_pageLayouts.size())
            ? m_pageLayouts[i + 1].yOffset
            : (pageTop + m_pageLayouts[i].scaledHeight);
        if (offsetY >= pageTop && offsetY < nextTop) {
            return m_pageLayouts[i].index;
        }
    }
    return m_pageLayouts.back().index;
}

double ViewportEngine::GetScrollYForPage(int pageIndex) const {
    if (pageIndex < 0 || pageIndex >= static_cast<int>(m_pageLayouts.size())) return 0.0;
    return m_pageLayouts[pageIndex].yOffset;
}

} // namespace ui::viewport
