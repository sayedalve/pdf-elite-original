#pragma once

#include "ITool.h"
#include <vector>

namespace ui::tools {

class InkTool : public ITool {
public:
    InkTool();
    ~InkTool() override;

    ToolType GetType() const override { return ToolType::Ink; }
    std::wstring GetName() const override { return L"Ink"; }
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
    int m_pageIndex = -1;
    std::vector<PointF> m_points; // PDF coordinates
};

} // namespace ui::tools
