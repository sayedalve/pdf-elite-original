#include "CoordinateConverter.h"
#include <algorithm>
#include <cmath>

// =========================================================================
// Tier 1 <-> Tier 2: Screen Pixels <-> Logical DIPs (Per-Monitor V2 DPI)
// =========================================================================

LogicalPoint CoordinateConverter::ScreenToLogical(const ScreenPoint& screenPt, float dpiScale) {
    float scale = (dpiScale > 0.001f) ? dpiScale : 1.0f;
    return { screenPt.x / scale, screenPt.y / scale };
}

ScreenPoint CoordinateConverter::LogicalToScreen(const LogicalPoint& logicalPt, float dpiScale) {
    float scale = (dpiScale > 0.001f) ? dpiScale : 1.0f;
    return { logicalPt.x * scale, logicalPt.y * scale };
}

PointF CoordinateConverter::ScreenToLogical(double screenX, double screenY, float dpiScale) {
    float scale = (dpiScale > 0.001f) ? dpiScale : 1.0f;
    return { static_cast<float>(screenX / scale), static_cast<float>(screenY / scale) };
}

PointF CoordinateConverter::LogicalToScreen(double logicalX, double logicalY, float dpiScale) {
    float scale = (dpiScale > 0.001f) ? dpiScale : 1.0f;
    return { static_cast<float>(logicalX * scale), static_cast<float>(logicalY * scale) };
}

RectF CoordinateConverter::ScreenToLogicalRect(const RectF& screenRect, float dpiScale) {
    float scale = (dpiScale > 0.001f) ? dpiScale : 1.0f;
    return {
        screenRect.left / scale,
        screenRect.top / scale,
        screenRect.right / scale,
        screenRect.bottom / scale
    };
}

RectF CoordinateConverter::LogicalToScreenRect(const RectF& logicalRect, float dpiScale) {
    float scale = (dpiScale > 0.001f) ? dpiScale : 1.0f;
    return {
        logicalRect.left * scale,
        logicalRect.top * scale,
        logicalRect.right * scale,
        logicalRect.bottom * scale
    };
}

// =========================================================================
// Tier 4 <-> Tier 3: PDF Points (bottom-left) to Normalized (0.0 - 1.0, top-left)
// =========================================================================

PointF CoordinateConverter::PdfToNormalized(const PageContext& page, double pdfX, double pdfY) {
    // PDF coordinates: origin at bottom-left, unrotated
    // Output: Normalized [0.0 - 1.0], origin at top-left, rotated to visual
    double x = (page.widthPts > 0.0) ? (pdfX / page.widthPts) : 0.0;
    double y = (page.heightPts > 0.0) ? (1.0 - (pdfY / page.heightPts)) : 0.0;

    // Apply rotation
    if (page.rotation == 90) {
        return { static_cast<float>(y), static_cast<float>(1.0 - x) };
    } else if (page.rotation == 180) {
        return { static_cast<float>(1.0 - x), static_cast<float>(1.0 - y) };
    } else if (page.rotation == 270) {
        return { static_cast<float>(1.0 - y), static_cast<float>(x) };
    }
    
    return { static_cast<float>(x), static_cast<float>(y) };
}

PointF CoordinateConverter::NormalizedToPdf(const PageContext& page, double normX, double normY) {
    double x = normX;
    double y = normY;

    if (page.rotation == 90) {
        double temp = x;
        x = 1.0 - y;
        y = temp;
    } else if (page.rotation == 180) {
        x = 1.0 - x;
        y = 1.0 - y;
    } else if (page.rotation == 270) {
        double temp = x;
        x = y;
        y = 1.0 - temp;
    }

    return { static_cast<float>(x * page.widthPts), static_cast<float>((1.0 - y) * page.heightPts) };
}

// =========================================================================
// Tier 3 <-> Tier 2: Normalized to Viewport (Zoomed/Scaled infinite canvas)
// =========================================================================

PointF CoordinateConverter::NormalizedToViewport(const PageContext& page, const ViewContext& view, double normX, double normY) {
    // To Viewport: Scale normalized by actual page physical dimensions * zoom
    bool isLandscape = (page.rotation == 90 || page.rotation == 270);
    double visualWidth = isLandscape ? page.heightPts : page.widthPts;
    double visualHeight = isLandscape ? page.widthPts : page.heightPts;
    
    return {
        static_cast<float>(normX * visualWidth * view.zoom),
        static_cast<float>(normY * visualHeight * view.zoom)
    };
}

