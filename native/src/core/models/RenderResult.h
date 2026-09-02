#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "core/models/RenderRequest.h" // For RectF

namespace core {
namespace models {

enum class PixelFormat {
    BGRA8,
    RGBA8
};

struct RenderResult {
    std::wstring documentId;
    int generation;
    int pageIndex;
    RectF viewport;
    RectF tileRect;
    float renderScale;
    float dpi;
    int width;
    int height;
    int stride;
    PixelFormat pixelFormat;
    std::vector<uint8_t> pixelBuffer;
};

} // namespace models
} // namespace core
