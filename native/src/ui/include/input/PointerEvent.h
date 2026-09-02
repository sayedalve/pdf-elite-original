#pragma once

#include <cstdint>
#include <windows.h>
#include "EventResult.h"
#include "../../../core/Geometry.h"

namespace ui::input {

enum class PointerButton : uint32_t {
    None = 0,
    Left = 1 << 0,
    Right = 1 << 1,
    Middle = 1 << 2,
    XButton1 = 1 << 3,
    XButton2 = 1 << 4
};

inline PointerButton operator|(PointerButton a, PointerButton b) {
    return static_cast<PointerButton>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline PointerButton operator&(PointerButton a, PointerButton b) {
    return static_cast<PointerButton>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline PointerButton operator^(PointerButton a, PointerButton b) {
    return static_cast<PointerButton>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
}

inline PointerButton operator~(PointerButton a) {
    return static_cast<PointerButton>(~static_cast<uint32_t>(a));
}

inline PointerButton& operator|=(PointerButton& a, PointerButton b) {
    a = a | b;
    return a;
}

inline PointerButton& operator&=(PointerButton& a, PointerButton b) {
    a = a & b;
    return a;
}

inline bool HasButton(PointerButton flags, PointerButton check) {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(check)) != 0;
}

enum class KeyModifier : uint32_t {
    None = 0,
    Shift = 1 << 0,
    Control = 1 << 1,
    Alt = 1 << 2,
    Windows = 1 << 3
};

inline KeyModifier operator|(KeyModifier a, KeyModifier b) {
    return static_cast<KeyModifier>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline KeyModifier operator&(KeyModifier a, KeyModifier b) {
    return static_cast<KeyModifier>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline KeyModifier operator^(KeyModifier a, KeyModifier b) {
    return static_cast<KeyModifier>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
}

inline KeyModifier operator~(KeyModifier a) {
    return static_cast<KeyModifier>(~static_cast<uint32_t>(a));
}

inline KeyModifier& operator|=(KeyModifier& a, KeyModifier b) {
    a = a | b;
    return a;
}

inline KeyModifier& operator&=(KeyModifier& a, KeyModifier b) {
    a = a & b;
    return a;
}

inline bool HasModifier(KeyModifier flags, KeyModifier check) {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(check)) != 0;
}

enum class PointerEventType {
    Down,
    Move,
    Up,
    DoubleClick,
    Leave
};

// Canonical Pointer Event with spatial and modifier context across all coordinate tiers
struct PointerEvent {
    PointerEventType type = PointerEventType::Move;
    PointerButton button = PointerButton::None;
    PointerButton buttonsDown = PointerButton::None;
    KeyModifier modifiers = KeyModifier::None;

    // Spatial coordinates
    PointF physicalScreen = {0.0f, 0.0f}; // Win32 client physical pixels
    PointF clientDip = {0.0f, 0.0f};      // Logical DIPs (96 DPI normalized)
    PointF canvasPoint = {0.0f, 0.0f};    // Viewport continuous canvas space
    PointF pagePoint = {0.0f, 0.0f};      // PDF user points (72 pt/in, unrotated)
    int pageIndex = -1;                   // Target page index (-1 if outside)

    uint64_t timestampMs = 0;
    float pressure = 1.0f;                // Stylus / pen pressure
};

// Canonical Keyboard Event
struct KeyEvent {
    uint32_t virtualKey = 0;
    wchar_t charCode = 0;
    KeyModifier modifiers = KeyModifier::None;
    bool isDown = false;
    bool isRepeat = false;
    uint64_t timestampMs = 0;
};

// Canonical Scroll Event
struct ScrollEvent {
    float deltaX = 0.0f;            // DIPs or wheel ticks to scroll horizontally
    float deltaY = 0.0f;            // DIPs or wheel ticks to scroll vertically
    bool isZoom = false;            // Ctrl + Wheel zoom gesture
    float zoomFactor = 1.0f;        // Zoom multiplier (e.g. 1.1x or 0.9x)
    PointF anchorDip = {0.0f, 0.0f};// Anchor center for zoom in DIPs
    KeyModifier modifiers = KeyModifier::None;
    uint64_t timestampMs = 0;
};

} // namespace ui::input
