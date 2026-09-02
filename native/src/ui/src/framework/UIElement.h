#pragma once
#include <d2d1_1.h>
#include <wrl/client.h>
#include <string>
#include <memory>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace framework {

class UIElement {
public:
    virtual ~UIElement() = default;

    virtual void Render(ComPtr<ID2D1RenderTarget> target) = 0;
    virtual void Layout(const D2D1_RECT_F& bounds) {
        m_bounds = bounds;
    }

    virtual bool HitTest(float x, float y) {
        return x >= m_bounds.left && x <= m_bounds.right &&
               y >= m_bounds.top && y <= m_bounds.bottom;
    }

    virtual void OnMouseMove(float x, float y) { (void)x; (void)y; }
    virtual void OnMouseDown(float x, float y) { (void)x; (void)y; }
    virtual void OnMouseUp(float x, float y) { (void)x; (void)y; }
    virtual void OnMouseLeave() {}
    
    virtual std::wstring GetTooltipText() const { return L""; }
    virtual void OnMouseWheel(float delta) { (void)delta; }

    const D2D1_RECT_F& GetBounds() const { return m_bounds; }

    // Visibility: an invisible element is neither rendered nor considered for
    // hit-testing / mouse dispatch by container panels. Used by the mode-aware
    // toolbar to show only the active mode's buttons without destroying them.
    void SetVisible(bool v) { m_visible = v; }
    bool IsVisible() const { return m_visible; }

protected:
    D2D1_RECT_F m_bounds = {0, 0, 0, 0};
    bool m_visible = true;
};

} // namespace framework
