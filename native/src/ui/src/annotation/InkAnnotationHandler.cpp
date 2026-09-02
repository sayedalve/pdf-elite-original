#include "InkAnnotationHandler.h"
#include "../../pdf_engine/src/commands/AnnotationCommands.h"
#include <algorithm>
#include <cmath>

namespace ui::annotation {

InkAnnotationHandler::InkAnnotationHandler() = default;

void InkAnnotationHandler::Initialize(const AnnotationHandlerContext& context) {
    m_context = context;
    m_state = InteractionState::Idle;
}

bool InkAnnotationHandler::OnMouseDown(const MouseEvent& event) {
    if (event.pageIndex < 0) return false;
    m_state = InteractionState::Creating;
    m_pageIndex = event.pageIndex;
    m_currentStroke.clear();
    m_currentStroke.push_back({static_cast<float>(event.pdfX), static_cast<float>(event.pdfY)});

    if (m_context.hwnd) SetCapture(m_context.hwnd);
    if (m_context.invalidateView) m_context.invalidateView();
    return true;
}

bool InkAnnotationHandler::OnMouseMove(const MouseEvent& event) {
    if (m_state != InteractionState::Creating) return false;
    if (event.pageIndex == m_pageIndex) {
        PointF newPt{static_cast<float>(event.pdfX), static_cast<float>(event.pdfY)};
        if (!m_currentStroke.empty()) {
            const auto& lastPt = m_currentStroke.back();
            float dx = newPt.x - lastPt.x;
            float dy = newPt.y - lastPt.y;
            // Only add if moved at least 0.5pt to reduce point bloat
            if (dx * dx + dy * dy >= 0.25f) {
                m_currentStroke.push_back(newPt);
                if (m_context.invalidateView) m_context.invalidateView();
            }
        } else {
            m_currentStroke.push_back(newPt);
            if (m_context.invalidateView) m_context.invalidateView();
        }
    }
    return true;
}

bool InkAnnotationHandler::OnMouseUp(const MouseEvent& event) {
    (void)event;
    if (m_state != InteractionState::Creating) return false;
    if (m_context.hwnd && GetCapture() == m_context.hwnd) {
        ReleaseCapture();
    }

    if (m_pageIndex >= 0 && m_currentStroke.size() >= 2 && m_context.getDocument) {
        auto* doc = m_context.getDocument();
        if (doc) {
            float minX = m_currentStroke[0].x, maxX = m_currentStroke[0].x;
            float minY = m_currentStroke[0].y, maxY = m_currentStroke[0].y;
            for (const auto& pt : m_currentStroke) {
                minX = std::min(minX, pt.x);
                maxX = std::max(maxX, pt.x);
                minY = std::min(minY, pt.y);
                maxY = std::max(maxY, pt.y);
            }

            // Pad bounds by stroke width
            float pad = m_strokeWidth / 2.0f;
            RectF bounds{minX - pad, minY - pad, maxX + pad, maxY + pad};

            std::vector<std::vector<PointF>> strokes;
            strokes.push_back(m_currentStroke);

            auto cmd = std::make_unique<pdf_engine::commands::AddInkAnnotationCommand>(
                doc, m_pageIndex, strokes, bounds, m_r, m_g, m_b, m_a, m_strokeWidth
            );
            if (m_context.executeCommand && m_context.executeCommand(std::move(cmd))) {
                if (m_context.onMutationCommitted) m_context.onMutationCommitted();
                if (m_context.reloadInteractables) m_context.reloadInteractables();
            }
        }
    }

    m_state = InteractionState::Idle;
    m_pageIndex = -1;
    m_currentStroke.clear();
    if (m_context.invalidateView) m_context.invalidateView();
    return true;
}

bool InkAnnotationHandler::OnKeyDown(int keyCode, bool shift, bool ctrl, bool alt) {
    (void)shift; (void)ctrl; (void)alt;
    if (keyCode == VK_ESCAPE) {
        Cancel();
        return true;
    }
    return false;
}

bool InkAnnotationHandler::OnKeyUp(int keyCode, bool shift, bool ctrl, bool alt) {
    (void)keyCode; (void)shift; (void)ctrl; (void)alt;
    return false;
}

HCURSOR InkAnnotationHandler::OnSetCursor(const PointF& viewPoint) {
    (void)viewPoint;
    return LoadCursor(nullptr, IDC_CROSS);
}

void InkAnnotationHandler::RenderPreview(ID2D1RenderTarget* target, float zoom, const PointF& scrollOffset) {
    (void)scrollOffset;
    if (!target || m_state != InteractionState::Creating || m_currentStroke.size() < 2 || m_pageIndex < 0 || !m_context.pageToView) return;

    ID2D1SolidColorBrush* brush = nullptr;
    float r = static_cast<float>(m_r) / 255.0f;
    float g = static_cast<float>(m_g) / 255.0f;
    float b = static_cast<float>(m_b) / 255.0f;
    float a = static_cast<float>(m_a) / 255.0f;
    target->CreateSolidColorBrush(D2D1::ColorF(r, g, b, a), &brush);

    if (brush) {
        float strokeWidthInView = std::max(1.0f, m_strokeWidth * zoom);
        for (size_t i = 0; i + 1 < m_currentStroke.size(); ++i) {
            double vx1, vy1, vx2, vy2;
            m_context.pageToView(m_currentStroke[i].x, m_currentStroke[i].y, m_pageIndex, vx1, vy1);
            m_context.pageToView(m_currentStroke[i + 1].x, m_currentStroke[i + 1].y, m_pageIndex, vx2, vy2);

            D2D1_POINT_2F p1 = D2D1::Point2F(static_cast<float>(vx1), static_cast<float>(vy1));
            D2D1_POINT_2F p2 = D2D1::Point2F(static_cast<float>(vx2), static_cast<float>(vy2));
            target->DrawLine(p1, p2, brush, strokeWidthInView);
        }
        brush->Release();
    }
}

void InkAnnotationHandler::Cancel() {
    if (m_context.hwnd && GetCapture() == m_context.hwnd) {
        ReleaseCapture();
    }
    m_state = InteractionState::Idle;
    m_pageIndex = -1;
    m_currentStroke.clear();
    if (m_context.invalidateView) m_context.invalidateView();
}

} // namespace ui::annotation
