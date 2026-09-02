#include "../../include/tools/ToolProperties.h"
#include "../../include/selection/CursorResolver.h"
#include "../../include/tools/StickyNoteTool.h"
#include "../../../pdf_engine/src/commands/AnnotationCommands.h"

namespace ui::tools {

StickyNoteTool::StickyNoteTool() = default;
StickyNoteTool::~StickyNoteTool() = default;

void StickyNoteTool::OnActivate(ToolContext& context) {
    (void)context;
    m_state = ToolState::Idle;
}

void StickyNoteTool::OnDeactivate(ToolContext& context) {
    Cancel(context);
}

void StickyNoteTool::Cancel(ToolContext& context) {
    m_state = ToolState::Idle;
    if (context.invalidateView) context.invalidateView();
}

input::EventResult StickyNoteTool::OnPointerDown(const input::PointerEvent& event, ToolContext& context) {
    if (event.button != input::PointerButton::Left) {
        return input::EventResult::Ignored;
    }

    int pageIdx = event.pageIndex;
    PointF pdfPt = event.pagePoint;
    if (pageIdx < 0 && context.canvasToPdf) {
        pdfPt = context.canvasToPdf(event.canvasPoint, pageIdx);
    }

    if (pageIdx >= 0 && context.document) {
        RectF bounds = { pdfPt.x, pdfPt.y, pdfPt.x + 24.0f, pdfPt.y + 24.0f };
        auto cmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
            context.document, pageIdx, core::interfaces::dom::AnnotationType::Text, bounds
        );

        auto rawCmd = cmd.get();
        if (context.executeCommand) {
            context.executeCommand(std::move(cmd));
        } else {
            context.document->GetCommandStack().ExecuteCommand(std::move(cmd));
        }

        if (auto annot = rawCmd->GetAnnotation()) {
            if (context.openAnnotationPopup) {
                context.openAnnotationPopup(annot->GetId());
            }
        }

        m_state = ToolState::Committed;
        m_state = ToolState::Idle;
        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }
    return input::EventResult::Ignored;
}

input::EventResult StickyNoteTool::OnPointerMove(const input::PointerEvent& event, ToolContext& context) {
    (void)event; (void)context;
    return input::EventResult::Ignored;
}

input::EventResult StickyNoteTool::OnPointerUp(const input::PointerEvent& event, ToolContext& context) {
    (void)event; (void)context;
    return input::EventResult::Ignored;
}

HCURSOR StickyNoteTool::GetCursor(const PointF& point, ToolContext& context) const {
    (void)point; (void)context;
    return ui::selection::CursorResolver::ResolveToolCursor(ToolType::StickyNote, m_state);
}

void StickyNoteTool::RenderOverlay(ID2D1RenderTarget* renderTarget, ToolContext& context) {
    (void)renderTarget; (void)context;
}

} // namespace ui::tools



