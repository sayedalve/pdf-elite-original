#include "IconSystem.h"
#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>

namespace PdfElite {

static std::unordered_map<IconId, Microsoft::WRL::ComPtr<ID2D1SvgDocument>> g_svgCache;
static ID2D1DeviceContext5* g_lastDc = nullptr;

static std::wstring GetIconFileName(IconId id) {
    switch (id) {
        case IconId::Home: return L"resources/icons/home.svg";
        case IconId::Comment: return L"resources/icons/comment.svg";
        case IconId::Edit: return L"resources/icons/edit.svg";
        case IconId::Convert: return L"resources/icons/convert.svg";
        case IconId::View: return L"resources/icons/view.svg";
        case IconId::Organize: return L"resources/icons/organize.svg";
        case IconId::Tools: return L"resources/icons/tools.svg";
        case IconId::Form: return L"resources/icons/form.svg";
        case IconId::RecentFiles: return L"resources/icons/recent-files.svg";
        case IconId::StarredFiles: return L"resources/icons/star.svg";
        case IconId::RecentFolders: return L"resources/icons/folder.svg";
        case IconId::Spaces: return L"resources/icons/spaces.svg";
        case IconId::Cloud: return L"resources/icons/cloud.svg";
        case IconId::Agreement: return L"resources/icons/agreement.svg";
        case IconId::Receipt: return L"resources/icons/receipt.svg";
        case IconId::OpenFolder: return L"resources/icons/open-folder.svg";
        case IconId::CreateDoc: return L"resources/icons/add-file.svg";
        case IconId::Plus: return L"resources/icons/plus.svg";
        case IconId::Close: return L"resources/icons/cancel.svg";
        case IconId::Search: return L"resources/icons/search.svg";
        case IconId::List: return L"resources/icons/list.svg";
        case IconId::Grid: return L"resources/icons/grid.svg";
        case IconId::Filter: return L"resources/icons/filter.svg";
        case IconId::Refresh: return L"resources/icons/refresh.svg";
        case IconId::Undo: return L"resources/icons/undo.svg";
        case IconId::Redo: return L"resources/icons/redo.svg";
        case IconId::ZoomOut: return L"resources/icons/zoom-out.svg";
        case IconId::ZoomIn: return L"resources/icons/zoom-in.svg";
        case IconId::Hand: return L"resources/icons/hand.svg";
        case IconId::RectSelect: return L"resources/icons/rectangle.svg";
        case IconId::EditAll: return L"resources/icons/edit-all.svg";
        case IconId::AddText: return L"resources/icons/text.svg";
        case IconId::AddLink: return L"resources/icons/link.svg";
        case IconId::Image: return L"resources/icons/image.svg";
        case IconId::Watermark: return L"resources/icons/watermark.svg";
        case IconId::Background: return L"resources/icons/background.svg";
        case IconId::Ocr: return L"resources/icons/ocr.svg";
        case IconId::Crop: return L"resources/icons/crop.svg";
        case IconId::Combine: return L"resources/icons/combine.svg";
        case IconId::Compress: return L"resources/icons/compress.svg";
        case IconId::Extract: return L"resources/icons/extract.svg";
        case IconId::Split: return L"resources/icons/split.svg";
        case IconId::Insert: return L"resources/icons/insert.svg";
        case IconId::RotateLeft: return L"resources/icons/rotate-left.svg";
        case IconId::RotateRight: return L"resources/icons/rotate-right.svg";
        case IconId::Trash: return L"resources/icons/trash.svg";
        case IconId::Highlight: return L"resources/icons/highlight.svg";
        case IconId::HighlightArea: return L"resources/icons/rectangle.svg";
        case IconId::Pencil: return L"resources/icons/pencil.svg";
        case IconId::Eraser: return L"resources/icons/eraser.svg";
        case IconId::Underline: return L"resources/icons/underline.svg";
        case IconId::Strikethrough: return L"resources/icons/strikethrough.svg";
        case IconId::Text: return L"resources/icons/text.svg";
        case IconId::TextBox: return L"resources/icons/rectangle.svg";
        case IconId::Rectangle: return L"resources/icons/rectangle.svg";
        case IconId::Stamp: return L"resources/icons/stamp.svg";
        case IconId::Signature: return L"resources/icons/signature.svg";
        case IconId::Attachment: return L"resources/icons/attachment.svg";
        case IconId::Save: return L"resources/icons/save.svg";
        case IconId::Print: return L"resources/icons/print.svg";
        case IconId::CloudUpload: return L"resources/icons/cloud-upload.svg";
        case IconId::Upload: return L"resources/icons/upload.svg";
        case IconId::Thumbnails: return L"resources/icons/thumbnails.svg";
        case IconId::Bookmark: return L"resources/icons/bookmark.svg";
        case IconId::CommentBubble: return L"resources/icons/chat.svg";
        case IconId::Fields: return L"resources/icons/fields.svg";
        case IconId::More: return L"resources/icons/more.svg";
        case IconId::Up: return L"resources/icons/arrow-up.svg";
        case IconId::Down: return L"resources/icons/arrow-down.svg";
        case IconId::Fit: return L"resources/icons/fit.svg";
        case IconId::ZoomPercent: return L"resources/icons/search.svg";
        case IconId::ToolEdit: return L"resources/icons/edit.svg";
        case IconId::ToolConvert: return L"resources/icons/convert.svg";
        case IconId::ToolOcr: return L"resources/icons/ocr.svg";
        case IconId::ToolComment: return L"resources/icons/comment.svg";
        case IconId::ToolTranslate: return L"resources/icons/translate.svg";
        case IconId::ToolCombine: return L"resources/icons/combine.svg";
        case IconId::ToolCompress: return L"resources/icons/compress.svg";
        case IconId::ToolBatch: return L"resources/icons/batch.svg";
        case IconId::Menu: return L"resources/icons/list.svg";
        case IconId::Avatar: return L"resources/icons/home.svg";
        default: return L"resources/icons/home.svg";
    }
}

static void ChangeColorRecurse(ID2D1SvgElement* element, const D2D1_COLOR_F& color) {
    if (!element) return;
    element->SetAttributeValue(L"stroke", D2D1_SVG_ATTRIBUTE_POD_TYPE_COLOR, &color, sizeof(color));
    
    Microsoft::WRL::ComPtr<ID2D1SvgElement> child;
    element->GetFirstChild(&child);
    while (child) {
        ChangeColorRecurse(child.Get(), color);
        Microsoft::WRL::ComPtr<ID2D1SvgElement> next;
        element->GetNextChild(child.Get(), &next);
        child = next;
    }
}

void IconSystem::ClearCache() {
    g_svgCache.clear();
    g_lastDc = nullptr;
}

void IconSystem::DrawIcon(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect, IconId id, ID2D1SolidColorBrush* brush, float stroke) {
    Microsoft::WRL::ComPtr<ID2D1DeviceContext5> dc;
    HRESULT hr = rt->QueryInterface(IID_PPV_ARGS(&dc));
    if (FAILED(hr)) return;
    
    if (g_lastDc != dc.Get()) {
        g_svgCache.clear();
        g_lastDc = dc.Get();
    }
    
    if (g_svgCache.find(id) == g_svgCache.end()) {
        Microsoft::WRL::ComPtr<IStream> stream;
        hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
        
        // Read file
        std::wstring path = GetIconFileName(id);
        std::ifstream file(path.c_str(), std::ios::binary);
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string svg = buffer.str();
            
            ULONG written;
            stream->Write(svg.data(), svg.size(), &written);
            LARGE_INTEGER zero = {0};
            stream->Seek(zero, STREAM_SEEK_SET, NULL);
            
            Microsoft::WRL::ComPtr<ID2D1SvgDocument> doc;
            if (SUCCEEDED(dc->CreateSvgDocument(stream.Get(), D2D1::SizeF(24.0f, 24.0f), &doc))) {
                g_svgCache[id] = doc;
            }
        }
    }
    
    if (g_svgCache.find(id) != g_svgCache.end()) {
        auto doc = g_svgCache[id];
        
        if (brush) {
            Microsoft::WRL::ComPtr<ID2D1SvgElement> root;
            doc->GetRoot(&root);
            ChangeColorRecurse(root.Get(), brush->GetColor());
        }
        
        float w = rect.right - rect.left;
        float h = rect.bottom - rect.top;
        
        D2D1_MATRIX_3X2_F oldTransform;
        rt->GetTransform(&oldTransform);
        
        D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F::Scale(w / 24.0f, h / 24.0f) * D2D1::Matrix3x2F::Translation(rect.left, rect.top);
        rt->SetTransform(transform * oldTransform);
        
        dc->DrawSvgDocument(doc.Get());
        
        rt->SetTransform(oldTransform);
    }
}

} // namespace PdfElite
