#include "../../../core/models/RenderResult.h"
#pragma once
#include "framework/Panel.h"
#include "core/interfaces/dom/IDocument.h"
#include "../TileCache.h"

#include <memory>
#include <functional>
#include <set>

namespace components {

class ThumbnailViewer : public framework::Panel {
public:
    ThumbnailViewer();
    
    void SetDocument(std::shared_ptr<core::interfaces::dom::IDocument> doc);
    void SetDocumentId(const std::wstring& id) { m_documentId = id; }
    void Layout(const D2D1_RECT_F& bounds) override;
    void Render(ComPtr<ID2D1RenderTarget> target) override;
    
    void OnMouseWheel(float delta) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseMove(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    
    std::function<void(int)> onPageSelected;
    std::function<void(const std::set<int>&)> onSelectionChanged;
    std::function<void(int, int)> onPageMoved; // source, dest
    
    void SetActivePage(int pageIndex);
    void SetDarkMode(bool dark);
    const std::set<int>& GetSelectedPages() const { return m_selectedPages; }
    void ClearSelection();
    
    void OnTileReady(const core::models::RenderResult* result, Microsoft::WRL::ComPtr<ID2D1RenderTarget> target);

private:
    std::shared_ptr<core::interfaces::dom::IDocument> m_doc;
    std::wstring m_documentId;
    
    
    float m_scrollY = 0.0f;
    float m_maxScrollY = 0.0f;
    int m_activePage = 0;
    std::set<int> m_selectedPages;
    
    bool m_isDragging = false;
    bool m_isDarkMode = false;
    int m_dragSourcePage = -1;
    int m_dragInsertIndex = -1;
    float m_dragStartY = 0;
    
    float m_thumbnailWidth = 140.0f;
    float m_padding = 24.0f;
    
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
    int GetInsertIndexFromY(float py);
};

} // namespace components




