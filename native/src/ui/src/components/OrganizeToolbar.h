#pragma once
#include "framework/Panel.h"
#include <string>
#include <functional>

#include "../controls/IconRenderer.h"

namespace components {

class OrganizeToolbar : public framework::Panel {
public:
    OrganizeToolbar();
    
    std::function<void(const std::wstring&)> onAction;
    
    void UpdateState(bool hasSelection, int numSelected);
    void SetDarkMode(bool dark);
    
    void Layout(const D2D1_RECT_F& bounds) override;
    void Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) override;

    void OnMouseDown(float x, float y) override;
    void OnMouseMove(float x, float y) override;
    
private:
    struct Button {
        std::wstring id;
        std::wstring label; // text label
        controls::IconType iconType;
        D2D1_RECT_F bounds;
        bool enabled = true;
        bool requiresSelection = false;
        bool hover = false;
    };
    std::vector<Button> m_buttons;
    bool m_isDarkMode = false;
    
    int GetButtonAt(float x, float y);
};

} // namespace components
