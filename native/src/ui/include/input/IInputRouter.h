#pragma once

#include <windows.h>
#include "PointerEvent.h"
#include "EventResult.h"

namespace ui::input {

class IInputRouter {
public:
    virtual ~IInputRouter() = default;

    virtual EventResult RoutePointerDown(const PointerEvent& event) = 0;
    virtual EventResult RoutePointerMove(const PointerEvent& event) = 0;
    virtual EventResult RoutePointerUp(const PointerEvent& event) = 0;
    virtual EventResult RoutePointerDoubleClick(const PointerEvent& event) = 0;
    virtual EventResult RouteMouseWheel(const ScrollEvent& event) = 0;
    virtual EventResult RouteKeyDown(const KeyEvent& event) = 0;
    virtual EventResult RouteKeyUp(const KeyEvent& event) = 0;
    virtual EventResult RouteChar(const KeyEvent& event) = 0;
    virtual bool RouteSetCursor(HWND hwnd, UINT hitTest, UINT message) = 0;
    virtual HCURSOR QueryCursor(const PointF& clientDip) = 0;
    virtual void OnCaptureLost() = 0;
};

} // namespace ui::input
