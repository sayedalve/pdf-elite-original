#include "../../include/tools/ToolProperties.h"
#include "../../include/selection/CursorResolver.h"
#include "../../include/tools/ShapeTool.h"
#include "../../../pdf_engine/src/commands/AnnotationCommands.h"
#include <algorithm>
#include <cmath>
#include <wrl/client.h>

namespace ui::tools {

ShapeTool::ShapeTool(ToolType type) : m_type(type) {}
ShapeTool::~ShapeTool() = default;

std::wstring ShapeTool::GetName() const {
        switch (m_type) {
    case ToolType::Rectangle: return L"Rectangle";
    case ToolType::Ellipse: return L"Ellipse";
    case ToolType::Line: return L"Line";
    case ToolType::Arrow: return L"Arrow";
    case ToolType::AreaHighlight: return L"Area Highlight";
    default: return L"Shape";
    }
}

void ShapeTool::OnActivate(ToolContext& context) {
    (void)context;
    m_state = ToolState::Idle;
    m_pageIndex = -1;
}

void ShapeTool::OnDeactivate(ToolContext& context) {
    Cancel(context);
}

void ShapeTool::Cancel(ToolContext& context) {
    if (m_state == ToolState::Dragging && context.captureService) {
        context.captureService->ReleaseCapture(this);
    }
    m_state = ToolState::Idle;
    m_pageIndex = -1;
    if (context.invalidateView) {
        context.invalidateView();
    }
}

input::EventResult ShapeTool::OnPointerDown(const input::PointerEvent& event, ToolContext& context) {
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
        m_startPdf = pdfPt;
        m_currentPdf = pdfPt;
        m_shiftPressed = input::HasModifier(event.modifiers, input::KeyModifier::Shift);
        m_state = ToolState::Dragging;

        if (context.captureService && context.hwnd) {
            context.captureService->AcquireCapture(context.hwnd, this);
        }
        if (context.invalidateView) {
            context.invalidateView();
        }
        return input::EventResult::Handled;
    }
    return input::EventResult::Ignored;
}

input::EventResult ShapeTool::OnPointerMove(const input::PointerEvent& event, ToolContext& context) {
    if (m_state == ToolState::Dragging) {
        PointF pdfPt = event.pagePoint;
        if (event.pageIndex != m_pageIndex && context.canvasToPdf) {
            int dummy = -1;
            pdfPt = context.canvasToPdf(event.canvasPoint, dummy);
        }

        m_currentPdf = pdfPt;
        m_shiftPressed = input::HasModifier(event.modifiers, input::KeyModifier::Shift);

        if (context.invalidateView) {
            context.invalidateView();
        }
        return input::EventResult::Handled;
    }
    m_state = ToolState::Hovering;
    return input::EventResult::Ignored;
}

