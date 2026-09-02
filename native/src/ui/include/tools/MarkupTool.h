#pragma once

#include "ITool.h"

namespace ui::tools {

class MarkupTool : public ITool {
public:
    explicit MarkupTool(ToolType type = ToolType::Highlight);
    ~MarkupTool() override;

    ToolType GetType() const override { return m_type; }
    std::wstring GetName() const override;
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
    ToolType m_type;
    ToolState m_state = ToolState::Idle;
    int m_pageIndex = -1;
    PointF m_startPdf = { 0.0f, 0.0f };
    PointF m_currentPdf = { 0.0f, 0.0f };
    int m_startChar = -1;
    int m_endChar = -1;
};

} // namespace ui::tools
