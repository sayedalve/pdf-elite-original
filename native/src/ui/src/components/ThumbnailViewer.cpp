#include "../../../core/RenderController.h"
#include "ThumbnailViewer.h"

#include <fstream>
#include <chrono>
static void LogPipeline(const std::string& msg) {
    std::ofstream out("pipeline.log", std::ios_base::app);
    out << "[" << std::chrono::system_clock::now().time_since_epoch().count() << "] " << msg << "\n";
}


#include "../NativeDesignSystem.h"
#include "../GraphicsDevice.h"
#include <algorithm>
#include <string>

namespace components {

ThumbnailViewer::ThumbnailViewer() {
    SetBackgroundColor(design::Colors::SurfaceElevated);
}

void ThumbnailViewer::SetDocument(std::shared_ptr<core::interfaces::dom::IDocument> doc) {
    m_doc = doc;
    
    m_scrollY = 0.0f;
    m_activePage = 0;
    m_selectedPages.clear();
    m_generation++;
    m_cache.InvalidateAll();
    m_thumbs.clear();
    m_layoutDirty = true;
    if (m_doc) {
        UpdateLayoutParams();
    }
}

void ThumbnailViewer::SetActivePage(int pageIndex) {
    if (m_activePage != pageIndex) {
        m_activePage = pageIndex;
        m_selectedPages.clear();
        m_selectedPages.insert(pageIndex);
        
        
        if (m_activePage >= 0 && m_activePage < (int)m_thumbs.size()) {
            auto& t = m_thumbs[m_activePage];
            float viewHeight = GetBounds().bottom - GetBounds().top;
            if (t.bounds.top - m_scrollY < 0) {
                m_scrollY = t.bounds.top - m_padding;
            } else if (t.bounds.bottom - m_scrollY > viewHeight) {
                m_scrollY = t.bounds.bottom - viewHeight + m_padding;
            }
            m_scrollY = std::max(0.0f, std::min(m_scrollY, m_maxScrollY));
        }
    }
}

void ThumbnailViewer::ClearSelection() {
    m_selectedPages.clear();
    
}

void ThumbnailViewer::UpdateLayoutParams() {
    LogPipeline("ThumbnailViewer::UpdateLayoutParams start");
    if (!m_doc || m_thumbnailWidth <= 0) {
        LogPipeline("ThumbnailViewer::UpdateLayoutParams no doc");
        return;
    }
    
    LogPipeline("ThumbnailViewer::UpdateLayoutParams Get PageCount");
    int numPages = m_doc->PageCount();
    LogPipeline("ThumbnailViewer::UpdateLayoutParams PageCount=" + std::to_string(numPages));
    
    float currentY = m_padding;
    float contentX = m_padding;
    
    if (m_layoutDirty || m_thumbs.size() != numPages) {
        LogPipeline("ThumbnailViewer::UpdateLayoutParams resizing thumbs to " + std::to_string(numPages));
        m_thumbs.resize(numPages);
        
        float height0 = m_thumbnailWidth;
        if (numPages > 0) {
            LogPipeline("ThumbnailViewer::UpdateLayoutParams calling GetPageSize(0)");
            auto sz0 = m_doc->GetPageSize(0);
            LogPipeline("ThumbnailViewer::UpdateLayoutParams GetPageSize(0) returned");
            float aspect0 = (sz0.height > 0) ? (sz0.width / sz0.height) : 1.0f;
            height0 = m_thumbnailWidth / aspect0;
        }

        for (int i = 0; i < numPages; ++i) {
            float height = height0;
            if (numPages <= 50) {
                auto sz = m_doc->GetPageSize(i);
                float aspect = (sz.height > 0) ? (sz.width / sz.height) : 1.0f;
                height = m_thumbnailWidth / aspect;
            }
            
            m_thumbs[i].pageIndex = i;
            m_thumbs[i].height = height;
            m_thumbs[i].bounds = {contentX, currentY, contentX + m_thumbnailWidth, currentY + height};
            currentY += height + m_padding;
        }
        
        m_layoutDirty = false;
        LogPipeline("ThumbnailViewer::UpdateLayoutParams loop finished");
    } else {
        for (int i = 0; i < numPages; ++i) {
            m_thumbs[i].bounds = {contentX, currentY, contentX + m_thumbnailWidth, currentY + m_thumbs[i].height};
            currentY += m_thumbs[i].height + m_padding;
        }
    }
    
    float viewHeight = GetBounds().bottom - GetBounds().top;
    m_maxScrollY = std::max(0.0f, currentY - viewHeight);
    m_scrollY = std::min(m_scrollY, m_maxScrollY);
    LogPipeline("ThumbnailViewer::UpdateLayoutParams finish");
}

void ThumbnailViewer::Layout(const D2D1_RECT_F& bounds) {
    UIElement::Layout(bounds);
    UpdateLayoutParams();
    RequestVisibleThumbnails();
}

void ThumbnailViewer::RequestVisibleThumbnails() {
    if (!m_doc) return;
    
    float viewHeight = GetBounds().bottom - GetBounds().top;
    for (const auto& t : m_thumbs) {
        float top = t.bounds.top - m_scrollY;
        float bottom = t.bounds.bottom - m_scrollY;
        
        if (bottom > -30 && top < viewHeight + 30) { // Slight bleed
            float baseDpi = m_thumbnailWidth / m_doc->GetPageSize(t.pageIndex).width;
            UINT sysDpi = GetDpiForSystem();
            float screenDpi = sysDpi / 96.0f;
            float dpiScale = baseDpi * screenDpi;
            TileKey key;
            key.documentGeneration = m_generation;
            key.pageIndex = t.pageIndex;
            key.zoom = 1.0;
            key.dpiScale = dpiScale;
            key.tileX = 0;
            key.tileY = 0;
            key.tileWidth = static_cast<int>(m_thumbnailWidth);
            key.tileHeight = static_cast<int>(t.height);
            
            if (!m_cache.Get(key)) {
                core::models::RenderRequest task;
                task.generation = std::max((int)m_generation, core::RenderController::Instance().GetCurrentGeneration());
                
                task.viewport = {0,0,0,0};
                task.tileRect.left = 0;
                task.tileRect.top = 0;
                 task.tileRect.right = m_doc->GetPageSize(t.pageIndex).width;
                 task.tileRect.bottom = m_doc->GetPageSize(t.pageIndex).height;
                task.dpi = dpiScale;
                task.priority = 2; 
                task.pageIndex = t.pageIndex; task.darkMode = m_isDarkMode; task.renderScale = 1.0f; task.documentId = m_documentId; core::RenderController::Instance().EnqueueRequest(task);
            }
        }
    }
}

void ThumbnailViewer::OnTileReady(const core::models::RenderResult* result, Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) {
    if (!target || !result) return;
    if (result->pixelBuffer.empty() || result->width <= 0 || result->height <= 0) return;
    if (result->generation >= m_generation && result->viewport.left == 0 && result->viewport.top == 0) {
        ComPtr<ID2D1Bitmap> bitmap;
        D2D1_BITMAP_PROPERTIES props = {};
        props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        props.dpiX = 96.0f;
        props.dpiY = 96.0f;
        HRESULT hr = target->CreateBitmap(
            D2D1::SizeU(result->width, result->height),
            result->pixelBuffer.data(),
            result->stride,
            props,
            &bitmap
        );
        if (FAILED(hr) || !bitmap) return;
        
        TileKey key;
        key.documentGeneration = m_generation;
        key.pageIndex = result->pageIndex;
        key.zoom = result->renderScale;
        key.dpiScale = result->dpi;
        key.tileX = static_cast<int>(result->viewport.left);
        key.tileY = static_cast<int>(result->viewport.top);
        key.tileWidth = static_cast<int>(m_thumbnailWidth);
          float t_height = 0;
          for (const auto& th : m_thumbs) { if (th.pageIndex == result->pageIndex) { t_height = th.height; break; } }
          key.tileHeight = static_cast<int>(t_height);
        m_cache.Put(key, bitmap, result->pixelBuffer.size());
    }
}

void ThumbnailViewer::OnMouseWheel(float delta) {
    m_scrollY -= delta * 50.0f;
    m_scrollY = std::max(0.0f, std::min(m_scrollY, m_maxScrollY));
    RequestVisibleThumbnails();
}

void ThumbnailViewer::OnMouseDown(float x, float y) {
    (void)x;
    float py = y - GetBounds().top + m_scrollY;
    
    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    
    for (const auto& t : m_thumbs) {
        if (py >= t.bounds.top && py <= t.bounds.bottom) {
            int page = t.pageIndex;
            
            if (ctrl) {
                if (m_selectedPages.count(page)) {
                    m_selectedPages.erase(page);
                } else {
                    m_selectedPages.insert(page);
                    m_activePage = page;
                }
            } else if (shift && !m_selectedPages.empty()) {
                int start = m_activePage;
                int end = page;
                if (start > end) std::swap(start, end);
                m_selectedPages.clear();
                for (int i = start; i <= end; ++i) m_selectedPages.insert(i);
            } else {
                m_selectedPages.clear();
                m_selectedPages.insert(page);
                m_activePage = page;
                
            }
            
            
            
            
            
            
            
            SetCapture(nullptr); // Just concept, we use mouse moves
            return;
        }
    }
    // Clicked outside
    ClearSelection();
}

int ThumbnailViewer::GetInsertIndexFromY(float py) {
    if (m_thumbs.empty()) return 0;
    if (py < m_thumbs[0].bounds.top) return 0;
    
    for (size_t i = 0; i < m_thumbs.size(); ++i) {
        float midY = m_thumbs[i].bounds.top + m_thumbs[i].height / 2.0f;
        if (py < midY) return static_cast<int>(i);
    }
    return static_cast<int>(m_thumbs.size());
}

void ThumbnailViewer::OnMouseMove(float x, float y) {
    (void)x;
    (void)y;
}

void ThumbnailViewer::OnMouseUp(float x, float y) { (void)x; (void)y; }

void ThumbnailViewer::Render(ComPtr<ID2D1RenderTarget> target) {
    Panel::Render(target);
    if (!m_doc) return;
    
    float viewHeight = GetBounds().bottom - GetBounds().top;
    
    ComPtr<ID2D1SolidColorBrush> bgBrush, borderBrush, activeBrush, textBrush;
    target->CreateSolidColorBrush(design::Colors::SurfaceElevated, &bgBrush);
    target->CreateSolidColorBrush(design::Colors::Border, &borderBrush);
    target->CreateSolidColorBrush(design::Colors::AccentPrimary, &activeBrush); 
    target->CreateSolidColorBrush(design::Colors::TextPrimary, &textBrush);
    target->FillRectangle(m_bounds, bgBrush.Get());
    
    auto textFormat = design::FontManager::Instance().GetMetadata();
    if (textFormat) {
        textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    }
    
    for (const auto& t : m_thumbs) {
        float top = t.bounds.top - m_scrollY;
        float bottom = t.bounds.bottom - m_scrollY;
        
        if (bottom > -30 && top < viewHeight + 30) {
            D2D1_RECT_F rect = {
                GetBounds().left + t.bounds.left,
                GetBounds().top + top,
                GetBounds().left + t.bounds.right,
                GetBounds().top + bottom
            };
            
            if (m_selectedPages.count(t.pageIndex)) {
                D2D1_RECT_F ringRect = { rect.left - 4, rect.top - 4, rect.right + 4, rect.bottom + 4 };
                D2D1_ROUNDED_RECT ringRoundedRect = { ringRect, 6.0f, 6.0f };
                target->DrawRoundedRectangle(ringRoundedRect, activeBrush.Get(), 2.0f);
            }
            
            TileKey key;
            key.documentGeneration = m_generation;
            key.pageIndex = t.pageIndex;
            key.zoom = 1.0;
            float baseDpi = m_thumbnailWidth / m_doc->GetPageSize(t.pageIndex).width;
            UINT sysDpi = GetDpiForSystem();
            float screenDpi = sysDpi / 96.0f;
            key.dpiScale = baseDpi * screenDpi;
            key.tileX = 0;
            key.tileY = 0;
            key.tileWidth = static_cast<int>(m_thumbnailWidth);
            key.tileHeight = static_cast<int>(t.height);
            
            auto bmp = m_cache.Get(key);
            
            ComPtr<ID2D1RoundedRectangleGeometry> roundedGeom;
            D2D1_ROUNDED_RECT roundedRect = { rect, 4.0f, 4.0f };
            extern ID2D1Factory1* g_d2dFactory; // Note: Or use GraphicsDevice::Instance().GetD2DFactory()
            
            ComPtr<ID2D1Factory> factory = GraphicsDevice::Instance().GetD2DFactory();
            if (SUCCEEDED(factory->CreateRoundedRectangleGeometry(roundedRect, &roundedGeom))) {
                ComPtr<ID2D1Layer> layer;
                target->CreateLayer(&layer);
                target->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), roundedGeom.Get()), layer.Get());
                
                if (bmp) {
                    target->DrawBitmap(bmp.Get(), rect);
                } else {
                    target->FillRectangle(rect, borderBrush.Get());
                }
                
                target->PopLayer();
                target->DrawRoundedRectangle(roundedRect, borderBrush.Get(), 1.0f);
            } else {
                if (bmp) target->DrawBitmap(bmp.Get(), rect);
                else target->FillRectangle(rect, borderBrush.Get());
                target->DrawRectangle(rect, borderBrush.Get(), 1.0f);
            }
            
            // Draw page number
            std::wstring pageNum = std::to_wstring(t.pageIndex + 1);
            D2D1_RECT_F textRect = { rect.left, rect.bottom + 4.0f, rect.right, rect.bottom + 24.0f };
            target->DrawTextW(pageNum.c_str(), static_cast<UINT32>(pageNum.length()), textFormat, textRect, textBrush.Get());
        }
    }
    
}

void ThumbnailViewer::SetDarkMode(bool dark) {
    if (m_isDarkMode != dark) {
        m_isDarkMode = dark;
        m_generation++;
        m_cache.InvalidateAll();
        RequestVisibleThumbnails();
    }
}

} // namespace components
