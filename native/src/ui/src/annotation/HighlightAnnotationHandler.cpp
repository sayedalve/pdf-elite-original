#include "HighlightAnnotationHandler.h"
#include "../../pdf_engine/src/commands/AnnotationCommands.h"
#include <algorithm>

namespace ui::annotation {

HighlightAnnotationHandler::HighlightAnnotationHandler(ToolMode toolMode)
    : m_toolMode(toolMode) {
    if (m_toolMode == ToolMode::Underline) {
        m_r = 0; m_g = 120; m_b = 215; m_a = 255;
    } else if (m_toolMode == ToolMode::Strikeout) {
        m_r = 230; m_g = 0; m_b = 0; m_a = 255;
    }
}

void HighlightAnnotationHandler::Initialize(const AnnotationHandlerContext& context) {
    m_context = context;
    m_state = InteractionState::Idle;
}

bool HighlightAnnotationHandler::OnMouseDown(const MouseEvent& event) {
    if (event.pageIndex < 0) return false;
    m_state = InteractionState::Creating;
    m_startPage = event.pageIndex;
    m_endPage = event.pageIndex;
    m_startChar = -1;
    m_endChar = -1;

    if (m_context.getTextPage) {
        auto* tp = m_context.getTextPage(event.pageIndex);
        if (tp) {
            m_startChar = tp->GetCharIndexAtPos(static_cast<float>(event.pdfX), static_cast<float>(event.pdfY), 5.0f, 5.0f);
            m_endChar = m_startChar;
        }
    }

    if (m_context.hwnd) SetCapture(m_context.hwnd);
    if (m_context.invalidateView) m_context.invalidateView();
    return true;
}

bool HighlightAnnotationHandler::OnMouseMove(const MouseEvent& event) {
    if (m_state != InteractionState::Creating) return false;

    if (event.pageIndex >= 0 && m_context.getTextPage) {
        auto* tp = m_context.getTextPage(event.pageIndex);
        if (tp) {
            int charIndex = tp->GetCharIndexAtPos(static_cast<float>(event.pdfX), static_cast<float>(event.pdfY), 5.0f, 5.0f);
            if (charIndex >= 0) {
                m_endPage = event.pageIndex;
                m_endChar = charIndex;
                if (m_startChar < 0) m_startChar = charIndex;
                if (m_context.invalidateView) m_context.invalidateView();
            }
        }
    }
    return true;
}

bool HighlightAnnotationHandler::OnMouseUp(const MouseEvent& event) {
    (void)event;
    if (m_state != InteractionState::Creating) return false;
    if (m_context.hwnd && GetCapture() == m_context.hwnd) {
        ReleaseCapture();
    }

    if (m_startPage >= 0 && m_endPage == m_startPage && m_startChar >= 0 && m_endChar >= 0 && m_context.getTextPage && m_context.getDocument) {
        int start = std::min(m_startChar, m_endChar);
        int end = std::max(m_startChar, m_endChar);
        int count = end - start + 1;

        auto* tp = m_context.getTextPage(m_startPage);
        auto* doc = m_context.getDocument();
        if (tp && doc && count > 0) {
            auto rects = tp->GetRects(start, count);
            if (!rects.empty()) {
                core::interfaces::dom::AnnotationType type = core::interfaces::dom::AnnotationType::Highlight;
                if (m_toolMode == ToolMode::Underline) type = core::interfaces::dom::AnnotationType::Underline;
                else if (m_toolMode == ToolMode::Strikeout) type = core::interfaces::dom::AnnotationType::StrikeOut;

                float minX = rects[0].left, maxX = rects[0].right;
                float minY = std::min(rects[0].bottom, rects[0].top);
                float maxY = std::max(rects[0].bottom, rects[0].top);

                std::vector<QuadF> quads;
                quads.reserve(rects.size());

                for (const auto& r : rects) {
                    minX = std::min({minX, r.left, r.right});
                    maxX = std::max({maxX, r.left, r.right});
                    minY = std::min({minY, r.bottom, r.top});
                    maxY = std::max({maxY, r.bottom, r.top});

                    // In PDF points: p1=TL, p2=TR, p3=BL, p4=BR
                    float l = std::min(r.left, r.right);
                    float right = std::max(r.left, r.right);
                    float b = std::min(r.bottom, r.top);
                    float t = std::max(r.bottom, r.top);

                    QuadF quad;
                    quad.p1 = {l, t};
                    quad.p2 = {right, t};
                    quad.p3 = {l, b};
                    quad.p4 = {right, b};
                    quads.push_back(quad);
                }

                RectF boundingBox{minX, minY, maxX, maxY};
                auto cmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
                    doc, m_startPage, type, boundingBox
                );
                auto* rawCmd = cmd.get();
                if (m_context.executeCommand && m_context.executeCommand(std::move(cmd))) {
                    auto annot = rawCmd->GetAnnotation();
                    if (annot) {
                        annot->SetQuadPoints(quads);
                        annot->SetColor(m_r, m_g, m_b, m_a);
                        annot->SetOpacity(static_cast<float>(m_a) / 255.0f);
                    }
                    if (m_context.onMutationCommitted) m_context.onMutationCommitted();
                    if (m_context.reloadInteractables) m_context.reloadInteractables();
                }
            }
        }
    }

