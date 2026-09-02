#include "../../include/tools/ToolProperties.h"
#include "../../include/selection/CursorResolver.h"
#include "../../include/tools/MarkupTool.h"
#include "../../../pdf_engine/src/commands/AnnotationCommands.h"
#include "../../../core/interfaces/dom/ITextPage.h"
#include <algorithm>
#include <wrl/client.h>

namespace ui::tools {

MarkupTool::MarkupTool(ToolType type) : m_type(type) {}
MarkupTool::~MarkupTool() = default;

std::wstring MarkupTool::GetName() const {
    switch (m_type) {
    case ToolType::Highlight: return L"Highlight";
    case ToolType::Underline: return L"Underline";
    case ToolType::Strikeout: return L"Strikeout";
    
    
    default: return L"Markup";
    }
}

void MarkupTool::OnActivate(ToolContext& context) {
    (void)context;
    m_state = ToolState::Idle;
    m_pageIndex = -1;
}

void MarkupTool::OnDeactivate(ToolContext& context) {
    Cancel(context);
}

void MarkupTool::Cancel(ToolContext& context) {
    if (m_state == ToolState::Dragging && context.captureService) {
        context.captureService->ReleaseCapture(this);
    }
    m_state = ToolState::Idle;
    m_pageIndex = -1;
    m_startChar = -1;
    m_endChar = -1;
    if (context.invalidateView) {
        context.invalidateView();
    }
}

input::EventResult MarkupTool::OnPointerDown(const input::PointerEvent& event, ToolContext& context) {
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
        m_startChar = -1;
        m_endChar = -1;

        if (context.getTextPage) {
            auto* tp = context.getTextPage(m_pageIndex);
            if (tp) {
                m_startChar = tp->GetCharIndexAtPos(pdfPt.x, pdfPt.y, 5.0, 5.0);
                m_endChar = m_startChar;
            }
        }

        m_state = ToolState::Dragging;
        if (context.captureService && context.hwnd) {
            context.captureService->AcquireCapture(context.hwnd, this);
        }
        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }
    return input::EventResult::Ignored;
}

input::EventResult MarkupTool::OnPointerMove(const input::PointerEvent& event, ToolContext& context) {
    if (m_state == ToolState::Dragging) {
        PointF pdfPt = event.pagePoint;
        if (event.pageIndex != m_pageIndex && context.canvasToPdf) {
            int dummy = -1;
            pdfPt = context.canvasToPdf(event.canvasPoint, dummy);
        }
        m_currentPdf = pdfPt;

        if (context.getTextPage && m_pageIndex >= 0) {
            auto* tp = context.getTextPage(m_pageIndex);
            if (tp) {
                int charIdx = tp->GetCharIndexAtPos(pdfPt.x, pdfPt.y, 5.0, 5.0);
                if (charIdx >= 0) {
                    m_endChar = charIdx;
                    if (m_startChar < 0) {
                        m_startChar = charIdx;
                    }
                }
            }
        }

        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }
    m_state = ToolState::Hovering;
    return input::EventResult::Ignored;
}

