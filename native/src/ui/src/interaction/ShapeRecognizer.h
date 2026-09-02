#pragma once
#include <vector>
#include "../../../core/Geometry.h"

namespace ui {
namespace interaction {

enum class RecognizedShape {
    None,
    Line,
    Rectangle,
    Ellipse
};

struct ShapeRecognitionResult {
    RecognizedShape shape = RecognizedShape::None;
    RectF bounds;
    PointF start, end; // For Line
};

// Inspired by Xournal++ ShapeRecognizer
class ShapeRecognizer {
public:
    static ShapeRecognitionResult Recognize(const std::vector<PointF>& stroke, double errorTolerance = 0.1);

private:
    static bool IsLine(const std::vector<PointF>& stroke, double tolerance);
    static bool IsRectangle(const std::vector<PointF>& stroke, double tolerance);
    static bool IsEllipse(const std::vector<PointF>& stroke, double tolerance);
    static RectF CalculateBounds(const std::vector<PointF>& stroke);
};

} // namespace interaction
} // namespace ui
