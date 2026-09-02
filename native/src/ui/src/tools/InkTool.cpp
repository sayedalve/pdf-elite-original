#include "../../include/tools/ToolProperties.h"
#include "../../include/selection/CursorResolver.h"
#include "../../include/tools/InkTool.h"
#include "../../../pdf_engine/src/commands/AnnotationCommands.h"
#include <algorithm>
#include <wrl/client.h>

namespace ui::tools {

InkTool::InkTool() = default;
InkTool::~InkTool() = default;

void InkTool::OnActivate(ToolContext& context) {
    (void)context;
    m_state = ToolState::Idle;
    m_pageIndex = -1;
    m_points.clear();
}

void InkTool::OnDeactivate(ToolContext& context) {
    Cancel(context);
}

void InkTool::Cancel(ToolContext& context) {
    if (m_state == ToolState::Dragging && context.captureService) {
        context.captureService->ReleaseCapture(this);
    }
    m_state = ToolState::Idle;
    m_pageIndex = -1;
    m_points.clear();
    if (context.invalidateView) {
        context.invalidateView();
    }
}

input::EventResult InkTool::OnPointerDown(const input::PointerEvent& event, ToolContext& context) {
    if (event.button != input::PointerButton::Left) {
        return input::EventResult::Ignored;
    }

    int pageIdx = event.pageIndex;
    PointF pdfPt = event.pagePoint;
    if (pageIdx < 0 && context.canvasToPdf) {
        pdfPt = context.canvasToPdf(event.canvasPoint, pageIdx);
    }

    if (pageIdx >= 0) {
        m_pageIndex = pageIdx;
        m_points.clear();
        m_points.push_back(pdfPt);
        m_state = ToolState::Dragging;

        if (context.captureService && context.hwnd) {
            context.captureService->AcquireCapture(context.hwnd, this);
        }
        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }
    return input::EventResult::Ignored;
}

input::EventResult InkTool::OnPointerMove(const input::PointerEvent& event, ToolContext& context) {
    if (m_state == ToolState::Dragging) {
        PointF pdfPt = event.pagePoint;
        if (event.pageIndex != m_pageIndex && context.canvasToPdf) {
            int dummy = -1;
            pdfPt = context.canvasToPdf(event.canvasPoint, dummy);
        }

        m_points.push_back(pdfPt);
        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }
    m_state = ToolState::Hovering;
    return input::EventResult::Ignored;
}

input::EventResult InkTool::OnPointerUp(const input::PointerEvent& event, ToolContext& context) {
    if (m_state == ToolState::Dragging) {
        if (context.captureService) {
            context.captureService->ReleaseCapture(this);
        }

        if (m_pageIndex >= 0 && m_points.size() >= 2 && context.document) {
            float minX = m_points[0].x, maxX = m_points[0].x;
            float minY = m_points[0].y, maxY = m_points[0].y;
            for (const auto& pt : m_points) {
                minX = (std::min)(minX, pt.x);
                maxX = (std::max)(maxX, pt.x);
                minY = (std::min)(minY, pt.y);
                maxY = (std::max)(maxY, pt.y);
            }

            RectF bounds = { minX, minY, maxX, maxY };
            std::vector<std::vector<PointF>> strokes = { m_points };
            auto cmd = std::make_unique<pdf_engine::commands::AddInkAnnotationCommand>(
                context.document, m_pageIndex, strokes, bounds, static_cast<uint8_t>(ui::tools::ToolProperties::pencilRed * 255), static_cast<uint8_t>(ui::tools::ToolProperties::pencilGreen * 255), static_cast<uint8_t>(ui::tools::ToolProperties::pencilBlue * 255), static_cast<uint8_t>(ui::tools::ToolProperties::pencilAlpha * 255), ui::tools::ToolProperties::pencilWidth
            );

            auto groupCmd = std::make_unique<core::CommandGroup>("Ink Strokes", ::GetTickCount64(), std::move(cmd));

            if (context.executeCommand) {
                context.executeCommand(std::move(groupCmd));
            } else {
                context.document->GetCommandStack().ExecuteCommand(std::move(groupCmd));
            }
        }

        m_state = ToolState::Committed;
        m_state = ToolState::Idle;
        m_pageIndex = -1;
        m_points.clear();

        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }
    (void)event;
    return input::EventResult::Ignored;
}

HCURSOR InkTool::GetCursor(const PointF& point, ToolContext& context) const {
    (void)point;
    (void)context;
    return ui::selection::CursorResolver::ResolveToolCursor(ToolType::Ink, m_state);
}

void InkTool::RenderOverlay(ID2D1RenderTarget* renderTarget, ToolContext& context) {
    if (!renderTarget || m_state != ToolState::Dragging || m_pageIndex < 0 || m_points.size() < 2 || !context.pdfToCanvas) {
        return;
    }

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    if (SUCCEEDED(renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush)) && brush) {
        for (size_t i = 1; i < m_points.size(); ++i) {
            PointF p0 = context.pdfToCanvas(m_pageIndex, m_points[i - 1]);
            PointF p1 = context.pdfToCanvas(m_pageIndex, m_points[i]);
            if (context.canvasToDip) {
                p0 = context.canvasToDip(p0);
                p1 = context.canvasToDip(p1);
            }
            renderTarget->DrawLine(D2D1::Point2F(p0.x, p0.y), D2D1::Point2F(p1.x, p1.y), brush.Get(), 2.0f);
        }
    }
}

} // namespace ui::tools