input::EventResult ShapeTool::OnPointerUp(const input::PointerEvent& event, ToolContext& context) {
    if (m_state == ToolState::Dragging) {
        if (context.captureService) {
            context.captureService->ReleaseCapture(this);
        }

        if (m_pageIndex >= 0 && context.document) {
            float x0 = (std::min)(m_startPdf.x, m_currentPdf.x);
            float y0 = (std::min)(m_startPdf.y, m_currentPdf.y);
            float x1 = (std::max)(m_startPdf.x, m_currentPdf.x);
            float y1 = (std::max)(m_startPdf.y, m_currentPdf.y);

            if (m_shiftPressed && (m_type == ToolType::Rectangle || m_type == ToolType::Ellipse)) {
                float side = (std::max)(x1 - x0, y1 - y0);
                x1 = x0 + side;
                y1 = y0 + side;
            }

            if (m_type == ToolType::Line || m_type == ToolType::Arrow) {
                float dx = m_currentPdf.x - m_startPdf.x;
                float dy = m_currentPdf.y - m_startPdf.y;
                if (std::abs(dx) < 4.0f && std::abs(dy) < 4.0f) {
                    m_currentPdf.x = m_startPdf.x + 50.0f;
                    m_currentPdf.y = m_startPdf.y + 50.0f;
                }
                x0 = (std::min)(m_startPdf.x, m_currentPdf.x);
                y0 = (std::min)(m_startPdf.y, m_currentPdf.y);
                x1 = (std::max)(m_startPdf.x, m_currentPdf.x);
                y1 = (std::max)(m_startPdf.y, m_currentPdf.y);
            } else {
                if (x1 - x0 < 4.0f) x1 = x0 + 50.0f;
                if (y1 - y0 < 4.0f) y1 = y0 + 50.0f;
            }

                        core::interfaces::dom::AnnotationType annType = core::interfaces::dom::AnnotationType::Square;
            if (m_type == ToolType::Ellipse) annType = core::interfaces::dom::AnnotationType::Circle;
            else if (m_type == ToolType::Line || m_type == ToolType::Arrow) annType = core::interfaces::dom::AnnotationType::Line;
            else if (m_type == ToolType::AreaHighlight) annType = core::interfaces::dom::AnnotationType::Highlight;

            auto cmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
                context.document, m_pageIndex, annType, RectF{ x0, y0, x1, y1 }
            );

            if (m_type == ToolType::Line || m_type == ToolType::Arrow) {
                core::interfaces::dom::LineGeometry lg;
                lg.start = m_startPdf;
                lg.end = m_currentPdf;
                if (m_type == ToolType::Arrow) {
                    lg.endEnding = core::interfaces::dom::LineEnding::ClosedArrow;
                }
                cmd->SetLineGeometry(lg);
            }

            if (m_type == ToolType::AreaHighlight) {
                cmd->SetColor(static_cast<int>(ui::tools::ToolProperties::areaHighlightRed * 255), static_cast<int>(ui::tools::ToolProperties::areaHighlightGreen * 255), static_cast<int>(ui::tools::ToolProperties::areaHighlightBlue * 255), static_cast<int>(ui::tools::ToolProperties::areaHighlightAlpha * 255));
            } else {
                cmd->SetColor(static_cast<int>(ui::tools::ToolProperties::shapeRed * 255), static_cast<int>(ui::tools::ToolProperties::shapeGreen * 255), static_cast<int>(ui::tools::ToolProperties::shapeBlue * 255), static_cast<int>(ui::tools::ToolProperties::shapeAlpha * 255));
                cmd->SetBorderWidth(ui::tools::ToolProperties::shapeWidth);
            
            }

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
        }

        m_state = ToolState::Committed;
        m_state = ToolState::Idle;
        m_pageIndex = -1;

        if (context.invalidateView) {
            context.invalidateView();
        }
        return input::EventResult::Handled;
    }
    (void)event;
    return input::EventResult::Ignored;
}

HCURSOR ShapeTool::GetCursor(const PointF& point, ToolContext& context) const {
    (void)point;
    (void)context;
    return ui::selection::CursorResolver::ResolveToolCursor(m_type, m_state);
}

void ShapeTool::RenderOverlay(ID2D1RenderTarget* renderTarget, ToolContext& context) {
    if (!renderTarget || m_state != ToolState::Dragging || m_pageIndex < 0 || !context.pdfToCanvas) {
        return;
    }

    PointF c0 = context.pdfToCanvas(m_pageIndex, m_startPdf);
    PointF c1 = context.pdfToCanvas(m_pageIndex, m_currentPdf);

    if (context.canvasToDip) {
        c0 = context.canvasToDip(c0);
        c1 = context.canvasToDip(c1);
    }

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    if (SUCCEEDED(renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.47f, 0.84f, 0.8f), &brush)) && brush) {
        float left = (std::min)(c0.x, c1.x);
        float right = (std::max)(c0.x, c1.x);
        float top = (std::min)(c0.y, c1.y);
        float bottom = (std::max)(c0.y, c1.y);

        if (m_type == ToolType::AreaHighlight) {
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> fillBrush;
            renderTarget->CreateSolidColorBrush(D2D1::ColorF(ui::tools::ToolProperties::areaHighlightRed, ui::tools::ToolProperties::areaHighlightGreen, ui::tools::ToolProperties::areaHighlightBlue, ui::tools::ToolProperties::areaHighlightAlpha), &fillBrush);
            if (fillBrush) {
                renderTarget->FillRectangle(D2D1::RectF(left, top, right, bottom), fillBrush.Get());
            }
        } else if (m_type == ToolType::Rectangle) {
            renderTarget->DrawRectangle(D2D1::RectF(left, top, right, bottom), brush.Get(), ui::tools::ToolProperties::shapeWidth);
        } else if (m_type == ToolType::Ellipse) {
            D2D1_ELLIPSE ellipse = D2D1::Ellipse(
                D2D1::Point2F((left + right) * 0.5f, (top + bottom) * 0.5f),
                (right - left) * 0.5f,
                (bottom - top) * 0.5f
            );
            renderTarget->DrawEllipse(ellipse, brush.Get(), ui::tools::ToolProperties::shapeWidth);
        } else if (m_type == ToolType::Line || m_type == ToolType::Arrow) {
            renderTarget->DrawLine(D2D1::Point2F(c0.x, c0.y), D2D1::Point2F(c1.x, c1.y), brush.Get(), ui::tools::ToolProperties::shapeWidth);
        }
    }
}

} // namespace ui::tools








