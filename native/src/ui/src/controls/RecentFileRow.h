#pragma once
#include "../framework/UIElement.h"
#include <string>
#include <functional>
#include "IconRenderer.h"

namespace controls {

class RecentFileRow : public framework::UIElement {
public:
    RecentFileRow(const std::wstring& filename, const std::wstring& modified, const std::wstring& size);

    void SetOnClick(std::function<void()> onClick) { m_onClick = onClick; }

    void Render(ComPtr<ID2D1RenderTarget> target) override;

    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    void OnMouseLeave() override;

private:
    std::wstring m_filename;
    std::wstring m_modified;
    std::wstring m_size;
    std::function<void()> m_onClick;
    
    bool m_isHovered = false;
    bool m_isPressed = false;
};

} // namespace controls
