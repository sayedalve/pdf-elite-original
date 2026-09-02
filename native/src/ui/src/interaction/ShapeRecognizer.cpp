#include "ShapeRecognizer.h"
#include <cmath>
#include <algorithm>

namespace ui {
namespace interaction {

ShapeRecognitionResult ShapeRecognizer::Recognize(const std::vector<PointF>& stroke, double errorTolerance) {
    ShapeRecognitionResult result;
    if (stroke.size() < 5) return result;

    result.bounds = CalculateBounds(stroke);

    // Simple heuristic-based shape recognition (Xournal++ inspired)
    if (IsLine(stroke, errorTolerance)) {
        result.shape = RecognizedShape::Line;
        result.start = stroke.front();
        result.end = stroke.back();
    }
    // Checking endpoints distance for closed shapes
    else {
        double dx = stroke.back().x - stroke.front().x;
        double dy = stroke.back().y - stroke.front().y;
        double dist = std::sqrt(dx*dx + dy*dy);
        
        // If it's a closed loop
        if (dist < std::max(result.bounds.right - result.bounds.left, result.bounds.bottom - result.bounds.top) * 0.2) {
            if (IsRectangle(stroke, errorTolerance)) {
                result.shape = RecognizedShape::Rectangle;
            } else if (IsEllipse(stroke, errorTolerance)) {
                result.shape = RecognizedShape::Ellipse;
            }
        }
    }

    return result;
}

RectF ShapeRecognizer::CalculateBounds(const std::vector<PointF>& stroke) {
    RectF bounds = { 999999.0f, 999999.0f, -999999.0f, -999999.0f };
    for (const auto& p : stroke) {
        bounds.left = std::min(bounds.left, p.x);
        bounds.top = std::min(bounds.top, p.y);
        bounds.right = std::max(bounds.right, p.x);
        bounds.bottom = std::max(bounds.bottom, p.y);
    }
    return bounds;
}

bool ShapeRecognizer::IsLine(const std::vector<PointF>& stroke, double tolerance) {
    // Check linear regression error
    PointF start = stroke.front();
    PointF end = stroke.back();
    
    double length = std::sqrt(std::pow(end.x - start.x, 2) + std::pow(end.y - start.y, 2));
    if (length < 1.0) return false;

    double maxError = 0;
    for (const auto& p : stroke) {
        // Distance from point to line formed by start & end
        double num = std::abs((end.y - start.y) * p.x - (end.x - start.x) * p.y + end.x * start.y - end.y * start.x);
        double dist = num / length;
        maxError = std::max(maxError, dist);
    }

    return (maxError / length) < tolerance;
}

bool ShapeRecognizer::IsRectangle(const std::vector<PointF>& stroke, double tolerance) {
    (void)stroke;
    (void)tolerance;
    // Mock implementation for recognition logic
    return false; // To be expanded with full polygon detection
}

bool ShapeRecognizer::IsEllipse(const std::vector<PointF>& stroke, double tolerance) {
    (void)stroke;
    (void)tolerance;
    // Mock implementation for ellipse variance check
    return true; // If closed and not rectangle, assume ellipse for this stage
}

} // namespace interaction
} // namespace ui
