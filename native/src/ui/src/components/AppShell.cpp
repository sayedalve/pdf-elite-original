#include "AppShell.h"
#include "PopupManager.h"
#include "../NativeDesignSystem.h"
#include "../views/HomeView.h"
#include "../views/DocumentView.h"

namespace components {

AppShell::AppShell() {
    SetBackgroundColor(design::Colors::Background);
    m_tabBar = std::make_shared<TabBar>();
    AddChild(m_tabBar);
}

void AppShell::SetMode(AppShellMode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        Layout(m_bounds);
    }
}

void AppShell::SetHomeContent(std::shared_ptr<views::HomeView> view) {
    m_homeView = view;
    ClearChildren();
    if (m_homeView) AddChild(m_homeView);
    if (m_documentView) AddChild(m_documentView);
    AddChild(m_tabBar);
    if (m_mode == AppShellMode::Home) {
        Layout(m_bounds);
    }
}

void AppShell::SetDocumentWorkspace(std::shared_ptr<views::DocumentView> workspace) {
    m_documentView = workspace;
    ClearChildren();
    if (m_homeView) AddChild(m_homeView);
    if (m_documentView) AddChild(m_documentView);
    AddChild(m_tabBar);
    if (m_mode == AppShellMode::Document) {
        Layout(m_bounds);
    }
}

std::shared_ptr<TabBar> AppShell::GetDocumentTabs() {
    return m_tabBar;
}

std::shared_ptr<Toolbar> AppShell::GetReadingToolbar() {
    return m_documentView ? m_documentView->GetToolbar() : nullptr;
}

void AppShell::Layout(const D2D1_RECT_F& bounds) {
    UIElement::Layout(bounds);

    float titleBarHeight = 40.0f;
    D2D1_RECT_F titleBarBounds = {bounds.left, bounds.top, bounds.right, bounds.top + titleBarHeight};
    D2D1_RECT_F contentBounds = {bounds.left, titleBarBounds.bottom, bounds.right, bounds.bottom};

    // The TabBar occupies the left-center of the title bar. 
    // We leave 60px on the left for the App Icon, and 160px on the right for window controls.
    if (m_tabBar) {
        m_tabBar->Layout({bounds.left + 60.0f, bounds.top, bounds.right - 200.0f, bounds.top + titleBarHeight});
    }

    if (m_homeView) {
        m_homeView->SetVisible(m_mode == AppShellMode::Home);
        if (m_homeView->IsVisible()) m_homeView->Layout(contentBounds);
    }
    
    if (m_documentView) {
        m_documentView->SetVisible(m_mode == AppShellMode::Document);
        if (m_documentView->IsVisible()) m_documentView->Layout(contentBounds);
    }
}

void AppShell::RenderTitleBar(Microsoft::WRL::ComPtr<ID2D1RenderTarget> rt) {
    // Custom Window Controls (Minimize, Maximize, Close)
    float controlW = 46.0f;
    float controlH = 40.0f;
    float startX = m_bounds.right - (controlW * 3.0f);
    
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    for (int i = 0; i < 3; ++i) {
        float x = startX + (i * controlW);
        D2D1_RECT_F rect = D2D1::RectF(x, 0, x + controlW, controlH);
        
        bool pressed = (m_pressedControl == i);
        bool hovered = (m_hoveredControl == i);
        
        if (hovered || pressed) {
            D2D1_COLOR_F bg;
            if (i == 2) {
                bg = pressed ? D2D1::ColorF(0.7f, 0.1f, 0.1f) : D2D1::ColorF(0.9f, 0.1f, 0.1f);
            } else {
                bg = pressed ? design::Colors::SurfacePressed : design::Colors::SurfaceHover;
            }
            rt->CreateSolidColorBrush(bg, &brush);
            rt->FillRectangle(rect, brush.Get());
        }
        
        D2D1_COLOR_F iconColor = (m_hoveredControl == 2 && i == 2) ? D2D1::ColorF(D2D1::ColorF::White) : design::Colors::TextPrimary;
        rt->CreateSolidColorBrush(iconColor, &brush);
        
        float cx = x + (controlW / 2.0f);
        float cy = controlH / 2.0f;
        
        if (i == 0) { // Minimize
            rt->DrawLine({cx - 5.0f, cy}, {cx + 5.0f, cy}, brush.Get(), 1.0f);
        } else if (i == 1) { // Maximize
            rt->DrawRectangle({cx - 4.0f, cy - 4.0f, cx + 4.0f, cy + 4.0f}, brush.Get(), 1.0f);
        } else if (i == 2) { // Close
            rt->DrawLine({cx - 4.0f, cy - 4.0f}, {cx + 4.0f, cy + 4.0f}, brush.Get(), 1.0f);
            rt->DrawLine({cx - 4.0f, cy + 4.0f}, {cx + 4.0f, cy - 4.0f}, brush.Get(), 1.0f);
        }
    }
}

void AppShell::Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> renderTarget) {
    framework::Panel::Render(renderTarget);
    RenderTitleBar(renderTarget);
    ui::components::PopupManager::Instance().Render(renderTarget);
}

bool AppShell::HitTest(float x, float y) {
    if (y < 40.0f && x >= m_bounds.right - (46.0f * 3.0f)) return true;
    return framework::Panel::HitTest(x, y); 
}

void AppShell::OnMouseMove(float x, float y) {
    ui::components::PopupManager::Instance().OnMouseMove(x, y);
    int hover = -1;
    if (y < 40.0f && x >= m_bounds.right - (46.0f * 3.0f)) {
        float relX = x - (m_bounds.right - (46.0f * 3.0f));
        hover = static_cast<int>(relX / 46.0f);
        if (hover > 2) hover = 2;
    }
    if (m_hoveredControl != hover) {
        m_hoveredControl = hover;
    }
    framework::Panel::OnMouseMove(x, y);
}

void AppShell::OnMouseWheel(float delta) {
    if (m_mode == AppShellMode::Home && m_homeView) {
        m_homeView->OnMouseWheel(delta);
    }
}

void AppShell::OnMouseLeave() {
    if (m_hoveredControl != -1 || m_pressedControl != -1) {
        m_hoveredControl = -1;
        m_pressedControl = -1;
    }
    framework::Panel::OnMouseLeave();
}

void AppShell::OnMouseDown(float x, float y) {
    if (ui::components::PopupManager::Instance().OnLButtonDown(x, y)) {
        return;
    }
    if (m_hoveredControl != -1) {
        m_pressedControl = m_hoveredControl;
    } else {
        framework::Panel::OnMouseDown(x, y);
    }
}

void AppShell::OnMouseUp(float x, float y) {
    ui::components::PopupManager::Instance().OnLButtonUp(x, y);
    if (m_pressedControl != -1) {
        int pressed = m_pressedControl;
        m_pressedControl = -1;
        if (pressed == m_hoveredControl) {
            if (pressed == 0 && onMinimize) onMinimize();
            else if (pressed == 1 && onMaximize) onMaximize();
            else if (pressed == 2 && onClose) onClose();
        }
    }
    framework::Panel::OnMouseUp(x, y);
}

} // namespace components


