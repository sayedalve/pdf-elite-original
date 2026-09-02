#pragma once

#include <vector>
#include <cmath>
#include <windows.h>
#include <d2d1.h>
#include "../../../core/Geometry.h"

namespace ui::selection {

enum class HandleType {
    None = 0,
    NW = 1,      // Top-Left
    N = 2,       // Top-Center
    NE = 3,      // Top-Right
    E = 4,       // Right-Center
    SE = 5,      // Bottom-Right
    S = 6,       // Bottom-Center
    SW = 7,      // Bottom-Left
    W = 8,       // Left-Center
    Rotation = 9,// Rotation stem handle
    Body = 10    // Inside object body (for translation/move)
};

struct HandleDescriptor {
    HandleType type = HandleType::None;
    PointF position = { 0.0f, 0.0f }; // In target view/screen space
    RectF hitBounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    float angleDegrees = 0.0f; // Visual outward angle taking rotation into account
};

class TransformHandles {
public:
    TransformHandles();
    ~TransformHandles();

    // Size constants (in logical DIPs)
    static constexpr float kDefaultHandleSizeDip = 8.0f;
    static constexpr float kDefaultRotationOffsetDip = 24.0f;
    static constexpr float kDefaultHitToleranceDip = 5.0f;

    // --- Hit Testing ---
    HandleType HitTest(
        const PointF& viewPt,
        const RectF& viewBounds,
        float rotationDegrees = 0.0f,
        float handleSizeDip = kDefaultHandleSizeDip,
        float hitToleranceDip = kDefaultHitToleranceDip,
        bool includeRotation = true) const;

    HandleType HitTestInverseMatrix(
        const PointF& viewPt,
        const RectF& localBounds,
        const Matrix3x2F& transformMatrix,
        float handleSizeDip = kDefaultHandleSizeDip,
        float hitToleranceDip = kDefaultHitToleranceDip,
        bool includeRotation = true) const;

    // --- Angle & Snapping Utilities ---
    static float SnapAngle15(float rawDegrees);
    static float ComputeRotationAngle(const PointF& center, const PointF& pointerPos);
    static PointF RotatePoint(const PointF& pt, const PointF& origin, float angleDegrees);
    static Matrix3x2F InvertMatrix(const Matrix3x2F& m);
    static PointF TransformPoint(const PointF& pt, const Matrix3x2F& m);

    // --- Handle Geometry ---
    std::vector<HandleDescriptor> ComputeHandles(
        const RectF& viewBounds,
        float rotationDegrees = 0.0f,
        float handleSizeDip = kDefaultHandleSizeDip,
        float rotationOffsetDip = kDefaultRotationOffsetDip,
        bool includeRotation = true) const;

    static PointF GetHandleDirectionVector(HandleType handle, float objectRotationDegrees);
    static float GetHandleBaseAngle(HandleType handle);

    // --- Rendering ---
    void Render(
        ID2D1RenderTarget* renderTarget,
        const RectF& viewBounds,
        float rotationDegrees = 0.0f,
        float scale = 1.0f,
        bool isSelected = true,
        bool showRotationHandle = true,
        float handleSizeDip = kDefaultHandleSizeDip) const;

    void RenderMarquee(
        ID2D1RenderTarget* renderTarget,
        const RectF& marqueeRect,
        float scale = 1.0f) const;
};

} // namespace ui::selection
