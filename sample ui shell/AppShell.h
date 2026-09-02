// AppShell.h - Application shell: TopBar, Tabs, Toolbar, Sidebar, StatusBar
#pragma once
#include "Theme.h"
#include "LayoutManager.h"
#include "CommandManager.h"
#include "PdfDocument.h"
#include <string>
#include <vector>

namespace PdfElite {

enum class ViewerMode { View, Comment, Edit, Organize };

struct TabInfo {
    std::wstring title;
    bool active;
    PdfDocument* doc;
};

class AppShell {
public:
    HRESULT Initialize(ID2D1RenderTarget* rt, IDWriteFactory* dwFactory, Theme* theme, CommandManager* cmdMgr);
    void Release();

    void Render(ID2D1RenderTarget* rt, const D2D1_RECT_F& topBar, const D2D1_RECT_F& toolbar, const D2D1_RECT_F& leftRail, const D2D1_RECT_F& rightRail, const D2D1_RECT_F& status, ViewerMode mode, const std::vector<TabInfo>& tabs, int currentPage, int totalPages, float zoom);
    bool HitTest(int x, int y, CommandId& outCmd);

    // Real state
    void SetViewerMode(ViewerMode m) { m_mode = m; }
    ViewerMode GetViewerMode() const { return m_mode; }

private:
    void RenderTopBar(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect, const std::vector<TabInfo>& tabs);
    void RenderToolbar(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect);
    void RenderLeftRail(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect);
    void RenderRightRail(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect, int currentPage, int totalPages, float zoom);
    void RenderStatus(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect, int currentPage, int totalPages, float zoom);
    void DrawToolbarItem(ID2D1RenderTarget* rt, float& x, float y, float h, const wchar_t* label, bool active = false, bool hasDropdown = false);

    // Toolbar per mode - exact from 4 screenshots
    void RenderViewToolbar(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect);
    void RenderCommentToolbar(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect);
    void RenderEditToolbar(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect);
    void RenderOrganizeToolbar(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect);

    Theme* m_theme = nullptr;
    CommandManager* m_cmd = nullptr;
    ViewerMode m_mode = ViewerMode::View;

    // Hover states - explicit, not accidental Windows default
    int m_hoveredLeftRail = -1;
    int m_hoveredToolbar = -1;
};

} // namespace PdfElite
