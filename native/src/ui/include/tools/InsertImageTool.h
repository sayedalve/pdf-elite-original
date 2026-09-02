#pragma once

#include "ITool.h"
#include <vector>

namespace ui::tools {

class InsertImageTool : public ITool {
public:
    InsertImageTool();
    ~InsertImageTool() override;

    ToolType GetType() const override { return ToolType::InsertImage; }
    std::wstring GetName() const override { return L"InsertImage"; }
    ToolState GetState() const override { return m_state; }

    void SetPendingImage(std::vector<uint8_t> data, uint32_t width, uint32_t height) {
        m_imageData = std::move(data);
        m_imageWidth = width;
        m_imageHeight = height;
    }

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
    std::vector<uint8_t> m_imageData;
    uint32_t m_imageWidth = 0;
    uint32_t m_imageHeight = 0;
};

} // namespace ui::tools
