#pragma once
#include "framework/Panel.h"
#include "components/ThumbnailViewer.h"
#include "components/Toolbar.h"
#include "components/TabBar.h"
#include "components/StatusBar.h"
#include "components/ModeRail.h"
#include "components/PropertiesPanel.h"
#include "OrganizeView.h"

namespace views {

class DocumentView : public framework::Panel {
public:
    void SetRightPanelVisible(bool visible) { m_showRightPanel = visible; }
    bool IsRightPanelVisible() const { return m_showRightPanel; }
    D2D1_RECT_F GetRightPanelBounds() const { return m_rightPanelBounds; }
    DocumentView();

    std::function<void(const std::wstring&)> onToolbarAction;
    std::function<void()> onHomeRequest;

    void Layout(const D2D1_RECT_F& bounds) override;
    void Render(ComPtr<ID2D1RenderTarget> target) override;

    std::shared_ptr<framework::Panel> GetTopBar() { return m_topBar; }
    std::shared_ptr<components::ModeRail> GetModeRail() { return m_modeRail; }
    std::shared_ptr<components::Toolbar> GetToolbar() { return m_toolbar; }
    std::shared_ptr<components::ThumbnailViewer> GetLeftSidebar() { return m_leftSidebar; }
    std::shared_ptr<components::StatusBar> GetStatusBar() { return m_statusBar; }
    std::shared_ptr<framework::Panel> GetCanvasContainer() { return m_pdfCanvasContainer; }
    std::shared_ptr<components::PropertiesPanel> GetPropertiesPanel() { return m_propertiesPanel; }
    std::shared_ptr<OrganizeView> GetOrganizeView() { return m_organizeView; }

    void SetAppMode(app::AppMode mode);

    // Width of the left vertical mode rail (Home / Comment / Edit / ...).
    static constexpr float kModeRailWidth = 56.0f;
    static constexpr float kPropertiesPanelWidth = 240.0f;

private:
    D2D1_RECT_F m_rightPanelBounds = {0,0,0,0};
    bool m_showRightPanel = false;
    std::shared_ptr<components::ThumbnailViewer> m_leftSidebar;
    std::shared_ptr<framework::Panel> m_topBar;
    std::shared_ptr<components::ModeRail> m_modeRail;
    std::shared_ptr<components::Toolbar> m_toolbar;

    std::shared_ptr<framework::Panel> m_pdfCanvasContainer;
    std::shared_ptr<components::StatusBar> m_statusBar;
    std::shared_ptr<components::PropertiesPanel> m_propertiesPanel;
    std::shared_ptr<OrganizeView> m_organizeView;
    app::AppMode m_currentMode = app::AppMode::View;
};

} // namespace views
