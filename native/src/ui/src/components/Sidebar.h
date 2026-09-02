#pragma once
#include "../framework/Panel.h"
#include "../controls/IconButton.h"
#include "SidebarItem.h"

namespace components {

class Sidebar : public framework::Panel {
public:
    Sidebar();
    void Render(ComPtr<ID2D1RenderTarget> target) override;
    void Layout(const D2D1_RECT_F& bounds) override;
    std::function<void()> onNewDocument;
    
private:
    std::shared_ptr<controls::IconButton> m_newDocBtn;
    
    std::shared_ptr<SidebarItem> m_navHome;
    std::shared_ptr<SidebarItem> m_navTools;
    std::shared_ptr<SidebarItem> m_navRecent;
    std::shared_ptr<SidebarItem> m_navStarred;
    // We could add more later
};

} // namespace components
