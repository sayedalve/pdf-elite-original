#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include "core/Geometry.h"
#include "core/CoordinateConverter.h"

namespace ui::viewport {

enum class FitMode {
    None,
    FitWidth,
    FitPage,
    FitHeight,
    FitVisible
};

struct ViewportPageInfo {
    int index = 0;
    double unscaledWidth = 0.0;
    double unscaledHeight = 0.0;
    int rotation = 0;
    double xOffset = 0.0;
    double yOffset = 0.0;
    double scaledWidth = 0.0;
    double scaledHeight = 0.0;
};

class ViewportEngine {
public:
    static constexpr double MIN_ZOOM = 0.01;   // 1%
    static constexpr double MAX_ZOOM = 64.00;  // 6400%
    static constexpr double DEFAULT_ZOOM = 1.0; // 100%
    static constexpr double ZOOM_STEP_FACTOR = 1.15; // Z * 1.15^k exponential step

    ViewportEngine();
    ~ViewportEngine() = default;

    // Zoom Management
    double GetZoom() const { return m_zoom; }
    bool SetZoom(double zoom);
    double ZoomIn(int steps = 1);
    double ZoomOut(int steps = 1);
    double GetNextPresetZoom(bool zoomIn) const;

    // Focal-Point Preserving Zoom Invariant
    // Guarantees point under mouse (focalX, focalY) remains invariant on screen during zoom
    bool ApplyFocalZoom(double newZoom, double focalScreenX, double focalScreenY, double pageOffsetX = 0.0, double pageOffsetY = 0.0);
    static void CalculateFocalZoomOffsets(double currentZoom, double newZoom,
                                         double focalScreenX, double focalScreenY,
                                         double currentScrollX, double currentScrollY,
                                         double& outScrollX, double& outScrollY,
                                         double pageOffsetX = 0.0, double pageOffsetY = 0.0);

    // Fit Mode Computations
    double CalculateFitWidth(double contentWidth, double viewportWidth, double padding = 20.0) const;
    double CalculateFitPage(double contentWidth, double contentHeight, double viewportWidth, double viewportHeight, double padding = 20.0) const;
    double CalculateFitHeight(double contentHeight, double viewportHeight, double padding = 20.0) const;
    
    void SetFitMode(FitMode mode) { m_fitMode = mode; }
    FitMode GetFitMode() const { return m_fitMode; }

    // Scroll & Viewport Dimensions
    double GetScrollX() const { return m_scrollX; }
    double GetScrollY() const { return m_scrollY; }
    void SetScroll(double scrollX, double scrollY);
    void ScrollBy(double deltaX, double deltaY);

    void SetViewportSize(double width, double height);
    double GetViewportWidth() const { return m_viewportWidth; }
    double GetViewportHeight() const { return m_viewportHeight; }

    double GetTotalContentWidth() const { return m_totalContentWidth; }
    double GetTotalContentHeight() const { return m_totalContentHeight; }

    void ClampScroll(double& inOutScrollX, double& inOutScrollY) const;

    // Multi-page Continuous Layout Engine
    void UpdateContinuousLayout(const std::vector<std::pair<double, double>>& pageSizes,
                                const std::vector<int>& rotations = {},
                                double pageGap = 10.0);

    const std::vector<ViewportPageInfo>& GetLayout() const { return m_pageLayouts; }
    bool GetVisiblePageRange(int& outStartPage, int& outEndPage) const;
    int GetPageAtOffset(double offsetY) const;
    double GetScrollYForPage(int pageIndex) const;

private:
    double m_zoom = DEFAULT_ZOOM;
    FitMode m_fitMode = FitMode::None;

    double m_scrollX = 0.0;
    double m_scrollY = 0.0;

    double m_viewportWidth = 800.0;
    double m_viewportHeight = 600.0;

    double m_totalContentWidth = 0.0;
    double m_totalContentHeight = 0.0;

    std::vector<ViewportPageInfo> m_pageLayouts;
    static const std::vector<double> PRESET_ZOOM_LEVELS;
};

} // namespace ui::viewport
