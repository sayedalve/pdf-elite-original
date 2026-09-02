#include "TextAnnotationHandler.h"
#include "../../pdf_engine/src/commands/AnnotationCommands.h"
#include <algorithm>

namespace ui::annotation {

TextAnnotationHandler::TextAnnotationHandler() = default;

void TextAnnotationHandler::Initialize(const AnnotationHandlerContext& context) {
    m_context = context;
    m_state = InteractionState::Idle;
}

bool TextAnnotationHandler::OnMouseDown(const MouseEvent& event) {
    if (event.pageIndex < 0) return false;
    m_state = InteractionState::Creating;
    m_pendingPageIndex = event.pageIndex;
    m_pendingPdfPos = {static_cast<float>(event.pdfX), static_cast<float>(event.pdfY)};
    if (m_context.hwnd) SetCapture(m_context.hwnd);
    if (m_context.invalidateView) m_context.invalidateView();
    return true;
}

bool TextAnnotationHandler::OnMouseMove(const MouseEvent& event) {
    m_hasHover = (event.pageIndex >= 0);
    m_hoverPageIndex = event.pageIndex;
    m_currentHoverPdfPos = {static_cast<float>(event.pdfX), static_cast<float>(event.pdfY)};
    if (m_context.invalidateView) m_context.invalidateView();
    return true;
}

bool TextAnnotationHandler::OnMouseUp(const MouseEvent& event) {
    if (m_state != InteractionState::Creating) return false;
    if (m_context.hwnd && GetCapture() == m_context.hwnd) {
        ReleaseCapture();
    }

    int pageIndex = (event.pageIndex >= 0) ? event.pageIndex : m_pendingPageIndex;
    float posX = static_cast<float>(event.pdfX);
    float posY = static_cast<float>(event.pdfY);
    if (posX == 0.0f && posY == 0.0f) {
        posX = m_pendingPdfPos.x;
        posY = m_pendingPdfPos.y;
    }

    if (pageIndex >= 0 && m_context.getDocument) {
        auto* doc = m_context.getDocument();
        if (doc) {
            RectF bounds{posX, posY, posX + m_noteWidth, posY + m_noteHeight};
            auto cmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
                doc, pageIndex, core::interfaces::dom::AnnotationType::Text, bounds
            );
            auto* rawCmd = cmd.get();
            if (m_context.executeCommand && m_context.executeCommand(std::move(cmd))) {
                auto annot = rawCmd->GetAnnotation();
                if (annot) {
                    annot->SetContents(m_defaultContents);
                    annot->SetAuthor(m_defaultAuthor);
                    annot->SetColor(255, 215, 0, 255); // Standard Gold/Yellow sticky note
                }
                if (m_context.onMutationCommitted) m_context.onMutationCommitted();
                if (m_context.reloadInteractables) m_context.reloadInteractables();
            }
        }
    }

    m_state = InteractionState::Idle;
    m_pendingPageIndex = -1;
    if (m_context.invalidateView) m_context.invalidateView();
    return true;
}

bool TextAnnotationHandler::OnKeyDown(int keyCode, bool shift, bool ctrl, bool alt) {
    (void)shift; (void)ctrl; (void)alt;
    if (keyCode == VK_ESCAPE) {
        Cancel();
        return true;
    }
    return false;
}

bool TextAnnotationHandler::OnKeyUp(int keyCode, bool shift, bool ctrl, bool alt) {
    (void)keyCode; (void)shift; (void)ctrl; (void)alt;
    return false;
}

HCURSOR TextAnnotationHandler::OnSetCursor(const PointF& viewPoint) {
    (void)viewPoint;
    return LoadCursor(nullptr, IDC_CROSS);
}

void TextAnnotationHandler::RenderPreview(ID2D1RenderTarget* target, float zoom, const PointF& scrollOffset) {
    (void)zoom; (void)scrollOffset;
    if (!target) return;

    PointF renderPos;
    int pageIndex = -1;
    if (m_state == InteractionState::Creating && m_pendingPageIndex >= 0) {
        renderPos = m_pendingPdfPos;
        pageIndex = m_pendingPageIndex;
    } else if (m_hasHover && m_hoverPageIndex >= 0) {
        renderPos = m_currentHoverPdfPos;
        pageIndex = m_hoverPageIndex;
    } else {
        return;
    }

    if (m_context.pageToView && pageIndex >= 0) {
        double vx1, vy1, vx2, vy2;
        m_context.pageToView(renderPos.x, renderPos.y, pageIndex, vx1, vy1);
        m_context.pageToView(renderPos.x + m_noteWidth, renderPos.y + m_noteHeight, pageIndex, vx2, vy2);

        float l = static_cast<float>(std::min(vx1, vx2));
        float r = static_cast<float>(std::max(vx1, vx2));
        float t = static_cast<float>(std::min(vy1, vy2));
        float b = static_cast<float>(std::max(vy1, vy2));

        D2D1_RECT_F rect = D2D1::RectF(l, t, r, b);

        ID2D1SolidColorBrush* fillBrush = nullptr;
        ID2D1SolidColorBrush* strokeBrush = nullptr;
        target->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.84f, 0.0f, 0.6f), &fillBrush);
        target->CreateSolidColorBrush(D2D1::ColorF(0.8f, 0.6f, 0.0f, 1.0f), &strokeBrush);

        if (fillBrush && strokeBrush) {
            D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(rect, 3.0f, 3.0f);
            target->FillRoundedRectangle(roundedRect, fillBrush);
            target->DrawRoundedRectangle(roundedRect, strokeBrush, 1.5f);
        }

        if (fillBrush) fillBrush->Release();
        if (strokeBrush) strokeBrush->Release();
    }
}

void TextAnnotationHandler::Cancel() {
    if (m_context.hwnd && GetCapture() == m_context.hwnd) {
        ReleaseCapture();
    }
    m_state = InteractionState::Idle;
    m_pendingPageIndex = -1;
    if (m_context.invalidateView) m_context.invalidateView();
}

} // namespace ui::annotation