PointF CoordinateConverter::ViewportToNormalized(const PageContext& page, const ViewContext& view, double viewX, double viewY) {
    bool isLandscape = (page.rotation == 90 || page.rotation == 270);
    double visualWidth = isLandscape ? page.heightPts : page.widthPts;
    double visualHeight = isLandscape ? page.widthPts : page.heightPts;
    double denomX = visualWidth * view.zoom;
    double denomY = visualHeight * view.zoom;
    
    return {
        static_cast<float>((denomX != 0.0) ? (viewX / denomX) : 0.0),
        static_cast<float>((denomY != 0.0) ? (viewY / denomY) : 0.0)
    };
}

// =========================================================================
// Tier 2 <-> Tier 1: Viewport to Screen (UI Pixels / DIPs)
// =========================================================================

PointF CoordinateConverter::ViewportToScreen(const ViewContext& view, double viewX, double viewY) {
    return {
        static_cast<float>(view.pageOffsetX - view.scrollX + viewX),
        static_cast<float>(view.pageOffsetY - view.scrollY + viewY)
    };
}

PointF CoordinateConverter::ScreenToViewport(const ViewContext& view, double screenX, double screenY) {
    return {
        static_cast<float>(screenX + view.scrollX - view.pageOffsetX),
        static_cast<float>(screenY + view.scrollY - view.pageOffsetY)
    };
}

// =========================================================================
// Direct Multi-Tier Conversions
// =========================================================================

PointF CoordinateConverter::PdfToScreen(const PageContext& page, const ViewContext& view, double pdfX, double pdfY) {
    double viewX = pdfX * view.zoom;
    double viewY = (page.heightPts - pdfY) * view.zoom;

    double screenX = view.pageOffsetX - view.scrollX + viewX;
    double screenY = view.pageOffsetY - view.scrollY + viewY;

    return { static_cast<float>(screenX), static_cast<float>(screenY) };
}

PointF CoordinateConverter::ScreenToPdf(const PageContext& page, const ViewContext& view, double screenX, double screenY) {
    double viewX = screenX + view.scrollX - view.pageOffsetX;
    double viewY = screenY + view.scrollY - view.pageOffsetY;

    if (view.zoom == 0.0) {
        return { 0.0f, 0.0f };
    }

    double pdfX = viewX / view.zoom;
    double pdfY = page.heightPts - (viewY / view.zoom);

    return { static_cast<float>(pdfX), static_cast<float>(pdfY) };
}

RectF CoordinateConverter::PdfToScreenRect(const PageContext& page, const ViewContext& view, double pdfLeft, double pdfTop, double pdfRight, double pdfBottom) {
    PointF p1 = PdfToScreen(page, view, pdfLeft, pdfTop);
    PointF p2 = PdfToScreen(page, view, pdfRight, pdfBottom);
    
    return {
        std::min(p1.x, p2.x),
        std::min(p1.y, p2.y),
        std::max(p1.x, p2.x),
        std::max(p1.y, p2.y)
    };
}

RectF CoordinateConverter::ScreenToPdfRect(const PageContext& page, const ViewContext& view, double screenLeft, double screenTop, double screenRight, double screenBottom) {
    PointF p1 = ScreenToPdf(page, view, screenLeft, screenTop);
    PointF p2 = ScreenToPdf(page, view, screenRight, screenBottom);
    
    return {
        std::min(p1.x, p2.x),
        std::min(p1.y, p2.y),
        std::max(p1.x, p2.x),
        std::max(p1.y, p2.y)
    };
}

// =========================================================================
// Rotation Matrix & Transformation Helpers
// =========================================================================

