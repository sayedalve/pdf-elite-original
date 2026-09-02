#pragma once
#include "../framework/UIElement.h"
#include "../controls/IconRenderer.h"
#include <string>
#include <functional>

namespace components {

class SidebarItem : public framework::UIElement {
public:
    SidebarItem(const std::wstring& text, controls::IconType icon);
    
    void Render(ComPtr<ID2D1RenderTarget> target) override;
    void OnMouseMove(float x, float y) override;
    void OnMouseLeave() override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    
    void SetSelected(bool sel) { m_isSelected = sel; }
    void SetOnClick(std::function<void()> cb) { m_onClick = cb; }

private:
    std::wstring m_text;
    controls::IconType m_icon;
    bool m_isHovered = false;
    bool m_isPressed = false;
    bool m_isSelected = false;
    std::function<void()> m_onClick;
};

} // namespace components
