#include "../../include/tools/ToolProperties.h"
#include "../../include/selection/CursorResolver.h"
#include "../../include/tools/EraserTool.h"
#include "../../../pdf_engine/src/commands/AnnotationCommands.h"
#include "../../../core/interfaces/dom/IPage.h"
#include <windows.h>
namespace ui::tools {

EraserTool::EraserTool() = default;
EraserTool::~EraserTool() = default;

void EraserTool::OnActivate(ToolContext& context) {
    (void)context;
    m_state = ToolState::Idle;
    m_isErasing = false;
}

void EraserTool::OnDeactivate(ToolContext& context) {
    Cancel(context);
}

void EraserTool::Cancel(ToolContext& context) {
    if (m_isErasing && context.captureService) {
        context.captureService->ReleaseCapture(this);
    }
    m_isErasing = false;
    m_state = ToolState::Idle;
    if (context.invalidateView) context.invalidateView();
}

bool EraserTool::EraseAt(const PointF& pdfPt, int pageIndex, ToolContext& context) {
    if (pageIndex < 0 || !context.document) return false;

    auto page = context.document->GetPage(pageIndex);
    if (!page) return false;

    auto annots = page->GetAnnotations();
    for (auto& annot : annots) {
        if (annot) {
            auto b = annot->GetBounds();
            if (pdfPt.x >= b.left && pdfPt.x <= b.right && pdfPt.y >= b.top && pdfPt.y <= b.bottom) {
                // If the prompt specifically wants only pencil strokes, we could check:
                if (annot->GetType() != core::interfaces::dom::AnnotationType::Ink) continue;
                
                auto cmd = std::make_unique<pdf_engine::commands::DeleteAnnotationCommand>(
                    context.document, pageIndex, annot
                );
                auto groupCmd = std::make_unique<core::CommandGroup>("Erase Annotation", ::GetTickCount64(), std::move(cmd));
                if (context.executeCommand) {
                    context.executeCommand(std::move(groupCmd));
                } else {
                    context.document->GetCommandStack().ExecuteCommand(std::move(groupCmd));
                }
                return true;
            }
        }
    }
    return false;
}

input::EventResult EraserTool::OnPointerDown(const input::PointerEvent& event, ToolContext& context) {
    if (event.button != input::PointerButton::Left) {
        return input::EventResult::Ignored;
    }

    int pageIdx = event.pageIndex;
    PointF pdfPt = event.pagePoint;
    if (pageIdx < 0 && context.canvasToPdf) {
        pdfPt = context.canvasToPdf(event.canvasPoint, pageIdx);
    }

    m_isErasing = true;
    m_state = ToolState::Dragging;
    if (context.captureService && context.hwnd) {
        context.captureService->AcquireCapture(context.hwnd, this);
    }

    if (pageIdx >= 0) {
        EraseAt(pdfPt, pageIdx, context);
    }
    return input::EventResult::Handled;
}

input::EventResult EraserTool::OnPointerMove(const input::PointerEvent& event, ToolContext& context) {
    if (m_isErasing) {
        int pageIdx = event.pageIndex;
        PointF pdfPt = event.pagePoint;
        if (pageIdx < 0 && context.canvasToPdf) {
            pdfPt = context.canvasToPdf(event.canvasPoint, pageIdx);
        }
        if (pageIdx >= 0) {
            EraseAt(pdfPt, pageIdx, context);
        }
        return input::EventResult::Handled;
    }
    m_state = ToolState::Hovering;
    return input::EventResult::Ignored;
}

input::EventResult EraserTool::OnPointerUp(const input::PointerEvent& event, ToolContext& context) {
    if (m_isErasing) {
        if (context.captureService) {
            context.captureService->ReleaseCapture(this);
        }
        m_isErasing = false;
        m_state = ToolState::Committed;
        m_state = ToolState::Idle;
        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }
    (void)event;
    return input::EventResult::Ignored;
}

HCURSOR EraserTool::GetCursor(const PointF& point, ToolContext& context) const {
    (void)point; (void)context;
    return ui::selection::CursorResolver::ResolveToolCursor(ToolType::Eraser, m_state);
}

void EraserTool::RenderOverlay(ID2D1RenderTarget* renderTarget, ToolContext& context) {
    (void)renderTarget; (void)context;
}

} // namespace ui::tools




