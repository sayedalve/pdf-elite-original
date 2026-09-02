#include "../../include/tools/StampTool.h"
#include "../../../pdf_engine/src/commands/AnnotationCommands.h"

namespace ui::tools {

StampTool::StampTool() = default;
StampTool::~StampTool() = default;

void StampTool::OnActivate(ToolContext& context) {
    (void)context;
    m_state = ToolState::Idle;
}

void StampTool::OnDeactivate(ToolContext& context) {
    Cancel(context);
}

void StampTool::Cancel(ToolContext& context) {
    m_state = ToolState::Idle;
    if (context.invalidateView) context.invalidateView();
}

input::EventResult StampTool::OnPointerDown(const input::PointerEvent& event, ToolContext& context) {
    if (event.button != input::PointerButton::Left) {
        return input::EventResult::Ignored;
    }

    int pageIdx = event.pageIndex;
    PointF pdfPt = event.pagePoint;
    if (pageIdx < 0 && context.canvasToPdf) {
        pdfPt = context.canvasToPdf(event.canvasPoint, pageIdx);
    }

    if (pageIdx >= 0 && context.document) {
        float stampW = 80.0f, stampH = 24.0f;
        RectF bounds = { pdfPt.x - stampW * 0.5f, pdfPt.y - stampH * 0.5f, pdfPt.x + stampW * 0.5f, pdfPt.y + stampH * 0.5f };
        auto cmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
            context.document, pageIdx, core::interfaces::dom::AnnotationType::Stamp, bounds
        );

        auto* cmdRaw = cmd.get();
        bool ok = false;
        if (context.executeCommand) {
            context.executeCommand(std::move(cmd));
            ok = true;
        } else {
            ok = context.document->GetCommandStack().ExecuteCommand(std::move(cmd));
        }

        if (ok && !m_stampLabel.empty()) {
            auto addedAnnot = cmdRaw->GetAnnotation();
            if (addedAnnot) {
                int utf8Len = WideCharToMultiByte(CP_UTF8, 0, m_stampLabel.c_str(), -1, nullptr, 0, nullptr, nullptr);
                std::string label(utf8Len > 0 ? utf8Len - 1 : 0, '\0');
                if (utf8Len > 0) WideCharToMultiByte(CP_UTF8, 0, m_stampLabel.c_str(), -1, label.data(), utf8Len, nullptr, nullptr);
                addedAnnot->SetContents(label);
            }
        }

        m_state = ToolState::Committed;
        m_state = ToolState::Idle;
        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }
    return input::EventResult::Ignored;
}

input::EventResult StampTool::OnPointerMove(const input::PointerEvent& event, ToolContext& context) {
    (void)event; (void)context;
    return input::EventResult::Ignored;
}

input::EventResult StampTool::OnPointerUp(const input::PointerEvent& event, ToolContext& context) {
    (void)event; (void)context;
    return input::EventResult::Ignored;
}

HCURSOR StampTool::GetCursor(const PointF& point, ToolContext& context) const {
    (void)point; (void)context;
    return ::LoadCursor(nullptr, IDC_CROSS);
}

void StampTool::RenderOverlay(ID2D1RenderTarget* renderTarget, ToolContext& context) {
    (void)renderTarget; (void)context;
}

} // namespace ui::tools
