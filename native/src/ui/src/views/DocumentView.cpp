#include "DocumentView.h"
#include "../NativeDesignSystem.h"
#include "../controls/IconButton.h"
#include "../controls/SearchBox.h"
#include <string>
#include "../../../utils/Logger.h"

namespace views {

DocumentView::DocumentView() {
    SetBackgroundColor(design::Colors::Background);
    
    // Left Sidebar (Thumbnails)
    m_leftSidebar = std::make_shared<components::ThumbnailViewer>();
    m_leftSidebar->SetBackgroundColor(design::Colors::SurfaceElevated);
    m_leftSidebar->SetVisible(false);
    
    // Left vertical mode rail (Home / Comment / Edit / Convert / View / ...)
    m_modeRail = std::make_shared<components::ModeRail>();

    m_topBar = std::make_shared<framework::Panel>();
    m_toolbar = std::make_shared<components::Toolbar>();

    // PDF Canvas
    m_pdfCanvasContainer = std::make_shared<framework::Panel>();
    m_pdfCanvasContainer->SetBackgroundColor(design::Colors::Background);

    // Right Rail (formerly Status Bar)
    m_statusBar = std::make_shared<components::StatusBar>();
    m_statusBar->SetBackgroundColor(design::Colors::SidebarBg);
    m_statusBar->onAction = [this](const std::wstring& action) {
        if (onToolbarAction) {
            onToolbarAction(action);
        }
    };

    m_propertiesPanel = std::make_shared<components::PropertiesPanel>();
    m_propertiesPanel->SetVisible(false); // Hidden by default
    
    m_organizeView = std::make_shared<OrganizeView>();
    m_organizeView->SetVisible(false);
    m_organizeView->onPageChanged = [this](int current, int total) { if (m_statusBar) m_statusBar->SetPageInfo(current, total); };
    
    AddChild(m_modeRail);
    AddChild(m_leftSidebar); // Wait, this is the thumbnail panel. It should be inside or attached to center. We'll handle its layout.
    AddChild(m_topBar);
    AddChild(m_toolbar);
    AddChild(m_pdfCanvasContainer);
    AddChild(m_organizeView);
    AddChild(m_statusBar);
    AddChild(m_propertiesPanel);
}

void DocumentView::SetAppMode(app::AppMode mode) {
    m_currentMode = mode;
    bool isOrg = (mode == app::AppMode::Organize);
    m_organizeView->SetVisible(isOrg);
    m_pdfCanvasContainer->SetVisible(!isOrg);
    m_toolbar->SetVisible(!isOrg);
    m_leftSidebar->SetVisible(!isOrg && m_leftSidebar->IsVisible()); 
    // Properties panel visibility should be managed, but hide in org
    if (isOrg) m_propertiesPanel->SetVisible(false);
}

void DocumentView::Layout(const D2D1_RECT_F& client) {
    UIElement::Layout(client);
    
    // 100% Faithful Recreation Bounds
    D2D1_RECT_F leftRail = D2D1::RectF(client.left, client.top, client.left + 60, client.bottom); // Narrower
    D2D1_RECT_F rightRail = D2D1::RectF(client.right - 48, client.top + 48, client.right, client.bottom);
    D2D1_RECT_F toolbar = D2D1::RectF(leftRail.right, client.top, client.right, client.top + 48);
    D2D1_RECT_F center = D2D1::RectF(leftRail.right, toolbar.bottom, rightRail.left, client.bottom);

    m_modeRail->Layout(leftRail);
    m_topBar->Layout(toolbar); // Use topBar as the background for toolbar
    m_toolbar->Layout(toolbar);
    
    bool isOrg = m_organizeView->IsVisible();
    m_statusBar->SetVisible(!isOrg);
    m_statusBar->Layout(rightRail);

    // PDF Canvas area: sidebar on LEFT, properties panel on RIGHT
    float canvasLeft = center.left;
    float canvasRight = center.right;

    // BUG-20 fix: thumbnail sidebar belongs on the LEFT side of the canvas, not the right
    if (m_leftSidebar->IsVisible()) {
        float panelWidth = 240.0f;
        D2D1_RECT_F lsBounds = {canvasLeft, center.top, canvasLeft + panelWidth, center.bottom};
        m_leftSidebar->Layout(lsBounds);
        canvasLeft += panelWidth;
    } else {
        m_leftSidebar->Layout({0,0,0,0});
    }

    if (m_propertiesPanel->IsVisible()) {
        canvasRight -= kPropertiesPanelWidth;
        D2D1_RECT_F propsBounds = {canvasRight, center.top, canvasRight + kPropertiesPanelWidth, center.bottom};
        m_propertiesPanel->Layout(propsBounds);
    }
    
    if (m_showRightPanel) {
        float rightPanelWidth = 300.0f;
        canvasRight -= rightPanelWidth;
        m_rightPanelBounds = {canvasRight, center.top, canvasRight + rightPanelWidth, center.bottom};
    } else {
        m_rightPanelBounds = {0,0,0,0};
    }

    D2D1_RECT_F canvasBounds = {canvasLeft, center.top, canvasRight, center.bottom};
    m_pdfCanvasContainer->Layout(canvasBounds);
    
    if (m_organizeView->IsVisible()) {
        m_organizeView->Layout(D2D1::RectF(leftRail.right, client.top, client.right, client.bottom));
    } else {
        m_organizeView->Layout({0,0,0,0});
    }
}

void DocumentView::Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) {
    utils::Logger::Log("DOCUMENTVIEW_RENDER_START");
    Panel::Render(target);
    // BUG-32: Removed 500+ useless grid line draw calls per frame. The grid lines
    // were drawn at alpha=0.03 (nearly invisible) and were entirely covered by the
    // white PDF page background, so they were never visible.
    // BUG-31: Removed hardcoded "1 / 1  100%" status text. StatusBar component renders itself.
    utils::Logger::Log("DOCUMENTVIEW_RENDER_END");
}

} // namespace views
