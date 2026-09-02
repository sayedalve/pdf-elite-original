#pragma once
#include "../framework/UIElement.h"
#include <vector>
#include <string>
#include <functional>
#include <dwrite.h>
#include <wrl/client.h>

namespace components {

class TabBar : public framework::UIElement {
public:
    TabBar();
    
    void SetTabs(const std::vector<std::wstring>& titles, int activeIndex);
    
    void SetOnTabSelected(std::function<void(int)> callback) { onTabSelected = callback; }
    void SetOnTabClosed(std::function<void(int)> callback) { onTabClosed = callback; }

    void Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) override;
    
    void OnMouseDown(float x, float y) override;
    void OnMouseMove(float x, float y) override;

private:
    std::function<void(int)> onTabSelected;
    std::function<void(int)> onTabClosed;
    
    std::vector<std::wstring> m_titles;
    int m_activeIndex = -1;
    int m_hoverIndex = -1;

    // Dedicated text format for tab titles: leading-aligned, single-line, with
    // an ellipsis trimming sign so long file names are truncated ("Long_file…")
    // instead of wrapping or overflowing into neighbouring tabs / the canvas.
    // Created lazily on first render (needs the DWrite factory) and cached.
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_tabFormat;
    Microsoft::WRL::ComPtr<IDWriteInlineObject> m_ellipsisSign;
    void EnsureTabFormat();
};

} // namespace components
