#pragma once

#include <d2d1.h>
#include <wrl/client.h>
#include <vector>
#include <functional>
#include <memory>
#include "core/models/SearchResult.h"
#include "core/interfaces/dom/ITextPage.h"

namespace ui {
namespace search {

using Microsoft::WRL::ComPtr;
using SearchResult = core::models::SearchResult;

struct SearchHighlightStyle {
    D2D1_COLOR_F inactiveFillColor = D2D1::ColorF(1.0f, 1.0f, 0.0f, 0.4f);
    D2D1_COLOR_F activeFillColor   = D2D1::ColorF(1.0f, 0.5f, 0.0f, 0.6f);
    D2D1_COLOR_F activeBorderColor = D2D1::ColorF(0.9f, 0.3f, 0.0f, 0.9f);
    float activeBorderWidth = 1.5f;
    float scrollMargin = 40.0f; // Padding in DIPs when centering active result
};

struct AutoScrollResult {
    bool shouldScroll = false;
    float newScrollY = 0.0f;
};

class SearchHighlightOverlay {
public:
    using TextPageProvider = std::function<core::interfaces::dom::ITextPage*(int pageIndex)>;
    using PageToViewConverter = std::function<void(double px, double py, int pageIndex, double& vx, double& vy)>;

    explicit SearchHighlightOverlay(SearchHighlightStyle style = {});
    ~SearchHighlightOverlay() = default;

    // Results management
    void SetResults(const std::vector<SearchResult>& results, int activeIndex = 0);
    void SetActiveIndex(int activeIndex);
    int GetActiveIndex() const;
    size_t GetResultCount() const;
    const SearchResult* GetResult(size_t index) const;
    const SearchResult* GetActiveResult() const;
    const std::vector<SearchResult>& GetAllResults() const;
    void Clear();

    // Style configuration
    const SearchHighlightStyle& GetStyle() const;
    void SetStyle(const SearchHighlightStyle& style);

    // Rendering
    void Render(
        ID2D1RenderTarget* target,
        const D2D1_RECT_F& viewportBounds,
        TextPageProvider textPageProvider,
        PageToViewConverter pageToView
    );

    // Auto-scrolling calculation for centering active match in viewport
    AutoScrollResult CalculateAutoScroll(
        float viewportHeight,
        float currentScrollY,
        TextPageProvider textPageProvider,
        PageToViewConverter pageToView
    ) const;

private:
    std::vector<SearchResult> m_results;
    int m_activeIndex = -1;
    SearchHighlightStyle m_style;

    // Cached brushes
    ComPtr<ID2D1SolidColorBrush> m_inactiveBrush;
    ComPtr<ID2D1SolidColorBrush> m_activeBrush;
    ComPtr<ID2D1SolidColorBrush> m_activeBorderBrush;
    ID2D1RenderTarget* m_lastTarget = nullptr;

    void EnsureBrushes(ID2D1RenderTarget* target);
};

} // namespace search
} // namespace ui
