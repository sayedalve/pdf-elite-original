#pragma once
#include "../framework/Panel.h"
#include "Sidebar.h"
#include "TabBar.h"
#include "Toolbar.h"

namespace views { class DocumentView; class HomeView; }

namespace components {

enum class AppShellMode {
    Home,
    Document
};

class AppShell : public framework::Panel {
public:
    AppShell();
    
    void Layout(const D2D1_RECT_F& bounds) override;
    void Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> renderTarget) override;
    bool HitTest(float x, float y) override;

    void SetMode(AppShellMode mode);
    AppShellMode GetMode() const { return m_mode; }

    void SetHomeContent(std::shared_ptr<views::HomeView> view);
    void SetDocumentWorkspace(std::shared_ptr<views::DocumentView> workspace);

    std::shared_ptr<TabBar> GetDocumentTabs();
    std::shared_ptr<Toolbar> GetReadingToolbar();
    
    std::function<void()> onHomeRequest;
    
    std::function<void()> onMinimize;
    std::function<void()> onMaximize;
    std::function<void()> onClose;
    
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    void OnMouseMove(float x, float y) override;
    void OnMouseLeave() override;
    void OnMouseWheel(float delta) override;

private:
    void RenderTitleBar(Microsoft::WRL::ComPtr<ID2D1RenderTarget> rt);

    AppShellMode m_mode = AppShellMode::Home;
    std::shared_ptr<TabBar> m_tabBar;
    std::shared_ptr<views::HomeView> m_homeView;
    std::shared_ptr<views::DocumentView> m_documentView;
    
    int m_hoveredControl = -1;
    int m_pressedControl = -1; // 0=min, 1=max, 2=close
};

} // namespace components
