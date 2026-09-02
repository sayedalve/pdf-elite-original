#pragma once

#include "ITool.h"
#include <string>

namespace ui::tools {

class StampTool : public ITool {
public:
    StampTool();
    ~StampTool() override;

    ToolType GetType() const override { return ToolType::Stamp; }
    std::wstring GetName() const override { return L"Stamp"; }
    ToolState GetState() const override { return m_state; }

    void SetStampLabel(const std::wstring& label) { m_stampLabel = label; }
    const std::wstring& GetStampLabel() const { return m_stampLabel; }

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
    std::wstring m_stampLabel = L"APPROVED";
};

} // namespace ui::tools
