#pragma once

#include <windows.h>
#include "../tools/ToolType.h"
#include "../tools/ITool.h"
#include "TransformHandles.h"

namespace ui::selection {

class CursorResolver {
public:
    // Resolve cursor from high-level interaction state
    static HCURSOR ResolveCursor(
        tools::ToolType activeTool,
        tools::ToolState toolState,
        HandleType hoverHandle,
        float objectRotationDegrees = 0.0f,
        bool isHoveringText = false,
        bool isHoveringLink = false,
        bool isHoveringObject = false);

    // Specific resolver methods
    static HCURSOR ResolveHandleCursor(HandleType handle, float objectRotationDegrees = 0.0f);
    static HCURSOR AngleToResizeCursor(float angleDegrees);
    static HCURSOR ResolveToolCursor(tools::ToolType tool, tools::ToolState state = tools::ToolState::Idle);
    static HCURSOR ResolveTextCursor();
    static HCURSOR ResolveLinkCursor();
    static HCURSOR ResolveMoveCursor();
    static HCURSOR GetSystemCursor(LPCWSTR cursorId);
};

} // namespace ui::selection