Matrix3x2F CoordinateConverter::GetRotationMatrix(int rotationDegrees) {
    int rot = (rotationDegrees % 360 + 360) % 360;
    if (rot == 90) {
        return { 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f };
    } else if (rot == 180) {
        return { -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f };
    } else if (rot == 270) {
        return { 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
    }
    return { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
}

Matrix3x2F CoordinateConverter::GetPdfToNormalizedMatrix(const PageContext& page) {
    float invW = (page.widthPts > 0.0) ? static_cast<float>(1.0 / page.widthPts) : 0.0f;
    float invH = (page.heightPts > 0.0) ? static_cast<float>(1.0 / page.heightPts) : 0.0f;

    int rot = (page.rotation % 360 + 360) % 360;
    if (rot == 90) {
        return { 0.0f, -invW, -invH, 0.0f, 1.0f, 1.0f };
    } else if (rot == 180) {
        return { -invW, 0.0f, 0.0f, invH, 1.0f, 0.0f };
    } else if (rot == 270) {
        return { 0.0f, invW, invH, 0.0f, 0.0f, 0.0f };
    }
    return { invW, 0.0f, 0.0f, -invH, 0.0f, 1.0f };
}

Matrix3x2F CoordinateConverter::GetNormalizedToPdfMatrix(const PageContext& page) {
    float w = static_cast<float>(page.widthPts);
    float h = static_cast<float>(page.heightPts);

    int rot = (page.rotation % 360 + 360) % 360;
    if (rot == 90) {
        return { 0.0f, -h, -w, 0.0f, w, h };
    } else if (rot == 180) {
        return { -w, 0.0f, 0.0f, h, w, 0.0f };
    } else if (rot == 270) {
        return { 0.0f, h, w, 0.0f, 0.0f, 0.0f };
    }
    return { w, 0.0f, 0.0f, -h, 0.0f, h };
}

Matrix3x2F CoordinateConverter::GetPdfToViewportMatrix(const PageContext& page, double zoom) {
    Matrix3x2F normMat = GetPdfToNormalizedMatrix(page);
    bool isLandscape = (page.rotation == 90 || page.rotation == 270);
    float vw = static_cast<float>((isLandscape ? page.heightPts : page.widthPts) * zoom);
    float vh = static_cast<float>((isLandscape ? page.widthPts : page.heightPts) * zoom);

    return {
        normMat.a * vw,
        normMat.b * vh,
        normMat.c * vw,
        normMat.d * vh,
        normMat.e * vw,
        normMat.f * vh
    };
}

Matrix3x2F CoordinateConverter::GetViewportToPdfMatrix(const PageContext& page, double zoom) {
    Matrix3x2F normToPdf = GetNormalizedToPdfMatrix(page);
    bool isLandscape = (page.rotation == 90 || page.rotation == 270);
    float invVw = (zoom > 0.0) ? static_cast<float>(1.0 / ((isLandscape ? page.heightPts : page.widthPts) * zoom)) : 0.0f;
    float invVh = (zoom > 0.0) ? static_cast<float>(1.0 / ((isLandscape ? page.widthPts : page.heightPts) * zoom)) : 0.0f;

    return {
        normToPdf.a * invVw,
        normToPdf.b * invVh,
        normToPdf.c * invVw,
        normToPdf.d * invVh,
        normToPdf.e,
        normToPdf.f
    };
}

PointF CoordinateConverter::PdfToPage(const PageContext& page, double pdfX, double pdfY) {
    PointF norm = PdfToNormalized(page, pdfX, pdfY);
    bool isLandscape = (page.rotation == 90 || page.rotation == 270);
    double visualWidth = isLandscape ? page.heightPts : page.widthPts;
    double visualHeight = isLandscape ? page.widthPts : page.heightPts;
    return { static_cast<float>(norm.x * visualWidth), static_cast<float>(norm.y * visualHeight) };
}

int CoordinateConverter::FindPageIndexAtViewportY(double viewportY, const std::vector<ContinuousPageLayout>& layouts) {
    if (layouts.empty()) return 0;
    for (const auto& layout : layouts) {
        if (viewportY >= layout.yOffset && viewportY < layout.yOffset + layout.height) {
            return layout.index;
        }
    }
    if (viewportY < layouts.front().yOffset) return layouts.front().index;
    return layouts.back().index;
}

PointF CoordinateConverter::MultiPageOffset(int pageIndex, const std::vector<ContinuousPageLayout>& layouts) {
    for (const auto& layout : layouts) {
        if (layout.index == pageIndex) {
            return { 0.0f, layout.yOffset };
        }
    }
    return { 0.0f, 0.0f };
}


