#pragma once

#include "ITool.h"

namespace ui::tools {

class PanTool : public ITool {
public:
    PanTool();
    ~PanTool() override;

    ToolType GetType() const override { return ToolType::Pan; }
    std::wstring GetName() const override { return L"Pan"; }
    ToolState GetState() const override { return m_state; }

    void OnActivate(ToolContext& context) override;
    void OnDeactivate(ToolContext& context) override;
    void Cancel(ToolContext& context) override;

    input::EventResult OnPointerDown(const input::PointerEvent& event, ToolContext& context) override;
    input::EventResult OnPointerMove(const input::PointerEvent& event, ToolContext& context) override;
    input::EventResult OnPointerUp(const input::PointerEvent& event, ToolContext& context) override;

    HCURSOR GetCursor(const PointF& point, ToolContext& context) const override;
    void RenderOverlay(ID2D1RenderTarget* renderTarget, ToolContext& context) override;

private:
    ToolState m_state = ToolState::Idle;
    PointF m_startDip = {0.0f, 0.0f};
    PointF m_lastDip = {0.0f, 0.0f};
    bool m_isDragging = false;
};

} // namespace ui::tools
