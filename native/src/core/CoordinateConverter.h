#pragma once
#include "Geometry.h"
#include <vector>
#include <cmath>
#include <algorithm>

// 4-Tier Coordinate Pipeline based on Okular / PDF4QT precision math
// Tier 1: Screen Physical Pixels (Hardware)
// Tier 2: Logical DIPs (Device-Independent Pixels, Per-Monitor V2 DPI scaling)
// Tier 3: Viewport / Canvas space (Zoomed/Panned continuous multi-page space)
// Tier 4: PDF Page points (72 DPI, bottom-left origin, unrotated physical space)

using ScreenPoint = PointF;
using LogicalPoint = PointF;
using PdfPoint = PointF;
using ViewportPoint = PointF;
using NormalizedPoint = PointF;

enum class PageRotation {
    Rotate0 = 0,
    Rotate90 = 90,
    Rotate180 = 180,
    Rotate270 = 270
};

struct ViewportOffset {
    double x = 0.0;
    double y = 0.0;
};

struct ContinuousPageLayout {
    int index = 0;
    float yOffset = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

class CoordinateConverter {
public:
    // Page dimensions in PDF points
    struct PageContext {
        double widthPts;
        double heightPts;
        int rotation; // 0, 90, 180, 270
    };

    // View context
    struct ViewContext {
        double zoom;
        double scrollX;
        double scrollY;
        double pageOffsetX; // Screen offset of the page's top-left corner
        double pageOffsetY;
    };

    // =========================================================================
    // Tier 1 <-> Tier 2: Screen Pixels <-> Logical DIPs (Per-Monitor V2 DPI)
    // =========================================================================
    static LogicalPoint ScreenToLogical(const ScreenPoint& screenPt, float dpiScale = 1.0f);
    static ScreenPoint LogicalToScreen(const LogicalPoint& logicalPt, float dpiScale = 1.0f);
    static PointF ScreenToLogical(double screenX, double screenY, float dpiScale = 1.0f);
    static PointF LogicalToScreen(double logicalX, double logicalY, float dpiScale = 1.0f);
    static RectF ScreenToLogicalRect(const RectF& screenRect, float dpiScale = 1.0f);
    static RectF LogicalToScreenRect(const RectF& logicalRect, float dpiScale = 1.0f);

    // =========================================================================
    // Tier 4 <-> Tier 3: PDF Points (bottom-left) to Normalized (0.0 - 1.0, top-left)
    // =========================================================================
    static PointF PdfToNormalized(const PageContext& page, double pdfX, double pdfY);
    static PointF NormalizedToPdf(const PageContext& page, double normX, double normY);

    // =========================================================================
    // Tier 3 <-> Tier 2: Normalized to Viewport (Zoomed/Scaled infinite canvas)
    // =========================================================================
    static PointF NormalizedToViewport(const PageContext& page, const ViewContext& view, double normX, double normY);
    static PointF ViewportToNormalized(const PageContext& page, const ViewContext& view, double viewX, double viewY);

    // =========================================================================
    // Tier 2 <-> Tier 1: Viewport to Screen (UI Pixels / DIPs)
    // =========================================================================
    static PointF ViewportToScreen(const ViewContext& view, double viewX, double viewY);
    static PointF ScreenToViewport(const ViewContext& view, double screenX, double screenY);

    // =========================================================================
    // Direct Multi-Tier Conversions
    // =========================================================================
    static PointF PdfToScreen(const PageContext& page, const ViewContext& view, double pdfX, double pdfY);
    static PointF ScreenToPdf(const PageContext& page, const ViewContext& view, double screenX, double screenY);
    
    static RectF PdfToScreenRect(const PageContext& page, const ViewContext& view, double pdfLeft, double pdfTop, double pdfRight, double pdfBottom);
    static RectF ScreenToPdfRect(const PageContext& page, const ViewContext& view, double screenLeft, double screenTop, double screenRight, double screenBottom);

    // =========================================================================
    // Rotation Matrix & Transformation Helpers
    // =========================================================================
    static Matrix3x2F GetRotationMatrix(int rotationDegrees);
    static Matrix3x2F GetPdfToNormalizedMatrix(const PageContext& page);
    static Matrix3x2F GetNormalizedToPdfMatrix(const PageContext& page);
    static Matrix3x2F GetPdfToViewportMatrix(const PageContext& page, double zoom);
    static Matrix3x2F GetViewportToPdfMatrix(const PageContext& page, double zoom);

    // Compatibility helper for tests expecting PdfToPage
    static PointF PdfToPage(const PageContext& page, double pdfX, double pdfY);

    // Continuous Multi-Page Layout Helpers
    static int FindPageIndexAtViewportY(double viewportY, const std::vector<ContinuousPageLayout>& layouts);
    static PointF MultiPageOffset(int pageIndex, const std::vector<ContinuousPageLayout>& layouts);
};

