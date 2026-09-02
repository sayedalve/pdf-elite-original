#include "../../include/tools/InsertImageTool.h"
#include "../../../pdf_engine/src/commands/ImageCommands.h"

namespace ui::tools {

InsertImageTool::InsertImageTool() = default;
InsertImageTool::~InsertImageTool() = default;

void InsertImageTool::OnActivate(ToolContext& context) {
    (void)context;
    m_state = ToolState::Idle;
}

void InsertImageTool::OnDeactivate(ToolContext& context) {
    Cancel(context);
}

void InsertImageTool::Cancel(ToolContext& context) {
    m_state = ToolState::Idle;
    m_imageData.clear();
    if (context.invalidateView) context.invalidateView();
}

input::EventResult InsertImageTool::OnPointerDown(const input::PointerEvent& event, ToolContext& context) {
    if (event.button != input::PointerButton::Left) {
        return input::EventResult::Ignored;
    }

    int pageIdx = event.pageIndex;
    PointF pdfPt = event.pagePoint;
    if (pageIdx < 0 && context.canvasToPdf) {
        pdfPt = context.canvasToPdf(event.canvasPoint, pageIdx);
    }

    if (pageIdx >= 0 && context.document && !m_imageData.empty()) {
        RectF bounds = { pdfPt.x, pdfPt.y, pdfPt.x + 100.0f, pdfPt.y + 100.0f };
        auto cmd = std::make_unique<pdf_engine::commands::InsertImageCommand>(
            context.document, pageIdx, m_imageData, m_imageWidth, m_imageHeight, bounds
        );

        if (context.executeCommand) {
            context.executeCommand(std::move(cmd));
        } else {
            context.document->GetCommandStack().ExecuteCommand(std::move(cmd));
        }

        m_imageData.clear();
        m_state = ToolState::Committed;
        m_state = ToolState::Idle;
        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }
    return input::EventResult::Ignored;
}

input::EventResult InsertImageTool::OnPointerMove(const input::PointerEvent& event, ToolContext& context) {
    (void)event; (void)context;
    return input::EventResult::Ignored;
}

input::EventResult InsertImageTool::OnPointerUp(const input::PointerEvent& event, ToolContext& context) {
    (void)event; (void)context;
    return input::EventResult::Ignored;
}

HCURSOR InsertImageTool::GetCursor(const PointF& point, ToolContext& context) const {
    (void)point; (void)context;
    return ::LoadCursor(nullptr, IDC_CROSS);
}

void InsertImageTool::RenderOverlay(ID2D1RenderTarget* renderTarget, ToolContext& context) {
    (void)renderTarget; (void)context;
}

} // namespace ui::tools
