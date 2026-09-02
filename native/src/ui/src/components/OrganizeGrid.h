#pragma once
#include "framework/Panel.h"
#include "core/interfaces/dom/IDocument.h"
#include "../TileCache.h"
#include "../../../core/models/RenderResult.h"

#include <memory>
#include <functional>
#include <set>
#include <vector>

#include <vector>

namespace components {

class OrganizeGrid : public framework::Panel {
public:
    OrganizeGrid();
    ~OrganizeGrid() override;
    
    void SetDocument(std::shared_ptr<core::interfaces::dom::IDocument> doc);
    void SetDocumentId(const std::wstring& id) { 
        m_documentId = id; 
    }
    void SetDarkMode(bool dark);
    
    void HandleAction(const std::wstring& action);

    void Layout(const D2D1_RECT_F& bounds) override;
    void Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) override;

    void OnMouseWheel(float delta) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseMove(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    
    std::function<void(const std::set<int>&)> onSelectionChanged;

    void OnTileReady(const core::models::RenderResult* result, Microsoft::WRL::ComPtr<ID2D1RenderTarget> target);

private:
    std::shared_ptr<core::interfaces::dom::IDocument> m_doc;
    std::wstring m_documentId;
    bool m_isDarkMode = false;
    
    float m_scrollY = 0.0f;
    float m_maxScrollY = 0.0f;
    
    std::set<int> m_selectedPages;
    int m_activePage = 0;
    
    // Drag state
    bool m_isDragging = false;
    float m_dragStartX = 0.0f;
    float m_dragStartY = 0.0f;
    int m_dragInsertIndex = -1; // Index BEFORE which to insert
    
    float m_thumbnailWidth = 190.0f;
    float m_paddingX = 40.0f;
    float m_paddingY = 60.0f;
    
    struct ThumbData {
        int pageIndex;
        D2D1_RECT_F bounds;
        float height;
    };
    std::vector<ThumbData> m_thumbs;
    bool m_layoutDirty = true;
    
    TileCache m_cache;
    uint64_t m_generation = 1;
    
    void UpdateLayoutParams();
    void RequestVisibleThumbnails();
    int GetInsertIndexFromPos(float px, float py);
    void ClearSelection();
};

} // namespace components
