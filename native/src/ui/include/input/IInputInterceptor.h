#pragma once

#include "PointerEvent.h"
#include "EventResult.h"
#include <windows.h>
#include <optional>

namespace ui::input {

class IInputInterceptor {
public:
    virtual ~IInputInterceptor() = default;

    virtual EventResult RoutePointerDown(const PointerEvent& event) { (void)event; return EventResult::Ignored; }
    virtual EventResult RoutePointerMove(const PointerEvent& event) { (void)event; return EventResult::Ignored; }
    virtual EventResult RoutePointerUp(const PointerEvent& event) { (void)event; return EventResult::Ignored; }
    virtual EventResult RoutePointerDoubleClick(const PointerEvent& event) { (void)event; return EventResult::Ignored; }
    
    virtual EventResult RouteMouseWheel(const ScrollEvent& event) { (void)event; return EventResult::Ignored; }
    virtual EventResult RouteKeyDown(const KeyEvent& event) { (void)event; return EventResult::Ignored; }
    virtual EventResult RouteKeyUp(const KeyEvent& event) { (void)event; return EventResult::Ignored; }
    virtual EventResult RouteChar(const KeyEvent& event) { (void)event; return EventResult::Ignored; }

    virtual std::optional<HCURSOR> QueryCursor(const PointF& clientDip) { (void)clientDip; return std::nullopt; }
    virtual void OnCaptureLost() {}
};

} // namespace ui::input
