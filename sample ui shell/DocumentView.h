// DocumentView.h - Document workspace: ThumbnailPanel + PdfCanvas + DocumentControls
#pragma once
#include "Theme.h"
#include "LayoutManager.h"
#include "PdfCanvas.h"
#include "PdfDocument.h"
#include "CommandManager.h"
#include <vector>

namespace PdfElite {

enum class DocViewMode { SinglePage, ThumbnailGrid };

struct ThumbInfo {
    int pageNumber;
    std::wstring title;
    bool selected;
};

class DocumentView {
public:
    HRESULT Initialize(ID2D1RenderTarget* rt, IDWriteFactory* dwFactory, Theme* theme, CommandManager* cmdMgr);
    void Release();

    void Render(ID2D1RenderTarget* rt, const Layout& layout, PdfDocument* doc, DocViewMode mode, int selectedThumb);

    bool HitTestThumbnail(int x, int y, const Layout& layout, int& outIndex);

private:
    void RenderThumbnailGrid(ID2D1RenderTarget* rt, const Layout& layout);

    Theme* m_theme = nullptr;
    CommandManager* m_cmdMgr = nullptr;
    PdfCanvas m_canvas;

    std::vector<ThumbInfo> m_thumbs;

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_thumbBorder;
};

} // namespace PdfElite
