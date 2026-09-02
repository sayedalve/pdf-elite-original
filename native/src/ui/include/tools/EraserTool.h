#pragma once

#include "ITool.h"

namespace ui::tools {

class EraserTool : public ITool {
public:
    EraserTool();
    ~EraserTool() override;

    ToolType GetType() const override { return ToolType::Eraser; }
    std::wstring GetName() const override { return L"Eraser"; }
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
    bool EraseAt(const PointF& pdfPt, int pageIndex, ToolContext& context);
    ToolState m_state = ToolState::Idle;
    bool m_isErasing = false;
};

} // namespace ui::tools
