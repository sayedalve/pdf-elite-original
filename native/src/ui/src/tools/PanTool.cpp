#include "../../include/tools/PanTool.h"

namespace ui::tools {

PanTool::PanTool() = default;
PanTool::~PanTool() = default;

void PanTool::OnActivate(ToolContext& context) {
    (void)context;
    m_state = ToolState::Idle;
    m_isDragging = false;
}

void PanTool::OnDeactivate(ToolContext& context) {
    Cancel(context);
}

void PanTool::Cancel(ToolContext& context) {
    if (m_isDragging && context.captureService) {
        context.captureService->ReleaseCapture(this);
    }
    m_isDragging = false;
    m_state = ToolState::Idle;
    if (context.invalidateView) {
        context.invalidateView();
    }
}

input::EventResult PanTool::OnPointerDown(const input::PointerEvent& event, ToolContext& context) {
    if (event.button == input::PointerButton::Left) {
        m_state = ToolState::Dragging;
        m_isDragging = true;
        m_startDip = event.clientDip;
        m_lastDip = event.clientDip;
        if (context.captureService && context.hwnd) {
            context.captureService->AcquireCapture(context.hwnd, this);
        }
        return input::EventResult::Handled;
    }
    return input::EventResult::Ignored;
}

input::EventResult PanTool::OnPointerMove(const input::PointerEvent& event, ToolContext& context) {
    if (m_isDragging) {
        float dx = event.clientDip.x - m_lastDip.x;
        float dy = event.clientDip.y - m_lastDip.y;
        m_lastDip = event.clientDip;

        if (context.scrollViewport) {
            context.scrollViewport(-dx, -dy);
        }
        if (context.invalidateView) {
            context.invalidateView();
        }
        return input::EventResult::Handled;
    }

    m_state = ToolState::Hovering;
    return input::EventResult::Ignored;
}

input::EventResult PanTool::OnPointerUp(const input::PointerEvent& event, ToolContext& context) {
    if (m_isDragging && (event.button == input::PointerButton::Left || event.button == input::PointerButton::None)) {
        if (context.captureService) {
            context.captureService->ReleaseCapture(this);
        }
        m_isDragging = false;
        m_state = ToolState::Committed;
        m_state = ToolState::Idle;
        if (context.invalidateView) {
            context.invalidateView();
        }
        return input::EventResult::Handled;
    }
    return input::EventResult::Ignored;
}

HCURSOR PanTool::GetCursor(const PointF& point, ToolContext& context) const {
    (void)point;
    (void)context;
    if (m_isDragging || m_state == ToolState::Dragging) {
        return ::LoadCursor(nullptr, IDC_SIZEALL);
    }
    return ::LoadCursor(nullptr, IDC_HAND);
}

void PanTool::RenderOverlay(ID2D1RenderTarget* renderTarget, ToolContext& context) {
    (void)renderTarget;
    (void)context;
}

} // namespace ui::tools
