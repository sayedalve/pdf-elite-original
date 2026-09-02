#include "FreeTextAnnotationHandler.h"
#include "../../pdf_engine/src/commands/AnnotationCommands.h"
#include <algorithm>

namespace ui::annotation {

FreeTextAnnotationHandler::FreeTextAnnotationHandler() = default;

void FreeTextAnnotationHandler::Initialize(const AnnotationHandlerContext& context) {
    m_context = context;
    m_state = InteractionState::Idle;
}

bool FreeTextAnnotationHandler::OnMouseDown(const MouseEvent& event) {
    if (event.pageIndex < 0) return false;
    m_state = InteractionState::Creating;
    m_pageIndex = event.pageIndex;
    m_startPdf = {static_cast<float>(event.pdfX), static_cast<float>(event.pdfY)};
    m_currentPdf = m_startPdf;

    if (m_context.hwnd) SetCapture(m_context.hwnd);
    if (m_context.invalidateView) m_context.invalidateView();
    return true;
}

bool FreeTextAnnotationHandler::OnMouseMove(const MouseEvent& event) {
    if (m_state != InteractionState::Creating) return false;
    m_currentPdf = {static_cast<float>(event.pdfX), static_cast<float>(event.pdfY)};
    if (m_context.invalidateView) m_context.invalidateView();
    return true;
}

bool FreeTextAnnotationHandler::OnMouseUp(const MouseEvent& event) {
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

            // If user clicked without dragging, use default text box size
            if (x1 - x0 < 10.0f) x1 = x0 + m_defaultWidth;
            if (y1 - y0 < 10.0f) y1 = y0 + m_defaultHeight;

            RectF bounds{x0, y0, x1, y1};
            auto cmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
                doc, m_pageIndex, core::interfaces::dom::AnnotationType::FreeText, bounds
            );
            auto* rawCmd = cmd.get();
            if (m_context.executeCommand && m_context.executeCommand(std::move(cmd))) {
                auto annot = rawCmd->GetAnnotation();
                if (annot) {
                    annot->SetContents(m_defaultText);
                    annot->SetColor(m_textR, m_textG, m_textB, m_textA);
                    annot->SetBorderWidth(1.0f);
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

bool FreeTextAnnotationHandler::OnKeyDown(int keyCode, bool shift, bool ctrl, bool alt) {
    (void)shift; (void)ctrl; (void)alt;
    if (keyCode == VK_ESCAPE) {
        Cancel();
        return true;
    }
    return false;
}

bool FreeTextAnnotationHandler::OnKeyUp(int keyCode, bool shift, bool ctrl, bool alt) {
    (void)keyCode; (void)shift; (void)ctrl; (void)alt;
    return false;
}

HCURSOR FreeTextAnnotationHandler::OnSetCursor(const PointF& viewPoint) {
    (void)viewPoint;
    return LoadCursor(nullptr, IDC_IBEAM);
}

void FreeTextAnnotationHandler::RenderPreview(ID2D1RenderTarget* target, float zoom, const PointF& scrollOffset) {
    (void)zoom; (void)scrollOffset;
    if (!target || m_state != InteractionState::Creating || m_pageIndex < 0 || !m_context.pageToView) return;

    float x0 = std::min(m_startPdf.x, m_currentPdf.x);
    float y0 = std::min(m_startPdf.y, m_currentPdf.y);
    float x1 = std::max(m_startPdf.x, m_currentPdf.x);
    float y1 = std::max(m_startPdf.y, m_currentPdf.y);

    if (x1 - x0 < 10.0f) x1 = x0 + m_defaultWidth;
    if (y1 - y0 < 10.0f) y1 = y0 + m_defaultHeight;

    double vx1, vy1, vx2, vy2;
    m_context.pageToView(x0, y0, m_pageIndex, vx1, vy1);
    m_context.pageToView(x1, y1, m_pageIndex, vx2, vy2);

    float l = static_cast<float>(std::min(vx1, vx2));
    float r = static_cast<float>(std::max(vx1, vx2));
    float t = static_cast<float>(std::min(vy1, vy2));
    float b = static_cast<float>(std::max(vy1, vy2));

    D2D1_RECT_F rect = D2D1::RectF(l, t, r, b);

    ID2D1SolidColorBrush* strokeBrush = nullptr;
    ID2D1SolidColorBrush* fillBrush = nullptr;
    target->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.45f, 0.85f, 0.9f), &strokeBrush);
    target->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.7f), &fillBrush);

    if (strokeBrush && fillBrush) {
        target->FillRectangle(rect, fillBrush);
        target->DrawRectangle(rect, strokeBrush, 1.5f);
    }

    if (strokeBrush) strokeBrush->Release();
    if (fillBrush) fillBrush->Release();
}

void FreeTextAnnotationHandler::Cancel() {
    if (m_context.hwnd && GetCapture() == m_context.hwnd) {
        ReleaseCapture();
    }
    m_state = InteractionState::Idle;
    m_pageIndex = -1;
    if (m_context.invalidateView) m_context.invalidateView();
}

} // namespace ui::annotation
