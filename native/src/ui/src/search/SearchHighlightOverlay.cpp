#include "search/SearchHighlightOverlay.h"
#include <algorithm>
#include <cmath>

namespace ui {
namespace search {

SearchHighlightOverlay::SearchHighlightOverlay(SearchHighlightStyle style)
    : m_style(style), m_activeIndex(-1), m_lastTarget(nullptr) {
}

void SearchHighlightOverlay::SetResults(const std::vector<SearchResult>& results, int activeIndex) {
    m_results = results;
    if (m_results.empty()) {
        m_activeIndex = -1;
    } else {
        m_activeIndex = std::max(0, std::min(activeIndex, static_cast<int>(m_results.size()) - 1));
    }
}

void SearchHighlightOverlay::SetActiveIndex(int activeIndex) {
    if (m_results.empty()) {
        m_activeIndex = -1;
    } else {
        m_activeIndex = std::max(0, std::min(activeIndex, static_cast<int>(m_results.size()) - 1));
    }
}

int SearchHighlightOverlay::GetActiveIndex() const {
    return m_activeIndex;
}

size_t SearchHighlightOverlay::GetResultCount() const {
    return m_results.size();
}

const SearchResult* SearchHighlightOverlay::GetResult(size_t index) const {
    if (index < m_results.size()) {
        return &m_results[index];
    }
    return nullptr;
}

const SearchResult* SearchHighlightOverlay::GetActiveResult() const {
    if (m_activeIndex >= 0 && static_cast<size_t>(m_activeIndex) < m_results.size()) {
        return &m_results[m_activeIndex];
    }
    return nullptr;
}

const std::vector<SearchResult>& SearchHighlightOverlay::GetAllResults() const {
    return m_results;
}

void SearchHighlightOverlay::Clear() {
    m_results.clear();
    m_activeIndex = -1;
}

const SearchHighlightStyle& SearchHighlightOverlay::GetStyle() const {
    return m_style;
}

void SearchHighlightOverlay::SetStyle(const SearchHighlightStyle& style) {
    m_style = style;
    // Invalidate brushes so they recreate on next render
    m_inactiveBrush.Reset();
    m_activeBrush.Reset();
    m_activeBorderBrush.Reset();
}

void SearchHighlightOverlay::EnsureBrushes(ID2D1RenderTarget* target) {
    if (!target) return;

    if (m_lastTarget != target || !m_inactiveBrush || !m_activeBrush || !m_activeBorderBrush) {
        m_lastTarget = target;
        m_inactiveBrush.Reset();
        m_activeBrush.Reset();
        m_activeBorderBrush.Reset();

        target->CreateSolidColorBrush(m_style.inactiveFillColor, &m_inactiveBrush);
        target->CreateSolidColorBrush(m_style.activeFillColor, &m_activeBrush);
        target->CreateSolidColorBrush(m_style.activeBorderColor, &m_activeBorderBrush);
    }
}

void SearchHighlightOverlay::Render(
    ID2D1RenderTarget* target,
    const D2D1_RECT_F& viewportBounds,
    TextPageProvider textPageProvider,
    PageToViewConverter pageToView
) {
    if (!target || m_results.empty() || !textPageProvider || !pageToView) return;

    EnsureBrushes(target);
    if (!m_inactiveBrush || !m_activeBrush || !m_activeBorderBrush) return;

    for (int i = 0; i < static_cast<int>(m_results.size()); ++i) {
        const auto& res = m_results[i];
        if (res.charCount <= 0) continue;

        auto* tp = textPageProvider(res.pageIndex);
        if (!tp) continue;

        auto rects = tp->GetRects(res.charIndex, res.charCount);
        bool isActive = (i == m_activeIndex);

        for (const auto& r : rects) {
            double vx1 = 0, vy1 = 0, vx2 = 0, vy2 = 0;
            pageToView(r.left, r.bottom, res.pageIndex, vx1, vy1);
            pageToView(r.right, r.top, res.pageIndex, vx2, vy2);

            float left = static_cast<float>(std::min(vx1, vx2));
            float right = static_cast<float>(std::max(vx1, vx2));
            float top = static_cast<float>(std::min(vy1, vy2));
            float bottom = static_cast<float>(std::max(vy1, vy2));

            // Viewport culling
            if (bottom < viewportBounds.top || top > viewportBounds.bottom) {
                continue;
            }

            D2D1_RECT_F drawRect = D2D1::RectF(left, top, right, bottom);
            if (isActive) {
                target->FillRectangle(drawRect, m_activeBrush.Get());
                target->DrawRectangle(drawRect, m_activeBorderBrush.Get(), m_style.activeBorderWidth);
            } else {
                target->FillRectangle(drawRect, m_inactiveBrush.Get());
            }
        }
    }
}

AutoScrollResult SearchHighlightOverlay::CalculateAutoScroll(
    float viewportHeight,
    float currentScrollY,
    TextPageProvider textPageProvider,
    PageToViewConverter pageToView
) const {
    AutoScrollResult result;
    if (m_activeIndex < 0 || static_cast<size_t>(m_activeIndex) >= m_results.size()) {
        return result;
    }

    if (!textPageProvider || !pageToView || viewportHeight <= 0.0f) {
        return result;
    }

    const auto& activeRes = m_results[m_activeIndex];
    if (activeRes.charCount <= 0) return result;

    auto* tp = textPageProvider(activeRes.pageIndex);
    if (!tp) return result;

    auto rects = tp->GetRects(activeRes.charIndex, activeRes.charCount);
    if (rects.empty()) return result;

    double vx1 = 0, vy1 = 0, vx2 = 0, vy2 = 0;
    pageToView(rects[0].left, rects[0].bottom, activeRes.pageIndex, vx1, vy1);
    pageToView(rects[0].right, rects[0].top, activeRes.pageIndex, vx2, vy2);

    float top = static_cast<float>(std::min(vy1, vy2));
    float bottom = static_cast<float>(std::max(vy1, vy2));
    float matchHeight = std::max(1.0f, bottom - top);

    float visibleTop = currentScrollY + m_style.scrollMargin;
    float visibleBottom = currentScrollY + viewportHeight - m_style.scrollMargin;

    if (top < visibleTop || bottom > visibleBottom) {
        result.shouldScroll = true;
        result.newScrollY = std::max(0.0f, top - (viewportHeight - matchHeight) / 2.0f);
    }

    return result;
}

} // namespace search
} // namespace ui