    m_state = InteractionState::Idle;
    m_startPage = -1;
    m_startChar = -1;
    m_endPage = -1;
    m_endChar = -1;
    if (m_context.invalidateView) m_context.invalidateView();
    return true;
}

bool HighlightAnnotationHandler::OnKeyDown(int keyCode, bool shift, bool ctrl, bool alt) {
    (void)shift; (void)ctrl; (void)alt;
    if (keyCode == VK_ESCAPE) {
        Cancel();
        return true;
    }
    return false;
}

bool HighlightAnnotationHandler::OnKeyUp(int keyCode, bool shift, bool ctrl, bool alt) {
    (void)keyCode; (void)shift; (void)ctrl; (void)alt;
    return false;
}

HCURSOR HighlightAnnotationHandler::OnSetCursor(const PointF& viewPoint) {
    (void)viewPoint;
    return LoadCursor(nullptr, IDC_IBEAM);
}

void HighlightAnnotationHandler::RenderPreview(ID2D1RenderTarget* target, float zoom, const PointF& scrollOffset) {
    (void)zoom; (void)scrollOffset;
    if (!target || m_state != InteractionState::Creating) return;
    if (m_startPage < 0 || m_startChar < 0 || m_endChar < 0 || !m_context.getTextPage || !m_context.pageToView) return;

    int start = std::min(m_startChar, m_endChar);
    int end = std::max(m_startChar, m_endChar);
    int count = end - start + 1;

    auto* tp = m_context.getTextPage(m_startPage);
    if (!tp || count <= 0) return;

    auto rects = tp->GetRects(start, count);
    if (rects.empty()) return;

    ID2D1SolidColorBrush* brush = nullptr;
    float r = static_cast<float>(m_r) / 255.0f;
    float g = static_cast<float>(m_g) / 255.0f;
    float b = static_cast<float>(m_b) / 255.0f;
    float a = (m_toolMode == ToolMode::Highlight) ? 0.4f : 0.8f;
    target->CreateSolidColorBrush(D2D1::ColorF(r, g, b, a), &brush);

    if (brush) {
        for (const auto& rect : rects) {
            double vx1, vy1, vx2, vy2;
            m_context.pageToView(rect.left, rect.bottom, m_startPage, vx1, vy1);
            m_context.pageToView(rect.right, rect.top, m_startPage, vx2, vy2);

            float left = static_cast<float>(std::min(vx1, vx2));
            float right = static_cast<float>(std::max(vx1, vx2));
            float top = static_cast<float>(std::min(vy1, vy2));
            float bottom = static_cast<float>(std::max(vy1, vy2));

            if (m_toolMode == ToolMode::Highlight) {
                target->FillRectangle(D2D1::RectF(left, top, right, bottom), brush);
            } else if (m_toolMode == ToolMode::Underline) {
                target->DrawLine(D2D1::Point2F(left, bottom - 1.0f), D2D1::Point2F(right, bottom - 1.0f), brush, 2.0f);
            } else if (m_toolMode == ToolMode::Strikeout) {
                float midY = (top + bottom) / 2.0f;
                target->DrawLine(D2D1::Point2F(left, midY), D2D1::Point2F(right, midY), brush, 2.0f);
            }
        }
        brush->Release();
    }
}

void HighlightAnnotationHandler::Cancel() {
    if (m_context.hwnd && GetCapture() == m_context.hwnd) {
        ReleaseCapture();
    }
    m_state = InteractionState::Idle;
    m_startPage = -1;
    m_startChar = -1;
    m_endPage = -1;
    m_endChar = -1;
    if (m_context.invalidateView) m_context.invalidateView();
}

} // namespace ui::annotation
