#pragma once
#include <string>
#include <vector>
#include "core/Geometry.h"

namespace core {
namespace models {

enum class ObjectType {
    Unknown,
    Text,
    Image,
    Annotation_Line,
    Annotation_Square,
    Annotation_Circle,
    Annotation_Highlight,
    Annotation_Text,
    Annotation_Ink,
    Annotation_FreeText,
    Annotation_Other
};

struct Point {
    float x;
    float y;
};

struct LineGeometry {
    Point start;
    Point end;
};

struct PageObject {
    std::string id;       // Unique ID (e.g. cast from uint64_t or annotation ID)
    ObjectType type;
    int pageIndex;
    RectF bounds;
    float rotation = 0.0f;
    
    // For text
    std::wstring text;
    float fontSize = 0.0f;
    
    // For lines
    bool hasLineGeometry = false;
    LineGeometry lineGeometry;
};

} // namespace models
} // namespace core
