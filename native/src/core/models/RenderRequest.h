#pragma once
#include <string>
#include <cstdint>

namespace core {
namespace models {

struct RectF {
    float left;
    float top;
    float right;
    float bottom;
};

enum class RenderPriority {
    Background = 0,
    Thumbnail = 1,
    Nearby = 2,
    Visible = 3
};

struct RenderRequest {
    std::wstring documentId;
    int generation;
    int pageIndex;
    float renderScale;
    float dpi;
    RectF viewport;
    RectF tileRect;
    RenderPriority category = RenderPriority::Visible;
    float tileCy = 0.0f; // Center Y of the tile relative to the viewport
    int priority = 0;
    bool darkMode = false;
};

} // namespace models
} // namespace core
