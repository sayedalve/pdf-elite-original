#pragma once

#include "IInputRouter.h"
#include "IInputInterceptor.h"
#include "PointerCaptureService.h"
#include "../tools/ToolStateMachine.h"
#include <memory>

namespace ui::input {

class InputRouter : public IInputRouter {
public:
    InputRouter(std::shared_ptr<IPointerCaptureService> captureService,
                std::shared_ptr<tools::ToolStateMachine> stateMachine);
    ~InputRouter() override;

    void AddInterceptor(std::shared_ptr<IInputInterceptor> interceptor);
    void RemoveInterceptor(std::shared_ptr<IInputInterceptor> interceptor);

    EventResult RoutePointerDown(const PointerEvent& event) override;
    EventResult RoutePointerMove(const PointerEvent& event) override;
    EventResult RoutePointerUp(const PointerEvent& event) override;
    EventResult RoutePointerDoubleClick(const PointerEvent& event) override;
    EventResult RouteMouseWheel(const ScrollEvent& event) override;
    EventResult RouteKeyDown(const KeyEvent& event) override;
    EventResult RouteKeyUp(const KeyEvent& event) override;
    EventResult RouteChar(const KeyEvent& event) override;
    bool RouteSetCursor(HWND hwnd, UINT hitTest, UINT message) override;
    HCURSOR QueryCursor(const PointF& clientDip) override;
    void OnCaptureLost() override;

    std::shared_ptr<IPointerCaptureService> GetCaptureService() const { return m_captureService; }
    std::shared_ptr<tools::ToolStateMachine> GetStateMachine() const { return m_stateMachine; }

    void SetHwnd(HWND hwnd) { m_hwnd = hwnd; }
    HWND GetHwnd() const { return m_hwnd; }

private:
    std::shared_ptr<IPointerCaptureService> m_captureService;
    std::shared_ptr<tools::ToolStateMachine> m_stateMachine;
    std::vector<std::shared_ptr<IInputInterceptor>> m_interceptors;
    HWND m_hwnd = nullptr;
};

} // namespace ui::input
