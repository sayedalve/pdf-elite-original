#include "ShapeAnnotationHandler.h"
#include "../../pdf_engine/src/commands/AnnotationCommands.h"
#include <algorithm>
#include <cmath>

namespace ui::annotation {

ShapeAnnotationHandler::ShapeAnnotationHandler(ToolMode toolMode)
    : m_toolMode(toolMode) {
}

void ShapeAnnotationHandler::Initialize(const AnnotationHandlerContext& context) {
    m_context = context;
    m_state = InteractionState::Idle;
}

bool ShapeAnnotationHandler::OnMouseDown(const MouseEvent& event) {
    if (event.pageIndex < 0) return false;
    m_state = InteractionState::Creating;
    m_pageIndex = event.pageIndex;
    m_startPdf = {static_cast<float>(event.pdfX), static_cast<float>(event.pdfY)};
    m_currentPdf = m_startPdf;
    m_shiftPressed = event.shift;

    if (m_context.hwnd) SetCapture(m_context.hwnd);
    if (m_context.invalidateView) m_context.invalidateView();
    return true;
}

bool ShapeAnnotationHandler::OnMouseMove(const MouseEvent& event) {
    if (m_state != InteractionState::Creating) return false;
    m_currentPdf = {static_cast<float>(event.pdfX), static_cast<float>(event.pdfY)};
    m_shiftPressed = event.shift;

    if (m_shiftPressed) {
        float dx = m_currentPdf.x - m_startPdf.x;
        float dy = m_currentPdf.y - m_startPdf.y;
        if (m_toolMode == ToolMode::Rectangle || m_toolMode == ToolMode::Ellipse) {
            float size = std::max(std::abs(dx), std::abs(dy));
            m_currentPdf.x = m_startPdf.x + (dx >= 0 ? size : -size);
            m_currentPdf.y = m_startPdf.y + (dy >= 0 ? size : -size);
        } else if (m_toolMode == ToolMode::Line || m_toolMode == ToolMode::Arrow) {
            // Snap to 0, 45, 90 degrees
            double angle = std::atan2(dy, dx);
            double snapped = std::round(angle / (3.141592653589793 / 4.0)) * (3.141592653589793 / 4.0);
            double length = std::sqrt(dx * dx + dy * dy);
            m_currentPdf.x = m_startPdf.x + static_cast<float>(length * std::cos(snapped));
            m_currentPdf.y = m_startPdf.y + static_cast<float>(length * std::sin(snapped));
        }
    }

    if (m_context.invalidateView) m_context.invalidateView();
    return true;
}

bool ShapeAnnotationHandler::OnMouseUp(const MouseEvent& event) {
    (void)event;
    if (m_state != InteractionState::Creating) return false;
    if (m_context.hwnd && GetCapture() == m_context.hwnd) {
        ReleaseCapture();
    }

    if (m_pageIndex >= 0 && m_context.getDocument) {
        auto* doc = m_context.getDocument();
        if (doc) {
            float x0 = std::min(m_startPdf.x, m_currentPdf.x);
            float y0 = std::min(m_startPdf.y, m_currentPdf.y);
            float x1 = std::max(m_startPdf.x, m_currentPdf.x);
            float y1 = std::max(m_startPdf.y, m_currentPdf.y);

            // Minimum 4pt dimension
            if (x1 - x0 < 4.0f) x1 = x0 + 4.0f;
            if (y1 - y0 < 4.0f) y1 = y0 + 4.0f;

            core::interfaces::dom::AnnotationType annType = core::interfaces::dom::AnnotationType::Square;
            if (m_toolMode == ToolMode::Ellipse) annType = core::interfaces::dom::AnnotationType::Circle;
            else if (m_toolMode == ToolMode::Line || m_toolMode == ToolMode::Arrow) annType = core::interfaces::dom::AnnotationType::Line;

            RectF bounds{x0, y0, x1, y1};
            auto cmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
                doc, m_pageIndex, annType, bounds
            );
            auto* rawCmd = cmd.get();
            if (m_context.executeCommand && m_context.executeCommand(std::move(cmd))) {
                auto annot = rawCmd->GetAnnotation();
                if (annot) {
                    annot->SetColor(m_strokeR, m_strokeG, m_strokeB, m_strokeA);
                    if (m_hasFill && (annType == core::interfaces::dom::AnnotationType::Square || annType == core::interfaces::dom::AnnotationType::Circle)) {
                        annot->SetFillColor(m_fillR, m_fillG, m_fillB, m_fillA);
                    }
                    annot->SetBorderWidth(m_borderWidth);

                    if (annType == core::interfaces::dom::AnnotationType::Line) {
                        core::interfaces::dom::LineGeometry lg;
                        lg.start = m_startPdf;
                        lg.end = m_currentPdf;
                        if (m_toolMode == ToolMode::Arrow) {
                            lg.endEnding = core::interfaces::dom::LineEnding::ClosedArrow;
                        }
                        annot->SetLineGeometry(lg);
                    }
                }
                if (m_context.onMutationCommitted) m_context.onMutationCommitted();
                if (m_context.reloadInteractables) m_context.reloadInteractables();
            }
        }
    }

    m_state = InteractionState::Idle;
    m_pageIndex = -1;
    if (m_context.invalidateView) m_context.invalidateView();
    return true;
}