input::EventResult MarkupTool::OnPointerUp(const input::PointerEvent& event, ToolContext& context) {
    if (m_state == ToolState::Dragging) {
        if (context.captureService) {
            context.captureService->ReleaseCapture(this);
        }

        if (m_pageIndex >= 0 && context.document) {
            core::interfaces::dom::AnnotationType annType = core::interfaces::dom::AnnotationType::Highlight;
            if (m_type == ToolType::Underline) annType = core::interfaces::dom::AnnotationType::Underline;
            else if (m_type == ToolType::Strikeout) annType = core::interfaces::dom::AnnotationType::StrikeOut;
            
            

            float x0 = (std::min)(m_startPdf.x, m_currentPdf.x);
            float y0 = (std::min)(m_startPdf.y, m_currentPdf.y);
            float x1 = (std::max)(m_startPdf.x, m_currentPdf.x);
            float y1 = (std::max)(m_startPdf.y, m_currentPdf.y);

            std::vector<QuadF> quads;
            if (m_startChar >= 0 && m_endChar >= 0 && context.getTextPage) {
                if (auto* tp = context.getTextPage(m_pageIndex)) {
                    int start = std::min(m_startChar, m_endChar);
                    int count = std::abs(m_endChar - m_startChar) + 1;
                    std::vector<RectF> rects = tp->GetRects(start, count);
                    
                    if (!rects.empty()) {
                        x0 = rects[0].left; y0 = rects[0].top; x1 = rects[0].right; y1 = rects[0].bottom;
                        for (const auto& r : rects) {
                            x0 = std::min(x0, r.left);
                            y0 = std::min(y0, r.bottom);
                            x1 = std::max(x1, r.right);
                            y1 = std::max(y1, r.top);
                            quads.push_back({
                                PointF{r.left, r.top},
                                PointF{r.right, r.top},
                                PointF{r.left, r.bottom},
                                PointF{r.right, r.bottom}
                            });
                        }
                    }
                }
            }

            if (quads.empty()) { m_state = ToolState::Idle; m_pageIndex = -1; if (context.invalidateView) context.invalidateView(); return input::EventResult::Handled; }

            if (x1 - x0 < 4.0f) x1 = x0 + 4.0f;
            if (y1 - y0 < 4.0f) y1 = y0 + 4.0f;

            auto cmd = std::make_unique<pdf_engine::commands::AddAnnotationCommand>(
                context.document, m_pageIndex, annType, RectF{ x0, y0, x1, y1 }
            );

            if (!quads.empty()) {
                cmd->SetQuads(quads);
            }

            if (m_type == ToolType::Highlight) {
                cmd->SetColor(static_cast<int>(ui::tools::ToolProperties::highlightRed * 255), static_cast<int>(ui::tools::ToolProperties::highlightGreen * 255), static_cast<int>(ui::tools::ToolProperties::highlightBlue * 255), static_cast<int>(ui::tools::ToolProperties::highlightAlpha * 255));
            }

            if (context.executeCommand) {
                context.executeCommand(std::move(cmd));
            } else {
                context.document->GetCommandStack().ExecuteCommand(std::move(cmd));
            }
        }

        m_state = ToolState::Committed;
        m_state = ToolState::Idle;
        m_pageIndex = -1;
        m_startChar = -1;
        m_endChar = -1;

        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }
    (void)event;
    return input::EventResult::Ignored;
}

HCURSOR MarkupTool::GetCursor(const PointF& point, ToolContext& context) const {
    (void)point;
    (void)context;
    
    return ui::selection::CursorResolver::ResolveToolCursor(m_type, m_state);
}

void MarkupTool::RenderOverlay(ID2D1RenderTarget* renderTarget, ToolContext& context) {
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
    D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0.0f, 0.4f); // Highlight
    if (m_type == ToolType::Underline) color = D2D1::ColorF(0.0f, 0.8f, 0.0f, 0.6f);
    else if (m_type == ToolType::Strikeout) color = D2D1::ColorF(1.0f, 0.0f, 0.0f, 0.6f);
    
    

    if (SUCCEEDED(renderTarget->CreateSolidColorBrush(color, &brush)) && brush) {
        if (m_startChar >= 0 && m_endChar >= 0 && context.getTextPage) {
            if (auto* tp = context.getTextPage(m_pageIndex)) {
                int start = std::min(m_startChar, m_endChar);
                int count = std::abs(m_endChar - m_startChar) + 1;
                std::vector<RectF> rects = tp->GetRects(start, count);
                for (const auto& r : rects) {
                    PointF pt1 = context.pdfToCanvas(m_pageIndex, PointF{r.left, r.top});
                    PointF pt2 = context.pdfToCanvas(m_pageIndex, PointF{r.right, r.bottom});
                    if (context.canvasToDip) {
                        pt1 = context.canvasToDip(pt1);
                        pt2 = context.canvasToDip(pt2);
                    }
                    float l = (std::min)(pt1.x, pt2.x);
                    float right = (std::max)(pt1.x, pt2.x);
                    float t = (std::min)(pt1.y, pt2.y);
                    float b = (std::max)(pt1.y, pt2.y);
                    
                    if (m_type == ToolType::Highlight) {
                        renderTarget->FillRectangle(D2D1::RectF(l, t, right, b), brush.Get());
                    } else if (m_type == ToolType::Underline) {
                        renderTarget->DrawLine(D2D1::Point2F(l, b - 1.0f), D2D1::Point2F(right, b - 1.0f), brush.Get(), 2.0f);
                    } else if (m_type == ToolType::Strikeout) {
                        float midY = (t + b) / 2.0f;
                        renderTarget->DrawLine(D2D1::Point2F(l, midY), D2D1::Point2F(right, midY), brush.Get(), 2.0f);
                    } else {
                        renderTarget->FillRectangle(D2D1::RectF(l, t, right, b), brush.Get());
                    }
                }
            }
        } else {
            float left = (std::min)(c0.x, c1.x);
            float right = (std::max)(c0.x, c1.x);
            float top = (std::min)(c0.y, c1.y);
            float bottom = (std::max)(c0.y, c1.y);
            renderTarget->FillRectangle(D2D1::RectF(left, top, right, bottom), brush.Get());
        }
    }
}

} // namespace ui::tools










