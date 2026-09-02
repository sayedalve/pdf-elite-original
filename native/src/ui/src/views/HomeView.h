#pragma once
#include "framework/Panel.h"
#include <functional>
#include "../controls/SearchBox.h"

namespace views {

class HomeView : public framework::Panel {
public:
    HomeView();
    
    void Layout(const D2D1_RECT_F& bounds) override;
    void Render(ComPtr<ID2D1RenderTarget> target) override;
    bool HitTest(float x, float y) override;
    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseWheel(float delta);

    std::function<void()> onOpenRequest;
    std::function<void()> onCreateRequest;
    std::function<void(const std::wstring&)> onToolRequest;
    std::function<void(const std::wstring&)> onOpenFileRequest;
    std::function<void(const std::wstring&)> onOpenFolderRequest;
    std::function<void(int)> onNavRequest;

    int GetSelectedNav() const { return m_selectedNav; }
    void SetSelectedNav(int idx) { m_selectedNav = idx; }

private:
    void RenderHomeSidebar(ComPtr<ID2D1RenderTarget> target);
    void RenderHomeMain(ComPtr<ID2D1RenderTarget> target);

    std::shared_ptr<framework::Panel> m_quickToolsPanel;
    std::shared_ptr<framework::Panel> m_recentFilesPanel;
    
    // Sidebar hit test areas
    D2D1_RECT_F m_openBtnRect = {0,0,0,0};
    D2D1_RECT_F m_createBtnRect = {0,0,0,0};
    int m_hoveredNav = -1;
    float m_scrollOffset = 0.0f;
    int m_selectedNav = 0;
    D2D1_RECT_F m_navRects[3];
    
    // Main hit test areas
    D2D1_RECT_F m_allToolsRect = {0,0,0,0};
    bool m_hoveredAllTools = false;

    int m_hoveredTool = -1;
    D2D1_RECT_F m_toolRects[8];
    int m_hoveredFile = -1;
    int m_hoveredStar = -1;
    std::vector<D2D1_RECT_F> m_fileRects;
    std::vector<D2D1_RECT_F> m_starRects;
    std::vector<std::wstring> m_displayedPaths;
};

} // namespace views
