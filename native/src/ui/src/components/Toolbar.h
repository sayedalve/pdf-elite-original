#pragma once
#include "../framework/Panel.h"
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include "../controls/IconButton.h"
#include "../AppMode.h"
#include "../CommandManager.h"

namespace components {

class Toolbar : public framework::Panel, public ui::commands::IActionObserver {
public:
    Toolbar();
    ~Toolbar() override;

    void Layout(const D2D1_RECT_F& bounds) override;
    void Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) override;
    void SetActiveTool(const std::wstring& toolName);
    
    void OnActionStateChanged(const std::wstring& actionId, bool isEnabled, bool isChecked) override;

    // Show only the buttons that belong to `mode`, hide the rest, and re-layout.
    void SetMode(app::AppMode mode);
    app::AppMode GetMode() const { return m_mode; }

    std::function<void(const std::wstring&)> onAction;

private:
    struct Btn {
        std::shared_ptr<controls::IconButton> button;
        std::vector<app::AppMode> modes; // modes in which this button is visible
        std::wstring label;              // action name / display text
        bool isText;                     // text-only (variable width) vs icon (fixed)
        int group;                       // layout group; a gap is drawn on change
    };

    std::shared_ptr<controls::IconButton> AddButton(
        const std::wstring& text, controls::IconType icon, bool isText,
        bool enabled, int group, std::vector<app::AppMode> modes);

    void ApplyModeVisibility();

    std::vector<Btn> m_btns;
    std::unordered_map<std::wstring, std::shared_ptr<controls::IconButton>> m_buttons;
    app::AppMode m_mode = app::AppMode::View;
};

} // namespace components
