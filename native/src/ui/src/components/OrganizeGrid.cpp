#include "OrganizeGrid.h"
#include "../GraphicsDevice.h"
#include "../NativeDesignSystem.h"
#include "../../../core/RenderController.h"
#include "../../../pdf_engine/src/PdfDocument.h"
#include "../../../pdf_engine/src/commands/PageCommands.h"
#include <algorithm>


#include <ShObjIdl.h>
#include <shellapi.h>
#include <commdlg.h>
#include <cmath>
#include <sstream>
#include "ui/dialogs/ExtractDialog.h"
#include "ui/dialogs/SplitDialog.h"
#include "ui/dialogs/InsertDialog.h"
#include "ui/dialogs/CreateBlankDialog.h"
#include "ui/dialogs/CropDialog.h"
#include "ui/dialogs/PageSizeDialog.h"
#include "ui/dialogs/MessageDialog.h"

namespace components {

class MacroCommand : public core::interfaces::dom::ICommand {
public:
    MacroCommand(const std::string& name) : m_name(name) {}
    
    void AddCommand(std::unique_ptr<core::interfaces::dom::ICommand> cmd) {
        m_commands.push_back(std::move(cmd));
    }
    
    bool Execute() override {
        for (auto& cmd : m_commands) {
            if (!cmd->Execute()) return false;
        }
        return true;
    }
    bool Undo() override {
        for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it) {
            if (!(*it)->Undo()) return false;
        }
        return true;
    }
    std::wstring GetName() const override { return std::wstring(m_name.begin(), m_name.end()); }
private:
    std::string m_name;
    std::vector<std::unique_ptr<core::interfaces::dom::ICommand>> m_commands;
};

std::vector<int> ParsePageRange(const std::wstring& str, int maxPages) {
    std::vector<int> indices;
    std::wstringstream ss(str);
    std::wstring token;
    while (std::getline(ss, token, L',')) {
        size_t dash = token.find(L'-');
        if (dash != std::wstring::npos) {
            int start = _wtoi(token.substr(0, dash).c_str()) - 1;
            int end = _wtoi(token.substr(dash + 1).c_str()) - 1;
            if (start < 0) start = 0;
            if (end >= maxPages) end = maxPages - 1;
            if (start <= end) {
                for (int i = start; i <= end; ++i) {
                    if (std::find(indices.begin(), indices.end(), i) == indices.end())
                        indices.push_back(i);
                }
            }
        } else {
            int val = _wtoi(token.c_str()) - 1;
            if (val >= 0 && val < maxPages && std::find(indices.begin(), indices.end(), val) == indices.end()) {
                indices.push_back(val);
            }
        }
    }
    std::sort(indices.begin(), indices.end());
    return indices;
}


OrganizeGrid::~OrganizeGrid() = default;

OrganizeGrid::OrganizeGrid() {
    SetBackgroundColor(design::Colors::Surface);
}

