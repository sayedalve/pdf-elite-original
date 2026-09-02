#pragma once
#include "../framework/UIElement.h"
#include "../controls/IconRenderer.h"
#include "../AppMode.h"
#include <vector>
#include <string>
#include <functional>
#include <dwrite.h>
#include <wrl/client.h>

namespace components {

// Left vertical rail of workspace modes (Home / Comment / Edit / Convert /
// View / Organize / Tools / Form). Renders its own items rather than using
// IconButton, so it can show an icon-over-label cell with an active accent bar.
// Modes whose backend is not implemented are shown but disabled (greyed, inert)
// so the UI communicates intent without offering fake functionality.
class ModeRail : public framework::UIElement {
public:
    ModeRail();

    void SetActiveMode(app::AppMode mode) { m_activeMode = mode; }
    app::AppMode GetActiveMode() const { return m_activeMode; }

    void SetOnModeSelected(std::function<void(app::AppMode)> cb) { m_onModeSelected = cb; }

    void Render(ComPtr<ID2D1RenderTarget> target) override;

    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    void OnMouseLeave() override;

    static constexpr float kItemHeight = 58.0f;

private:
    struct Item {
        app::AppMode mode;
        std::wstring label;
        controls::IconType icon;
        bool enabled;
    };

    int ItemAt(float x, float y) const; // returns item index under (x,y), or -1
    void EnsureFormat();

    std::vector<Item> m_items;
    app::AppMode m_activeMode = app::AppMode::View;
    int m_hoverIndex = -1;
    int m_pressedIndex = -1;
    std::function<void(app::AppMode)> m_onModeSelected;

    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_labelFormat;
};

} // namespace components