bool ShapeAnnotationHandler::OnKeyDown(int keyCode, bool shift, bool ctrl, bool alt) {
    (void)ctrl; (void)alt;
    m_shiftPressed = shift;
    if (keyCode == VK_ESCAPE) {
        Cancel();
        return true;
    }
    return false;
}

bool ShapeAnnotationHandler::OnKeyUp(int keyCode, bool shift, bool ctrl, bool alt) {
    (void)keyCode; (void)ctrl; (void)alt;
    m_shiftPressed = shift;
    return false;
}

HCURSOR ShapeAnnotationHandler::OnSetCursor(const PointF& viewPoint) {
    (void)viewPoint;
    return LoadCursor(nullptr, IDC_CROSS);
}

void ShapeAnnotationHandler::RenderPreview(ID2D1RenderTarget* target, float zoom, const PointF& scrollOffset) {
    (void)scrollOffset;
    if (!target || m_state != InteractionState::Creating || m_pageIndex < 0 || !m_context.pageToView) return;

    double vx1, vy1, vx2, vy2;
    m_context.pageToView(m_startPdf.x, m_startPdf.y, m_pageIndex, vx1, vy1);
    m_context.pageToView(m_currentPdf.x, m_currentPdf.y, m_pageIndex, vx2, vy2);

    float l = static_cast<float>(std::min(vx1, vx2));
    float r = static_cast<float>(std::max(vx1, vx2));
    float t = static_cast<float>(std::min(vy1, vy2));
    float b = static_cast<float>(std::max(vy1, vy2));

    ID2D1SolidColorBrush* strokeBrush = nullptr;
    ID2D1SolidColorBrush* fillBrush = nullptr;

    float sR = static_cast<float>(m_strokeR) / 255.0f;
    float sG = static_cast<float>(m_strokeG) / 255.0f;
    float sB = static_cast<float>(m_strokeB) / 255.0f;
    float sA = static_cast<float>(m_strokeA) / 255.0f;
    target->CreateSolidColorBrush(D2D1::ColorF(sR, sG, sB, sA), &strokeBrush);

    float fR = static_cast<float>(m_fillR) / 255.0f;
    float fG = static_cast<float>(m_fillG) / 255.0f;
    float fB = static_cast<float>(m_fillB) / 255.0f;
    float fA = m_hasFill ? (static_cast<float>(m_fillA) / 255.0f) : 0.15f;
    target->CreateSolidColorBrush(D2D1::ColorF(fR, fG, fB, fA), &fillBrush);

    float strokeW = std::max(1.0f, m_borderWidth * zoom);

    if (strokeBrush) {
        if (m_toolMode == ToolMode::Rectangle) {
            D2D1_RECT_F rect = D2D1::RectF(l, t, r, b);
            if (fillBrush) target->FillRectangle(rect, fillBrush);
            target->DrawRectangle(rect, strokeBrush, strokeW);
        } else if (m_toolMode == ToolMode::Ellipse) {
            D2D1_ELLIPSE ellipse = D2D1::Ellipse(
                D2D1::Point2F((l + r) / 2.0f, (t + b) / 2.0f),
                (r - l) / 2.0f,
                (b - t) / 2.0f
            );
            if (fillBrush) target->FillEllipse(ellipse, fillBrush);
            target->DrawEllipse(ellipse, strokeBrush, strokeW);
        } else if (m_toolMode == ToolMode::Line || m_toolMode == ToolMode::Arrow) {
            D2D1_POINT_2F p1 = D2D1::Point2F(static_cast<float>(vx1), static_cast<float>(vy1));
            D2D1_POINT_2F p2 = D2D1::Point2F(static_cast<float>(vx2), static_cast<float>(vy2));
            target->DrawLine(p1, p2, strokeBrush, strokeW);

            if (m_toolMode == ToolMode::Arrow) {
                // Draw arrowhead at p2
                float dx = p2.x - p1.x;
                float dy = p2.y - p1.y;
                float length = std::sqrt(dx * dx + dy * dy);
                if (length > 5.0f) {
                    float uX = dx / length;
                    float uY = dy / length;
                    float arrowSize = std::max(8.0f, 12.0f * zoom);

                    D2D1_POINT_2F a1 = D2D1::Point2F(
                        p2.x - arrowSize * uX + (arrowSize * 0.5f) * uY,
                        p2.y - arrowSize * uY - (arrowSize * 0.5f) * uX
                    );
                    D2D1_POINT_2F a2 = D2D1::Point2F(
                        p2.x - arrowSize * uX - (arrowSize * 0.5f) * uY,
                        p2.y - arrowSize * uY + (arrowSize * 0.5f) * uX
                    );

                    target->DrawLine(p2, a1, strokeBrush, strokeW);
                    target->DrawLine(p2, a2, strokeBrush, strokeW);
                }
            }
        }
    }

    if (strokeBrush) strokeBrush->Release();
    if (fillBrush) fillBrush->Release();
}

void ShapeAnnotationHandler::Cancel() {
    if (m_context.hwnd && GetCapture() == m_context.hwnd) {
        ReleaseCapture();
    }
    m_state = InteractionState::Idle;
    m_pageIndex = -1;
    if (m_context.invalidateView) m_context.invalidateView();
}

} // namespace ui::annotation
