#include "../../include/input/InputRouter.h"

namespace ui::input {

InputRouter::InputRouter(std::shared_ptr<IPointerCaptureService> captureService,
                         std::shared_ptr<tools::ToolStateMachine> stateMachine)
    : m_captureService(std::move(captureService)), m_stateMachine(std::move(stateMachine)) {
    if (m_captureService && m_stateMachine) {
        m_captureService->SetCaptureLostCallback([this](void* ownerToken) {
            (void)ownerToken;
            if (m_stateMachine) {
                m_stateMachine->CancelActiveInteractions();
            }
            for (auto& interceptor : m_interceptors) {
                interceptor->OnCaptureLost();
            }
        });
    }
}

InputRouter::~InputRouter() {
    if (m_captureService) {
        m_captureService->ForceRelease();
    }
}

void InputRouter::AddInterceptor(std::shared_ptr<IInputInterceptor> interceptor) {
    if (interceptor) {
        m_interceptors.push_back(interceptor);
    }
}

void InputRouter::RemoveInterceptor(std::shared_ptr<IInputInterceptor> interceptor) {
    m_interceptors.erase(std::remove(m_interceptors.begin(), m_interceptors.end(), interceptor), m_interceptors.end());
}


#define ROUTE_EVENT(method, event) \
    for (auto& interceptor : m_interceptors) { \
        if (interceptor->method(event) == EventResult::Handled) { \
            return EventResult::Handled; \
        } \
    } \
    if (m_stateMachine) { \
        return m_stateMachine->method(event); \
    } \
    return EventResult::Ignored;

EventResult InputRouter::RoutePointerDown(const PointerEvent& event) {
    ROUTE_EVENT(RoutePointerDown, event)
}

EventResult InputRouter::RoutePointerMove(const PointerEvent& event) {
    ROUTE_EVENT(RoutePointerMove, event)
}

EventResult InputRouter::RoutePointerUp(const PointerEvent& event) {
    ROUTE_EVENT(RoutePointerUp, event)
}

EventResult InputRouter::RoutePointerDoubleClick(const PointerEvent& event) {
    ROUTE_EVENT(RoutePointerDoubleClick, event)
}

EventResult InputRouter::RouteMouseWheel(const ScrollEvent& event) {
    ROUTE_EVENT(RouteMouseWheel, event)
}

EventResult InputRouter::RouteKeyDown(const KeyEvent& event) {
    ROUTE_EVENT(RouteKeyDown, event)
}

EventResult InputRouter::RouteKeyUp(const KeyEvent& event) {
    ROUTE_EVENT(RouteKeyUp, event)
}

EventResult InputRouter::RouteChar(const KeyEvent& event) {
    ROUTE_EVENT(RouteChar, event)
}

bool InputRouter::RouteSetCursor(HWND hwnd, UINT hitTest, UINT message) {
    (void)message;
    if (hitTest == HTCLIENT) {
        POINT pt;
        ::GetCursorPos(&pt);
        ::ScreenToClient(hwnd, &pt);
        float scale = ::GetDpiForWindow(hwnd) / 96.0f;
        if (scale <= 0.0f) scale = 1.0f;
        PointF dipPt = { static_cast<float>(pt.x) / scale, static_cast<float>(pt.y) / scale };

        HCURSOR hCursor = QueryCursor(dipPt);
        if (hCursor) {
            ::SetCursor(hCursor);
            return true;
        }
    }
    return false;
}

HCURSOR InputRouter::QueryCursor(const PointF& clientDip) {
    for (auto& interceptor : m_interceptors) {
        if (auto cursor = interceptor->QueryCursor(clientDip)) {
            return *cursor;
        }
    }
    if (m_stateMachine) {
        return m_stateMachine->GetCursor(clientDip);
    }
    return ::LoadCursor(nullptr, IDC_ARROW);
}

void InputRouter::OnCaptureLost() {
    if (m_captureService) {
        m_captureService->OnCaptureLost();
    }
    if (m_stateMachine) {
        m_stateMachine->CancelActiveInteractions();
    }
}

} // namespace ui::input
