#include "../../include/selection/TransformHandles.h"
#include <algorithm>

namespace ui::selection {

static constexpr float kPi = 3.14159265358979323846f;
static constexpr float kDegToRad = kPi / 180.0f;
static constexpr float kRadToDeg = 180.0f / kPi;

TransformHandles::TransformHandles() = default;
TransformHandles::~TransformHandles() = default;

float TransformHandles::SnapAngle15(float rawDegrees) {
    float a = std::fmod(rawDegrees, 360.0f);
    if (a < 0.0f) a += 360.0f;
    float snapped = std::round(a / 15.0f) * 15.0f;
    if (snapped >= 360.0f) snapped = 0.0f;
    return snapped;
}

float TransformHandles::ComputeRotationAngle(const PointF& center, const PointF& pointerPos) {
    float dx = pointerPos.x - center.x;
    float dy = pointerPos.y - center.y;
    // In screen coordinates (Y down), top is (0, -R).
    // Angle clockwise from top:
    float deg = std::atan2(dx, -dy) * kRadToDeg;
    if (deg < 0.0f) deg += 360.0f;
    return deg;
}

PointF TransformHandles::RotatePoint(const PointF& pt, const PointF& origin, float angleDegrees) {
    if (std::abs(angleDegrees) < 1e-4f) return pt;
    float rad = angleDegrees * kDegToRad;
    float cosA = std::cos(rad);
    float sinA = std::sin(rad);
    float dx = pt.x - origin.x;
    float dy = pt.y - origin.y;
    return {
        origin.x + dx * cosA - dy * sinA,
        origin.y + dx * sinA + dy * cosA
    };
}

Matrix3x2F TransformHandles::InvertMatrix(const Matrix3x2F& m) {
    float det = m.a * m.d - m.b * m.c;
    if (std::abs(det) < 1e-7f) {
        return { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }; // Identity fallback
    }
    float invDet = 1.0f / det;
    return {
        m.d * invDet,
        -m.b * invDet,
        -m.c * invDet,
        m.a * invDet,
        (m.c * m.f - m.d * m.e) * invDet,
        (m.b * m.e - m.a * m.f) * invDet
    };
}

PointF TransformHandles::TransformPoint(const PointF& pt, const Matrix3x2F& m) {
    return {
        m.a * pt.x + m.c * pt.y + m.e,
        m.b * pt.x + m.d * pt.y + m.f
    };
}

float TransformHandles::GetHandleBaseAngle(HandleType handle) {
    switch (handle) {
    case HandleType::E:        return 0.0f;
    case HandleType::SE:       return 45.0f;
    case HandleType::S:        return 90.0f;
    case HandleType::SW:       return 135.0f;
    case HandleType::W:        return 180.0f;
    case HandleType::NW:       return 225.0f;
    case HandleType::N:        return 270.0f;
    case HandleType::NE:       return 315.0f;
    case HandleType::Rotation: return 270.0f;
    default:                   return 0.0f;
    }
}

PointF TransformHandles::GetHandleDirectionVector(HandleType handle, float objectRotationDegrees) {
    float angle = GetHandleBaseAngle(handle) + objectRotationDegrees;
    float rad = angle * kDegToRad;
    return { std::cos(rad), std::sin(rad) };
}

std::vector<HandleDescriptor> TransformHandles::ComputeHandles(
    const RectF& viewBounds,
    float rotationDegrees,
    float handleSizeDip,
    float rotationOffsetDip,
    bool includeRotation) const {
    
    std::vector<HandleDescriptor> descriptors;
    descriptors.reserve(includeRotation ? 9 : 8);

    float l = (std::min)(viewBounds.left, viewBounds.right);
    float r = (std::max)(viewBounds.left, viewBounds.right);
    float t = (std::min)(viewBounds.top, viewBounds.bottom);
    float b = (std::max)(viewBounds.top, viewBounds.bottom);

    PointF center = { (l + r) * 0.5f, (t + b) * 0.5f };
    float halfSize = handleSizeDip * 0.5f;

    struct RawHandle {
        HandleType type;
        PointF localPos;
    };

    RawHandle rawList[9] = {
        { HandleType::NW, { l, t } },
        { HandleType::N,  { center.x, t } },
        { HandleType::NE, { r, t } },
        { HandleType::E,  { r, center.y } },
        { HandleType::SE, { r, b } },
        { HandleType::S,  { center.x, b } },
        { HandleType::SW, { l, b } },
        { HandleType::W,  { l, center.y } },
        { HandleType::Rotation, { center.x, t - rotationOffsetDip } }
    };

    int count = includeRotation ? 9 : 8;
    for (int i = 0; i < count; ++i) {
        PointF rotatedPos = RotatePoint(rawList[i].localPos, center, rotationDegrees);
        HandleDescriptor desc;
        desc.type = rawList[i].type;
        desc.position = rotatedPos;
        desc.hitBounds = {
            rotatedPos.x - halfSize,
            rotatedPos.y - halfSize,
            rotatedPos.x + halfSize,
            rotatedPos.y + halfSize
        };
        desc.angleDegrees = GetHandleBaseAngle(desc.type) + rotationDegrees;
        descriptors.push_back(desc);
    }

    return descriptors;
}

HandleType TransformHandles::HitTest(
    const PointF& viewPt,
    const RectF& viewBounds,
    float rotationDegrees,
    float handleSizeDip,
    float hitToleranceDip,
    bool includeRotation) const {

    auto handles = ComputeHandles(viewBounds, rotationDegrees, handleSizeDip, kDefaultRotationOffsetDip, includeRotation);
    float hitRadiusSq = (handleSizeDip * 0.5f + hitToleranceDip);
    hitRadiusSq = hitRadiusSq * hitRadiusSq;

    // 1. Check handles first (priority over body)
    for (const auto& h : handles) {
        float dx = viewPt.x - h.position.x;
        float dy = viewPt.y - h.position.y;
        if (dx * dx + dy * dy <= hitRadiusSq) {
            return h.type;
        }
    }

    // 2. Check body (inside rotated rectangle)
    float l = (std::min)(viewBounds.left, viewBounds.right);
    float r = (std::max)(viewBounds.left, viewBounds.right);
    float t = (std::min)(viewBounds.top, viewBounds.bottom);
    float b = (std::max)(viewBounds.top, viewBounds.bottom);
    PointF center = { (l + r) * 0.5f, (t + b) * 0.5f };

    // Un-rotate the query point relative to center to test against axis-aligned bounds
    PointF localPt = RotatePoint(viewPt, center, -rotationDegrees);
    if (localPt.x >= l && localPt.x <= r && localPt.y >= t && localPt.y <= b) {
        return HandleType::Body;
    }

    return HandleType::None;
}

HandleType TransformHandles::HitTestInverseMatrix(
    const PointF& viewPt,
    const RectF& localBounds,
    const Matrix3x2F& transformMatrix,
    float handleSizeDip,
    float hitToleranceDip,
    bool includeRotation) const {

    Matrix3x2F inv = InvertMatrix(transformMatrix);
    PointF localPt = TransformPoint(viewPt, inv);

    float l = (std::min)(localBounds.left, localBounds.right);
    float r = (std::max)(localBounds.left, localBounds.right);
    float t = (std::min)(localBounds.top, localBounds.bottom);
    float b = (std::max)(localBounds.top, localBounds.bottom);
    PointF center = { (l + r) * 0.5f, (t + b) * 0.5f };

    // Handle positions in local coordinates
    struct LocalHandle {
        HandleType type;
        PointF pos;
    };

    LocalHandle handles[9] = {
        { HandleType::NW, { l, t } },
        { HandleType::N,  { center.x, t } },
        { HandleType::NE, { r, t } },
        { HandleType::E,  { r, center.y } },
        { HandleType::SE, { r, b } },
        { HandleType::S,  { center.x, b } },
        { HandleType::SW, { l, b } },
        { HandleType::W,  { l, center.y } },
        { HandleType::Rotation, { center.x, t - kDefaultRotationOffsetDip } }
    };

    // Calculate effective scale from matrix for handle hit radius in local space
    float scaleX = std::sqrt(transformMatrix.a * transformMatrix.a + transformMatrix.b * transformMatrix.b);
    if (scaleX < 1e-4f) scaleX = 1.0f;
    float localRadius = (handleSizeDip * 0.5f + hitToleranceDip) / scaleX;
    float localRadiusSq = localRadius * localRadius;

    int count = includeRotation ? 9 : 8;
    for (int i = 0; i < count; ++i) {
        float dx = localPt.x - handles[i].pos.x;
        float dy = localPt.y - handles[i].pos.y;
        if (dx * dx + dy * dy <= localRadiusSq) {
            return handles[i].type;
        }
    }

    if (localPt.x >= l && localPt.x <= r && localPt.y >= t && localPt.y <= b) {
        return HandleType::Body;
    }

    return HandleType::None;
}

void TransformHandles::Render(
    ID2D1RenderTarget* renderTarget,
    const RectF& viewBounds,
    float rotationDegrees,
    float scale,
    bool isSelected,
    bool showRotationHandle,
    float handleSizeDip) const {

    if (!renderTarget || !isSelected) return;

    if (scale <= 0.0f) scale = 1.0f;

    ID2D1SolidColorBrush* borderBrush = nullptr;
    ID2D1SolidColorBrush* handleFillBrush = nullptr;
    ID2D1SolidColorBrush* handleBorderBrush = nullptr;
    ID2D1SolidColorBrush* rotationFillBrush = nullptr;

    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0078D7, 1.0f), &borderBrush);
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &handleFillBrush);
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0078D7, 1.0f), &handleBorderBrush);
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x2B79D4, 1.0f), &rotationFillBrush);

    float l = (std::min)(viewBounds.left, viewBounds.right);
    float r = (std::max)(viewBounds.left, viewBounds.right);
    float t = (std::min)(viewBounds.top, viewBounds.bottom);
    float b = (std::max)(viewBounds.top, viewBounds.bottom);
    PointF center = { (l + r) * 0.5f, (t + b) * 0.5f };

    // Apply rotation transform for rendering bounds and handles
    D2D1_MATRIX_3X2_F oldTransform;
    renderTarget->GetTransform(&oldTransform);

    D2D1_MATRIX_3X2_F rotMatrix = D2D1::Matrix3x2F::Rotation(
        rotationDegrees,
        D2D1::Point2F(center.x, center.y)
    );
    renderTarget->SetTransform(rotMatrix * oldTransform);

    // 1. Draw selection bounding rectangle
    D2D1_RECT_F boundRect = D2D1::RectF(l, t, r, b);
    if (borderBrush) {
        renderTarget->DrawRectangle(boundRect, borderBrush, 1.0f / scale);
    }

    // 2. Draw rotation stem if enabled
    float rotOffset = kDefaultRotationOffsetDip / scale;
    if (showRotationHandle && borderBrush) {
        D2D1_POINT_2F p1 = D2D1::Point2F(center.x, t);
        D2D1_POINT_2F p2 = D2D1::Point2F(center.x, t - rotOffset);
        renderTarget->DrawLine(p1, p2, borderBrush, 1.0f / scale);
    }

    // 3. Draw resize handles (constant DIP size on screen)
    float hw = (handleSizeDip * 0.5f) / scale;

    PointF localHandles[8] = {
        { l, t },
        { center.x, t },
        { r, t },
        { r, center.y },
        { r, b },
        { center.x, b },
        { l, b },
        { l, center.y }
    };

    for (int i = 0; i < 8; ++i) {
        D2D1_RECT_F hr = D2D1::RectF(
            localHandles[i].x - hw,
            localHandles[i].y - hw,
            localHandles[i].x + hw,
            localHandles[i].y + hw
        );
        if (handleFillBrush) renderTarget->FillRectangle(hr, handleFillBrush);
        if (handleBorderBrush) renderTarget->DrawRectangle(hr, handleBorderBrush, 1.0f / scale);
    }

    // 4. Draw rotation handle
    if (showRotationHandle) {
        D2D1_ELLIPSE ellipse = D2D1::Ellipse(
            D2D1::Point2F(center.x, t - rotOffset),
            hw + 0.5f,
            hw + 0.5f
        );
        if (rotationFillBrush) renderTarget->FillEllipse(ellipse, rotationFillBrush);
        if (handleFillBrush) renderTarget->DrawEllipse(ellipse, handleFillBrush, 1.0f / scale);
    }

    renderTarget->SetTransform(oldTransform);

    if (borderBrush) borderBrush->Release();
    if (handleFillBrush) handleFillBrush->Release();
    if (handleBorderBrush) handleBorderBrush->Release();
    if (rotationFillBrush) rotationFillBrush->Release();
}

void TransformHandles::RenderMarquee(
    ID2D1RenderTarget* renderTarget,
    const RectF& marqueeRect,
    float scale) const {

    if (!renderTarget) return;
    if (scale <= 0.0f) scale = 1.0f;

    float l = (std::min)(marqueeRect.left, marqueeRect.right);
    float r = (std::max)(marqueeRect.left, marqueeRect.right);
    float t = (std::min)(marqueeRect.top, marqueeRect.bottom);
    float b = (std::max)(marqueeRect.top, marqueeRect.bottom);

    D2D1_RECT_F rect = D2D1::RectF(l, t, r, b);

    ID2D1SolidColorBrush* fillBrush = nullptr;
    ID2D1SolidColorBrush* borderBrush = nullptr;

    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0078D7, 0.15f), &fillBrush);
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0078D7, 0.85f), &borderBrush);

    if (fillBrush) {
        renderTarget->FillRectangle(rect, fillBrush);
    }
    if (borderBrush) {
        renderTarget->DrawRectangle(rect, borderBrush, 1.0f / scale);
    }

    if (fillBrush) fillBrush->Release();
    if (borderBrush) borderBrush->Release();
}

} // namespace ui::selection
