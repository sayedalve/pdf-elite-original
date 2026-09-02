#include "../../include/tools/ToolProperties.h"
#include "../../include/selection/CursorResolver.h"
#include "../../include/tools/FreeTextTool.h"
#include "../../../pdf_engine/src/commands/AnnotationCommands.h"

namespace ui::tools {

FreeTextTool::FreeTextTool(ToolType type) : m_type(type) {}
FreeTextTool::~FreeTextTool() = default;

std::wstring FreeTextTool::GetName() const {
    switch (m_type) {
    case ToolType::AddText: return L"AddText";
    case ToolType::EditText: return L"EditText";
    default: return L"FreeText";
    }
}

void FreeTextTool::OnActivate(ToolContext& context) {
    (void)context;
    m_state = ToolState::Idle;
}

void FreeTextTool::OnDeactivate(ToolContext& context) {
    Cancel(context);
}

void FreeTextTool::Cancel(ToolContext& context) {
    m_state = ToolState::Idle;
    if (context.invalidateView) context.invalidateView();
}

input::EventResult FreeTextTool::OnPointerDown(const input::PointerEvent& event, ToolContext& context) {
    if (event.button != input::PointerButton::Left) {
        return input::EventResult::Ignored;
    }

    int pageIdx = event.pageIndex;
    PointF pdfPt = event.pagePoint;
    if (pageIdx < 0 && context.canvasToPdf) {
        pdfPt = context.canvasToPdf(event.canvasPoint, pageIdx);
    }

    if (pageIdx >= 0 && context.document) {
        // PDF coordinates usually have y growing up, but existing code used y + 30. Let's just create a box.
        RectF bounds = { pdfPt.x, pdfPt.y - 30.0f, pdfPt.x + 150.0f, pdfPt.y };
        if (bounds.top < bounds.bottom) {
            std::swap(bounds.top, bounds.bottom);
        }

        auto cmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
            context.document, pageIdx, core::interfaces::dom::AnnotationType::FreeText, bounds
        );

        if (m_type == ToolType::TypeWriter || m_type == ToolType::AddText) {
            cmd->SetBorderWidth(0.0f);
        } else if (m_type == ToolType::TextBox) {
            cmd->SetBorderWidth(1.0f);
        } else if (m_type == ToolType::TextCallout) {
            cmd->SetBorderWidth(1.0f);
            core::interfaces::dom::LineGeometry lg;
            // A simple callout line from left-bottom towards bottom-left
            lg.start = { pdfPt.x - 40.0f, pdfPt.y - 40.0f };
            lg.end = { pdfPt.x, pdfPt.y - 15.0f };
            lg.endEnding = core::interfaces::dom::LineEnding::OpenArrow;
            cmd->SetLineGeometry(lg);
        }

        auto rawCmd = cmd.get();
        if (context.executeCommand) {
            context.executeCommand(std::move(cmd));
        } else {
            context.document->GetCommandStack().ExecuteCommand(std::move(cmd));
        }

        if (auto annot = rawCmd->GetAnnotation()) {
            if (context.enterAnnotationEditMode) {
                context.enterAnnotationEditMode(annot);
            }
        }

        m_state = ToolState::Committed;
        m_state = ToolState::Idle;
        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }
    return input::EventResult::Ignored;
}

HCURSOR FreeTextTool::GetCursor(const PointF& point, ToolContext& context) const {
    (void)point; (void)context;
    return ui::selection::CursorResolver::ResolveToolCursor(m_type, m_state);
}

input::EventResult FreeTextTool::OnPointerMove(const input::PointerEvent& event, ToolContext& context) {
    (void)event; (void)context;
    return input::EventResult::Ignored;
}

input::EventResult FreeTextTool::OnPointerUp(const input::PointerEvent& event, ToolContext& context) {
    (void)event; (void)context;
    return input::EventResult::Ignored;
}


void FreeTextTool::RenderOverlay(ID2D1RenderTarget* renderTarget, ToolContext& context) {
    (void)renderTarget; (void)context;
}

} // namespace ui::tools