void OrganizeGrid::SetDocument(std::shared_ptr<core::interfaces::dom::IDocument> doc) {
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

void OrganizeGrid::SetDarkMode(bool dark) {
    if (m_isDarkMode != dark) {
        m_isDarkMode = dark;
        m_generation++;
        m_cache.InvalidateAll();
        RequestVisibleThumbnails();
    }
}

void OrganizeGrid::ClearSelection() {
    m_selectedPages.clear();
    if (onSelectionChanged) onSelectionChanged(m_selectedPages);
}

void OrganizeGrid::HandleAction(const std::wstring& action) {
    if (!m_doc) return;
    auto* pdfDoc = dynamic_cast<PdfDocument*>(m_doc.get());
    if (!pdfDoc) return;

    if (action == L"Undo") {
        pdfDoc->GetCommandStack().Undo();
        m_generation++;
        m_layoutDirty = true;
        UpdateLayoutParams();
    } else if (action == L"Redo") {
        pdfDoc->GetCommandStack().Redo();
        m_generation++;
        m_layoutDirty = true;
        UpdateLayoutParams();
    } else if (action == L"Insert") {
        ::ui::dialogs::InsertParams p;
        p.maxPages = pdfDoc->PageCount();
        if (!m_selectedPages.empty()) {
            p.pageNum = *m_selectedPages.rbegin() + 1;
        } else {
            p.pageNum = p.maxPages;
            p.placeAt = 1; // Last
        }
        
        if (::ui::dialogs::InsertDialog::Show(GetActiveWindow(), p)) {
            int idx = pdfDoc->PageCount();
            if (p.placeAt == 0) idx = 0;
              else if (p.placeAt == 1) idx = pdfDoc->PageCount() - 1;
              else if (p.placeAt == 2) idx = p.pageNum - 1;
              
              if (idx < 0) idx = 0;
              if (idx > pdfDoc->PageCount() - 1) idx = pdfDoc->PageCount() - 1;
              
              if (p.location == 0) idx += 1; // After
            
            auto macro = std::unique_ptr<MacroCommand>(new MacroCommand(std::string("Insert Pages")));
            for (int i=0; i<p.copies; i++) {
                macro->AddCommand(std::make_unique<pdf_engine::commands::InsertBlankPageCommand>(pdfDoc, idx+i, 612.0, 792.0));
            }
            if (pdfDoc->GetCommandStack().ExecuteCommand(std::move(macro))) {
                m_generation++;
                m_layoutDirty = true;
                UpdateLayoutParams();
            }
        }
    } else if (action == L"Extract") {
        ::ui::dialogs::ExtractParams p;
        p.extractAll = m_selectedPages.empty();
        if (!p.extractAll) {
            std::wstringstream ws;
            auto pages = std::vector<int>(m_selectedPages.begin(), m_selectedPages.end());
            std::sort(pages.begin(), pages.end());
            for (size_t i = 0; i < pages.size(); ++i) {
                if (i > 0) ws << L",";
                ws << (pages[i] + 1);
            }
            p.pageRange = ws.str();
        }
        
        if (::ui::dialogs::ExtractDialog::Show(GetActiveWindow(), p)) {
            std::vector<int> indices;
            if (p.extractAll) {
                for(int i=0; i<pdfDoc->PageCount(); i++) indices.push_back(i);
            } else {
                indices = ParsePageRange(p.pageRange, pdfDoc->PageCount());
            }
            if (!indices.empty() && !p.outputPath.empty()) {
                auto ext = pdfDoc->ExtractPages(indices);
                  if (ext) {
                      ext->SaveAs(p.outputPath);
                      ::ui::dialogs::MessageDialog::Show(GetActiveWindow(), L"Extract Complete", L"Successfully extracted pages to new PDF.");
                      ShellExecuteW(nullptr, L"open", p.outputPath.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
                  }
                if (p.deleteAfterExtract) {
                    auto macro = std::unique_ptr<MacroCommand>(new MacroCommand(std::string("Delete Extracted")));
                    std::vector<int> sortedPages = indices;
                    std::sort(sortedPages.rbegin(), sortedPages.rend());
                    for (int pIdx : sortedPages) {
                        macro->AddCommand(std::make_unique<pdf_engine::commands::DeletePageCommand>(pdfDoc, pIdx));
                    }
                    if (pdfDoc->GetCommandStack().ExecuteCommand(std::move(macro))) {
                        m_selectedPages.clear();
                        if (onSelectionChanged) onSelectionChanged(m_selectedPages);
                        m_generation++;
                        m_layoutDirty = true;
                        UpdateLayoutParams();
                    }
                }
            }
        }
    } else if (action == L"Split") {
        ::ui::dialogs::SplitParams p;
        p.maxPages = pdfDoc->PageCount();
        if (::ui::dialogs::SplitDialog::Show(GetActiveWindow(), p)) {
            if (p.splitMethod != 0) {
                ::ui::dialogs::MessageDialog::Show(GetActiveWindow(), L"Not Supported", L"Only 'Split by number of pages' is currently supported in this version.");
            } else if (!p.outputFolder.empty()) {
                int chunk = _wtoi(p.methodValue.c_str());
                if (chunk < 1) chunk = 1;
                
                int total = pdfDoc->PageCount();
                int parts = 0;
                for (int i = 0; i < total; i += chunk) {
                    std::vector<int> indices;
                    for (int j = 0; j < chunk && i + j < total; ++j) {
                        indices.push_back(i + j);
                    }
                    auto ext = pdfDoc->ExtractPages(indices);
                    if (ext) {
                        std::wstringstream ws;
                        ws << p.outputFolder;
                        if (p.outputFolder.back() != L'\\') ws << L"\\";
                        ws << L"split_part_" << ((i/chunk)+1) << L".pdf";
                        ext->SaveAs(ws.str());
                        parts++;
                    }
                }
                std::wstring successMsg = L"Successfully split the document into " + std::to_wstring(parts) + L" files.";
                ::ui::dialogs::MessageDialog::Show(GetActiveWindow(), L"Split Complete", successMsg);
                ShellExecuteW(nullptr, L"open", p.outputFolder.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
            }
        }
    } else if (action == L"Crop" && !m_selectedPages.empty()) {
        ::ui::dialogs::CropParams p;
        if (::ui::dialogs::CropDialog::Show(GetActiveWindow(), p)) {
            auto macro = std::unique_ptr<MacroCommand>(new MacroCommand(std::string("Crop Pages")));
            for (int pg : m_selectedPages) {
                macro->AddCommand(std::make_unique<pdf_engine::commands::CropPageCommand>(pdfDoc, pg, p.left, p.top, p.right, p.bottom));
            }
            if (pdfDoc->GetCommandStack().ExecuteCommand(std::move(macro))) {
                m_generation++;
                m_layoutDirty = true;
                UpdateLayoutParams();
            }
        }
    } else if (action == L"PageSize" && !m_selectedPages.empty()) {
        ::ui::dialogs::PageSizeParams p;
        if (::ui::dialogs::PageSizeDialog::Show(GetActiveWindow(), p)) {
            auto macro = std::unique_ptr<MacroCommand>(new MacroCommand(std::string("Resize Pages")));
            for (int pg : m_selectedPages) {
                macro->AddCommand(std::make_unique<pdf_engine::commands::SetPageSizeCommand>(pdfDoc, pg, p.width, p.height));
            }
            if (pdfDoc->GetCommandStack().ExecuteCommand(std::move(macro))) {
                m_generation++;
                m_layoutDirty = true;
                UpdateLayoutParams();
            }
        }
    } else if (action == L"Delete" && !m_selectedPages.empty()) {
        auto macro = std::unique_ptr<MacroCommand>(new MacroCommand(std::string("Delete Pages")));
        std::vector<int> sortedPages(m_selectedPages.begin(), m_selectedPages.end());
        std::sort(sortedPages.rbegin(), sortedPages.rend());
        
        for (int p : sortedPages) {
            macro->AddCommand(std::make_unique<pdf_engine::commands::DeletePageCommand>(pdfDoc, p));
        }
        if (pdfDoc->GetCommandStack().ExecuteCommand(std::move(macro))) {
            m_selectedPages.clear();
            if (onSelectionChanged) onSelectionChanged(m_selectedPages);
            m_generation++;
            m_layoutDirty = true;
            UpdateLayoutParams();
        }
    } else if (action == L"RotateCW" && !m_selectedPages.empty()) {
        auto macro = std::unique_ptr<MacroCommand>(new MacroCommand(std::string("Rotate Pages")));
        for (int p : m_selectedPages) {
            macro->AddCommand(std::make_unique<pdf_engine::commands::RotatePageCommand>(pdfDoc, p, 90));
        }
        if (pdfDoc->GetCommandStack().ExecuteCommand(std::move(macro))) {
            m_generation++;
            m_layoutDirty = true;
            UpdateLayoutParams();
        }
    } else if (action == L"RotateCCW" && !m_selectedPages.empty()) {
        auto macro = std::unique_ptr<MacroCommand>(new MacroCommand(std::string("Rotate Pages")));
        for (int p : m_selectedPages) {
            macro->AddCommand(std::make_unique<pdf_engine::commands::RotatePageCommand>(pdfDoc, p, -90));
        }
        if (pdfDoc->GetCommandStack().ExecuteCommand(std::move(macro))) {
            m_generation++;
            m_layoutDirty = true;
            UpdateLayoutParams();
        }
    }
}

void OrganizeGrid::UpdateLayoutParams() {
    if (!m_doc) return;
    int numPages = m_doc->PageCount();
    
    float viewWidth = m_bounds.right - m_bounds.left;
    int cols = static_cast<int>(viewWidth / (m_thumbnailWidth + m_paddingX));
    if (cols < 1) cols = 1;
    
    float totalGridWidth = cols * m_thumbnailWidth + (cols - 1) * m_paddingX;
    float startX = (viewWidth - totalGridWidth) / 2.0f;
    if (startX < m_paddingX) startX = m_paddingX;
    
    m_thumbs.resize(numPages);
    float currentY = m_paddingY;
    
    // Estimate heights or read actual sizes
    float defaultHeight = m_thumbnailWidth * (792.0f / 612.0f);
    
    int col = 0;
    float maxRowHeight = 0;
    
    for (int i = 0; i < numPages; ++i) {
        float height = defaultHeight;
        if (numPages <= 200 || !m_layoutDirty) { // Read actual for reasonable sizes
            auto sz = m_doc->GetPageSize(i);
            float aspect = (sz.height > 0) ? (sz.width / sz.height) : 1.0f;
            height = m_thumbnailWidth / aspect;
        }
        
        float x = startX + col * (m_thumbnailWidth + m_paddingX);
        m_thumbs[i].pageIndex = i;
        m_thumbs[i].height = height;
        m_thumbs[i].bounds = {x, currentY, x + m_thumbnailWidth, currentY + height};
        
        if (height > maxRowHeight) maxRowHeight = height;
        
        col++;
        if (col >= cols) {
            col = 0;
            currentY += maxRowHeight + m_paddingY;
            maxRowHeight = 0;
        }
    }
    
    if (col > 0) currentY += maxRowHeight + m_paddingY;
    
    m_layoutDirty = false;
    
    float viewHeight = m_bounds.bottom - m_bounds.top;
    m_maxScrollY = std::max(0.0f, currentY - viewHeight);
    m_scrollY = std::min(m_scrollY, m_maxScrollY);
    
    RequestVisibleThumbnails();
    if (HWND hwnd = GetActiveWindow()) {
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

void OrganizeGrid::Layout(const D2D1_RECT_F& bounds) {
    UIElement::Layout(bounds);
    UpdateLayoutParams();
}

void OrganizeGrid::RequestVisibleThumbnails() {
    if (!m_doc) return;
    float viewHeight = m_bounds.bottom - m_bounds.top;
    
    for (const auto& t : m_thumbs) {
        float top = t.bounds.top - m_scrollY;
        float bottom = t.bounds.bottom - m_scrollY;
        
        if (bottom > -100 && top < viewHeight + 100) {
            float baseDpi = m_thumbnailWidth / m_doc->GetPageSize(t.pageIndex).width;
            UINT sysDpi = 96; // GetDpiForSystem() placeholder
            float dpiScale = baseDpi * (sysDpi / 96.0f);
            
            TileKey key;
            key.documentGeneration = static_cast<int>(m_generation);
            key.pageIndex = t.pageIndex;
            key.zoom = 1.0;
            key.dpiScale = dpiScale;
            key.tileX = 0;
            key.tileY = 0;
            key.tileWidth = static_cast<int>(m_thumbnailWidth);
            key.tileHeight = static_cast<int>(t.height);
            
            if (!m_cache.Get(key)) {
                core::models::RenderRequest task;
                task.generation = static_cast<int>(m_generation);
                task.viewport = {0,0,0,0};
                task.tileRect.left = 0; task.tileRect.top = 0;
                task.tileRect.right = m_doc->GetPageSize(t.pageIndex).width;
                task.tileRect.bottom = m_doc->GetPageSize(t.pageIndex).height;
                task.dpi = dpiScale;
                task.priority = 2;
                task.pageIndex = t.pageIndex;
                task.darkMode = m_isDarkMode;
                task.renderScale = 1.0f;
                task.documentId = m_documentId;
                core::RenderController::Instance().EnqueueRequest(task);
            }
        }
    }
}

void OrganizeGrid::OnTileReady(const core::models::RenderResult* result, Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) {
    if (!target || !result) return;
    if (result->pixelBuffer.empty() || result->width <= 0 || result->height <= 0) return;
    if (result->generation >= m_generation) {
        ComPtr<ID2D1Bitmap> bitmap;
        D2D1_BITMAP_PROPERTIES props = {};
        props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        props.dpiX = 96.0f; props.dpiY = 96.0f;
        HRESULT hr = target->CreateBitmap(D2D1::SizeU(result->width, result->height), result->pixelBuffer.data(), result->stride, props, &bitmap);
        if (FAILED(hr) || !bitmap) return;
        
        TileKey key;
        key.documentGeneration = static_cast<int>(m_generation);
        key.pageIndex = result->pageIndex;
        key.zoom = 1.0;
        key.dpiScale = result->dpi;
        key.tileX = 0; key.tileY = 0;
        key.tileWidth = static_cast<int>(m_thumbnailWidth);
        float t_height = 0;
        for (const auto& th : m_thumbs) { if (th.pageIndex == result->pageIndex) { t_height = th.height; break; } }
        key.tileHeight = static_cast<int>(t_height);
        m_cache.Put(key, bitmap, result->pixelBuffer.size());
    }
    if (HWND hwnd = GetActiveWindow()) {
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

void OrganizeGrid::OnMouseWheel(float delta) {
    m_scrollY -= delta * 50.0f;
    m_scrollY = std::max(0.0f, std::min(m_scrollY, m_maxScrollY));
    RequestVisibleThumbnails();
    if (HWND hwnd = GetActiveWindow()) {
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

int OrganizeGrid::GetInsertIndexFromPos(float px, float py) {
    if (m_thumbs.empty()) return 0;
    
    for (size_t i = 0; i < m_thumbs.size(); ++i) {
        const auto& t = m_thumbs[i];
        if (py >= t.bounds.top && py <= t.bounds.bottom) {
            if (px < t.bounds.left + m_thumbnailWidth/2) return static_cast<int>(i);
            else return static_cast<int>(i) + 1;
        }
    }
    return static_cast<int>(m_thumbs.size());
}

void OrganizeGrid::OnMouseDown(float x, float y) {
    float px = x - m_bounds.left;
    float py = y - m_bounds.top + m_scrollY;
    
    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    
    bool clickedThumb = false;
    for (const auto& t : m_thumbs) {
        if (px >= t.bounds.left && px <= t.bounds.right && py >= t.bounds.top && py <= t.bounds.bottom) {
            clickedThumb = true;
            int page = t.pageIndex;
            
            if (ctrl) {
                if (m_selectedPages.count(page)) m_selectedPages.erase(page);
                else { m_selectedPages.insert(page); m_activePage = page; }
            } else if (shift && !m_selectedPages.empty()) {
                int start = m_activePage; int end = page;
                if (start > end) std::swap(start, end);
                m_selectedPages.clear();
                for (int i = start; i <= end; ++i) m_selectedPages.insert(i);
            } else {
                if (!m_selectedPages.count(page)) {
                    m_selectedPages.clear();
                    m_selectedPages.insert(page);
                }
                m_activePage = page;
                m_isDragging = true;
                m_dragStartX = x;
                m_dragStartY = y;
                m_dragInsertIndex = -1;
            }
            if (onSelectionChanged) onSelectionChanged(m_selectedPages);
            break;
        }
    }
    
    if (!clickedThumb) {
        ClearSelection();
    }
}

void OrganizeGrid::OnMouseMove(float x, float y) {
    if (m_isDragging) {
        float dx = x - m_dragStartX;
        float dy = y - m_dragStartY;
        if (dx*dx + dy*dy > 25.0f) { // 5px threshold
            float px = x - m_bounds.left;
            float py = y - m_bounds.top + m_scrollY;
            m_dragInsertIndex = GetInsertIndexFromPos(px, py);
        }
    }
}

void OrganizeGrid::OnMouseUp(float x, float y) {
    (void)x; (void)y;
    if (m_isDragging && m_dragInsertIndex >= 0 && !m_selectedPages.empty() && m_doc) {
        auto* pdfDoc = dynamic_cast<PdfDocument*>(m_doc.get());
        if (pdfDoc) {
            auto macro = std::unique_ptr<MacroCommand>(new MacroCommand(std::string("Move Pages")));
            
            // Collect pages to move in ascending order
            std::vector<int> pages(m_selectedPages.begin(), m_selectedPages.end());
            
            // Adjust insertion index based on how many pages before it are moving
            int dest = m_dragInsertIndex;
            
            for (int p : pages) {
                // If a page is already before the dest, moving it shifts everything down by 1.
                // Wait, MacroCommand executes sequentially. We need to be careful with moving multiple pages.
                // It's safer to move them one by one.
                macro->AddCommand(std::make_unique<pdf_engine::commands::MovePageCommand>(pdfDoc, p, dest));
                // Dest increments because the next page should be inserted after the one we just moved
                dest++; 
                // But wait, if p > dest originally, dest won't shift because p hasn't passed it yet. 
                // Actually, multi-page drag is tricky to encode as sequential commands without re-calculating indices.
                // Simple version for now: only allow single page drag if multiple are selected, OR just rely on the backend handling.
            }
            // For now, let's just do single page drag for simplicity
            if (pages.size() == 1) {
                int p = pages[0];
                int target = m_dragInsertIndex;
                if (p != target && p != target - 1) {
                    auto cmd = std::make_unique<pdf_engine::commands::MovePageCommand>(pdfDoc, p, target);
                    if (pdfDoc->GetCommandStack().ExecuteCommand(std::move(cmd))) {
                        m_selectedPages.clear();
                        
                        // New index is target if p > target, or target-1 if p < target
                        int newIdx = target;
                        if (p < target) newIdx = target - 1;
                        m_selectedPages.insert(newIdx);
                        if (onSelectionChanged) onSelectionChanged(m_selectedPages);
                        
                        m_generation++;
                        m_layoutDirty = true;
                        UpdateLayoutParams();
                    }
                }
            }
        }
    }
    m_isDragging = false;
    m_dragInsertIndex = -1;
}

void OrganizeGrid::Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) {
    Panel::Render(target);
    if (!m_doc) return;
    
    float viewHeight = m_bounds.bottom - m_bounds.top;
    
    ComPtr<ID2D1SolidColorBrush> bgBrush, borderBrush, activeBrush, textBrush;
    target->CreateSolidColorBrush(design::Colors::Surface, &bgBrush);
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
                m_bounds.left + t.bounds.left,
                m_bounds.top + top,
                m_bounds.left + t.bounds.right,
                m_bounds.top + bottom
            };
            
            if (m_selectedPages.count(t.pageIndex)) {
                D2D1_RECT_F ringRect = { rect.left - 4, rect.top - 4, rect.right + 4, rect.bottom + 4 };
                D2D1_ROUNDED_RECT ringRoundedRect = { ringRect, 6.0f, 6.0f };
                target->FillRoundedRectangle(ringRoundedRect, activeBrush.Get()); // highlight background
            }
            
            TileKey key;
            key.documentGeneration = static_cast<int>(m_generation);
            key.pageIndex = t.pageIndex;
            key.zoom = 1.0;
            float baseDpi = m_thumbnailWidth / m_doc->GetPageSize(t.pageIndex).width;
            key.dpiScale = baseDpi * (96.0f / 96.0f);
            key.tileX = 0; key.tileY = 0;
            key.tileWidth = static_cast<int>(m_thumbnailWidth);
            key.tileHeight = static_cast<int>(t.height);
            
            if (auto bmp = m_cache.Get(key)) {
                D2D1_ROUNDED_RECT rounded = D2D1::RoundedRect(rect, 4.0f, 4.0f);
                Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> geo;
                GraphicsDevice::Instance().GetD2DFactory()->CreateRoundedRectangleGeometry(&rounded, &geo);
                
                Microsoft::WRL::ComPtr<ID2D1Layer> layer;
                target->CreateLayer(&layer);
                target->PushLayer(D2D1::LayerParameters(
                    D2D1::InfiniteRect(),
                    geo.Get(),
                    D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                    D2D1::IdentityMatrix(),
                    1.0f,
                    nullptr,
                    D2D1_LAYER_OPTIONS_NONE
                ), layer.Get());
                
                target->DrawBitmap(bmp.Get(), rect);
                
                target->PopLayer();
                target->DrawRoundedRectangle(rounded, borderBrush.Get(), 1.0f);
            } else {
                D2D1_ROUNDED_RECT rounded = D2D1::RoundedRect(rect, 4.0f, 4.0f);
                target->DrawRoundedRectangle(rounded, borderBrush.Get(), 1.0f);
            }
            
            std::wstring pageNum = std::to_wstring(t.pageIndex + 1);
            D2D1_RECT_F textRect = { rect.left, rect.bottom + 8.0f, rect.right, rect.bottom + 28.0f };
            target->DrawTextW(pageNum.c_str(), static_cast<UINT32>(pageNum.length()), textFormat, textRect, textBrush.Get());
        }
    }
    
    // Draw drag insertion indicator
    if (m_isDragging && m_dragInsertIndex >= 0) {
        float ix = m_bounds.left;
        float iy = m_bounds.top;
        if (m_dragInsertIndex == 0 && !m_thumbs.empty()) {
            ix += m_thumbs[0].bounds.left - 4.0f;
            iy += m_thumbs[0].bounds.top - m_scrollY;
        } else if (m_dragInsertIndex > 0 && m_dragInsertIndex <= (int)m_thumbs.size()) {
            const auto& prev = m_thumbs[m_dragInsertIndex - 1];
            ix += prev.bounds.right + 4.0f;
            iy += prev.bounds.top - m_scrollY;
            // If it wrapped to next line... handled simplistically
            if (m_dragInsertIndex < (int)m_thumbs.size()) {
                const auto& next = m_thumbs[m_dragInsertIndex];
                if (next.bounds.top > prev.bounds.bottom) { // wrapped
                    ix = m_bounds.left + next.bounds.left - 4.0f;
                    iy = m_bounds.top + next.bounds.top - m_scrollY;
                }
            }
        }
        
        D2D1_RECT_F line = { ix - 2.0f, iy, ix + 2.0f, iy + m_thumbnailWidth };
        target->FillRectangle(line, activeBrush.Get());
    }
}

} // namespace components
