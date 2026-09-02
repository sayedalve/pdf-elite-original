#pragma once
#include "framework/Panel.h"
#include "components/OrganizeGrid.h"
#include "components/OrganizeToolbar.h"
#include "components/StatusBar.h"
#include "core/interfaces/dom/IDocument.h"
#include <memory>
#include <functional>

namespace views {

class OrganizeView : public framework::Panel {
public:
    OrganizeView();
    
    void SetDocument(std::shared_ptr<core::interfaces::dom::IDocument> doc);
    void SetDocumentId(const std::wstring& id);
    void SetDarkMode(bool dark);

    void Layout(const D2D1_RECT_F& bounds) override;
    void Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) override;

    std::shared_ptr<components::OrganizeGrid> GetGrid() { return m_grid; }
    std::shared_ptr<components::OrganizeToolbar> GetToolbar() { return m_toolbar; }
    std::function<void(int, int)> onPageChanged;
    private:
    std::shared_ptr<components::OrganizeGrid> m_grid;
    std::shared_ptr<components::OrganizeToolbar> m_toolbar;
    
    
    std::shared_ptr<core::interfaces::dom::IDocument> m_doc;
};

} // namespace views
