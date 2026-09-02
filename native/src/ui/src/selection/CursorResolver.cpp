#include "../../include/selection/CursorResolver.h"
#include <cmath>

#ifndef IDC_PEN
#define IDC_PEN MAKEINTRESOURCE(32631)
#endif

namespace ui::selection {

HCURSOR CursorResolver::GetSystemCursor(LPCWSTR cursorId) {
    return ::LoadCursorW(nullptr, cursorId);
}

HCURSOR CursorResolver::ResolveTextCursor() {
    return GetSystemCursor(IDC_IBEAM);
}

HCURSOR CursorResolver::ResolveLinkCursor() {
    return GetSystemCursor(IDC_HAND);
}

HCURSOR CursorResolver::ResolveMoveCursor() {
    return GetSystemCursor(IDC_SIZEALL);
}

HCURSOR CursorResolver::AngleToResizeCursor(float angleDegrees) {
    // Normalize angle to [0, 180) because resize cursors are bidirectional
    float a = std::fmod(angleDegrees, 180.0f);
    if (a < 0.0f) a += 180.0f;

    // Sectors of 45° with 22.5° half-width boundaries:
    // Sector 0: [0, 22.5) or [157.5, 180) -> Horizontal (IDC_SIZEWE)
    // Sector 1: [22.5, 67.5)              -> Diagonal NW-SE (IDC_SIZENWSE in screen Y-down)
    // Sector 2: [67.5, 112.5)             -> Vertical (IDC_SIZENS)
    // Sector 3: [112.5, 157.5)            -> Diagonal NE-SW (IDC_SIZENESW in screen Y-down)
    if (a < 22.5f || a >= 157.5f) {
        return GetSystemCursor(IDC_SIZEWE);
    } else if (a >= 22.5f && a < 67.5f) {
        return GetSystemCursor(IDC_SIZENWSE);
    } else if (a >= 67.5f && a < 112.5f) {
        return GetSystemCursor(IDC_SIZENS);
    } else {
        return GetSystemCursor(IDC_SIZENESW);
    }
}

HCURSOR CursorResolver::ResolveHandleCursor(HandleType handle, float objectRotationDegrees) {
    switch (handle) {
    case HandleType::Rotation:
        return GetSystemCursor(IDC_SIZEALL);
    case HandleType::Body:
        return GetSystemCursor(IDC_SIZEALL);
    case HandleType::NW:
    case HandleType::N:
    case HandleType::NE:
    case HandleType::E:
    case HandleType::SE:
    case HandleType::S:
    case HandleType::SW:
    case HandleType::W: {
        float baseAngle = TransformHandles::GetHandleBaseAngle(handle);
        float effectiveAngle = baseAngle + objectRotationDegrees;
        return AngleToResizeCursor(effectiveAngle);
    }
    case HandleType::None:
    default:
        return nullptr;
    }
}

HCURSOR CursorResolver::ResolveToolCursor(tools::ToolType tool, tools::ToolState state) {
    if (state == tools::ToolState::InPlaceEditing) {
        return ResolveTextCursor();
    }

    switch (tool) {
    case tools::ToolType::Select:
        return (state == tools::ToolState::Dragging) ? GetSystemCursor(IDC_ARROW) : GetSystemCursor(IDC_ARROW);
    case tools::ToolType::Pan:
        return (state == tools::ToolState::Dragging) ? GetSystemCursor(IDC_SIZEALL) : GetSystemCursor(IDC_HAND);
    case tools::ToolType::TextSelect:
    case tools::ToolType::AddText:
    case tools::ToolType::EditText:
        return ResolveTextCursor();
    case tools::ToolType::Highlight:
    case tools::ToolType::Underline:
    case tools::ToolType::Strikeout:
        return ResolveTextCursor();
    case tools::ToolType::Ink:
        return GetSystemCursor(IDC_PEN);
    case tools::ToolType::Rectangle:
    case tools::ToolType::Ellipse:
    case tools::ToolType::Line:
    case tools::ToolType::Arrow:
    case tools::ToolType::Stamp:
    case tools::ToolType::InsertImage:
        return GetSystemCursor(IDC_CROSS);
    case tools::ToolType::Eraser:
        return GetSystemCursor(IDC_CROSS);
    case tools::ToolType::StickyNote:
    case tools::ToolType::FreeText:
        return ResolveTextCursor();
    default:
        return GetSystemCursor(IDC_ARROW);
    }
}

HCURSOR CursorResolver::ResolveCursor(
    tools::ToolType activeTool,
    tools::ToolState toolState,
    HandleType hoverHandle,
    float objectRotationDegrees,
    bool isHoveringText,
    bool isHoveringLink,
    bool isHoveringObject) {

    // 1. Hovering or dragging transform handles takes precedence in Select tool
    if (hoverHandle != HandleType::None) {
        HCURSOR h = ResolveHandleCursor(hoverHandle, objectRotationDegrees);
        if (h) return h;
    }

    // 2. Active in-place text editing
    if (toolState == tools::ToolState::InPlaceEditing) {
        return ResolveTextCursor();
    }

    // 3. Link hover has high priority
    if (isHoveringLink) {
        return ResolveLinkCursor();
    }

    // 4. Hovering selectable object in Select tool
    if (activeTool == tools::ToolType::Select) {
        if (isHoveringObject) {
            return ResolveMoveCursor();
        }
        if (isHoveringText) {
            return ResolveTextCursor();
        }
        return GetSystemCursor(IDC_ARROW);
    }

    // 5. TextSelect tool
    if (activeTool == tools::ToolType::TextSelect) {
        return ResolveTextCursor();
    }

    // 6. Text content hover when in general tools
    if (isHoveringText && (activeTool == tools::ToolType::None || activeTool == tools::ToolType::Select)) {
        return ResolveTextCursor();
    }

    // 7. Tool-specific fallback
    return ResolveToolCursor(activeTool, toolState);
}

} // namespace ui::selection







