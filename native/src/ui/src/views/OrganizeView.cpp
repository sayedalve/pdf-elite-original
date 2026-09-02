#include "OrganizeView.h"
#include "../NativeDesignSystem.h"

namespace views {

OrganizeView::OrganizeView() {
    SetBackgroundColor(design::Colors::SurfaceElevated);
    
    m_toolbar = std::make_shared<components::OrganizeToolbar>();
    m_grid = std::make_shared<components::OrganizeGrid>();
    AddChild(m_toolbar);
    AddChild(m_grid);
    // Wire up actions from Toolbar to Grid
    m_toolbar->onAction = [this](const std::wstring& action) {
        if (!m_doc) return;
        m_grid->HandleAction(action);
    };
    
    m_grid->onSelectionChanged = [this](const std::set<int>& selection) {
        m_toolbar->UpdateState(selection.size() > 0, static_cast<int>(selection.size()));
        if (onPageChanged) onPageChanged(selection.empty() ? -1 : *selection.begin(), m_doc ? m_doc->PageCount() : 0);
            };
}

void OrganizeView::SetDocument(std::shared_ptr<core::interfaces::dom::IDocument> doc) {
    m_doc = doc;
    m_grid->SetDocument(doc);
}

void OrganizeView::SetDocumentId(const std::wstring& id) {
    m_grid->SetDocumentId(id);
}

void OrganizeView::SetDarkMode(bool dark) {
    m_grid->SetDarkMode(dark);
    m_toolbar->SetDarkMode(dark);
}

void OrganizeView::Layout(const D2D1_RECT_F& bounds) {
    UIElement::Layout(bounds);
    
    float tbHeight = 48.0f;
    
    
    m_toolbar->Layout(D2D1::RectF(bounds.left, bounds.top, bounds.right, bounds.top + tbHeight));
    m_grid->Layout(D2D1::RectF(bounds.left, bounds.top + tbHeight, bounds.right, bounds.bottom));
    }

void OrganizeView::Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) {
    Panel::Render(target);
}

} // namespace views
