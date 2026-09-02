#include "StatusBar.h"
#include "../GraphicsDevice.h"
#include "../NativeDesignSystem.h"

namespace components {

StatusBar::StatusBar() {
    SetBackgroundColor(design::Colors::SidebarBg);

    m_btnDarkMode = AddButton(L"Dark Mode", controls::IconType::DarkMode);
    m_btnDarkMode->SetEnabled(true);
    m_btnThumbnails = AddButton(L"Thumbnails", controls::IconType::Thumbnails);
    m_btnThumbnails->SetEnabled(true);
    m_btnBookmarks = AddButton(L"Bookmarks", controls::IconType::Bookmark);
    m_btnBookmarks->SetEnabled(true);
    m_btnComments = AddButton(L"Comments", controls::IconType::Chat);
    m_btnComments->SetEnabled(true);
    m_btnMore = AddButton(L"More", controls::IconType::More);
    m_btnMore->SetEnabled(true);
    
    m_btnPageUp = AddButton(L"Page Up", controls::IconType::ArrowUp);
    m_btnPageUp->SetEnabled(true);
    m_btnPageDown = AddButton(L"Page Down", controls::IconType::ArrowDown);
    m_btnPageDown->SetEnabled(true);
    m_btnSelect = AddButton(L"Select", controls::IconType::Menu); // Using Menu as a placeholder for Select
    m_btnSelect->SetEnabled(false);
    m_btnFit = AddButton(L"Fit", controls::IconType::Fit);
    m_btnFit->SetEnabled(true);
    m_btnZoomIn = AddButton(L"Zoom In", controls::IconType::ZoomIn);
    m_btnZoomIn->SetEnabled(true);
    m_btnZoomOut = AddButton(L"Zoom Out", controls::IconType::ZoomOut);
    m_btnZoomOut->SetEnabled(true);
}

std::shared_ptr<controls::IconButton> StatusBar::AddButton(const std::wstring& text, controls::IconType icon) {
    auto btn = std::make_shared<controls::IconButton>(text, icon);
    btn->SetColors(D2D1::ColorF(0, 0.0f), design::Colors::SurfacePressed, design::Colors::TextPrimary);
    btn->SetShowText(false); // Only show icons
    btn->SetOnClick([this, text]() {
        if (onAction) onAction(text);
    });
    AddChild(btn);
    return btn;
}

void StatusBar::EnsureTextFormat() {
    if (m_textFormat) return;
    auto factory = GraphicsDevice::Instance().GetDWriteFactory();
    if (!factory) return;
    factory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"en-us", &m_textFormat);
    if (m_textFormat) {
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

void StatusBar::Layout(const D2D1_RECT_F& bounds) {
    UIElement::Layout(bounds);

    float btnSize = 40.0f;
    float padding = (bounds.right - bounds.left - btnSize) / 2.0f;
    if (padding < 0) padding = 0;
    
    float x1 = bounds.left + padding;
    float x2 = x1 + btnSize;
    
    // Top section
    float y = bounds.top + 8.0f;
    
    
    if (m_btnThumbnails) m_btnThumbnails->Layout({x1, y, x2, y + btnSize}); y += btnSize + 4.0f;
    if (m_btnBookmarks) m_btnBookmarks->Layout({x1, y, x2, y + btnSize}); y += btnSize + 4.0f;
    if (m_btnComments) m_btnComments->Layout({x1, y, x2, y + btnSize}); y += btnSize + 4.0f;
    if (m_btnMore) m_btnMore->Layout({x1, y, x2, y + btnSize});
    
    // Bottom section (we lay out bottom to top)
    // Prevent overlapping with top section (top section takes 280px, bottom takes 376px)
    float by = bounds.bottom - 8.0f;
    if (m_btnZoomOut) { by -= btnSize; m_btnZoomOut->Layout({x1, by, x2, by + btnSize}); by -= 8.0f; }
    if (m_btnZoomIn) { by -= btnSize; m_btnZoomIn->Layout({x1, by, x2, by + btnSize}); by -= 8.0f; }
    
    // Zoom % text
    by -= 32.0f; // 32px height for text box
    by -= 8.0f;
    
    if (m_btnFit) { by -= btnSize; m_btnFit->Layout({x1, by, x2, by + btnSize}); by -= 8.0f; }
    if (m_btnSelect) { by -= btnSize; m_btnSelect->Layout({x1, by, x2, by + btnSize}); by -= 8.0f; }
    
    // Page Numbers
    by -= 24.0f; // total pages text
    by -= 4.0f;
    by -= 32.0f; // current page box
    by -= 8.0f;
    
    if (m_btnPageDown) { by -= btnSize; m_btnPageDown->Layout({x1, by, x2, by + btnSize}); by -= 8.0f; }
    if (m_btnPageUp) { by -= btnSize; m_btnPageUp->Layout({x1, by, x2, by + btnSize}); by -= 8.0f; }
    
    // Dark mode button instead of page numbers
    if (m_btnDarkMode) { by -= btnSize; m_btnDarkMode->Layout({x1, by, x2, by + btnSize}); by -= 8.0f; }
}

void StatusBar::SetPageInfo(int currentPage, int totalPages) {
    m_currentPage = currentPage;
    m_totalPages = totalPages;
}

void StatusBar::SetZoom(float zoom) {
    m_zoom = zoom;
}

void StatusBar::SetFileName(const std::wstring& fileName) {
    m_fileName = fileName;
}

void StatusBar::SetState(const std::wstring& state) {
    m_state = state;
}

void StatusBar::Render(ComPtr<ID2D1RenderTarget> target) {
    if (m_bg.a > 0.0f) {
        ComPtr<ID2D1SolidColorBrush> brush;
        target->CreateSolidColorBrush(m_bg, &brush);
        target->FillRectangle(m_bounds, brush.Get());
    }
    EnsureTextFormat();
    
    // Left border
    ComPtr<ID2D1SolidColorBrush> borderBrush;
    if (SUCCEEDED(target->CreateSolidColorBrush(design::Colors::BorderSubtle, &borderBrush))) {
        target->DrawLine(D2D1::Point2F(m_bounds.left, m_bounds.top), D2D1::Point2F(m_bounds.left, m_bounds.bottom), borderBrush.Get(), 1.0f);
    }
    
    ComPtr<ID2D1SolidColorBrush> textBrush;
    target->CreateSolidColorBrush(design::Colors::TextPrimary, &textBrush);
    
    ComPtr<ID2D1SolidColorBrush> textBrushSecondary;
    target->CreateSolidColorBrush(design::Colors::TextSecondary, &textBrushSecondary);
    
    ComPtr<ID2D1SolidColorBrush> bgBrush;
    target->CreateSolidColorBrush(design::Colors::SurfaceActiveTab, &bgBrush);
    
    // Calculate layout positions again for drawing text
    float btnSize = 40.0f;
    float padding = (m_bounds.right - m_bounds.left - btnSize) / 2.0f;
    if (padding < 0) padding = 0;
    float x1 = m_bounds.left + padding;
    float x2 = x1 + btnSize;
    
    float by = m_bounds.bottom - 8.0f;
    by -= btnSize + 8.0f; // zoom out
    by -= btnSize + 8.0f; // zoom in
    
    // Draw Zoom %
    float zoomH = 32.0f;
    by -= zoomH;
    D2D1_RECT_F zoomRect = {x1, by, x2, by + zoomH};
    target->FillRoundedRectangle(D2D1::RoundedRect(zoomRect, 8.0f, 8.0f), bgBrush.Get());
    std::wstring zoomText = std::to_wstring((int)(m_zoom * 100)) + L"%";
    if (m_textFormat && textBrush) {
        target->DrawTextW(zoomText.c_str(), static_cast<UINT32>(zoomText.length()), m_textFormat.Get(), zoomRect, textBrush.Get());
    }
    
    by -= 8.0f; // space before fit
    by -= btnSize + 8.0f; // fit
    by -= btnSize + 8.0f; // select
    
    // Draw Page Numbers
    by -= 24.0f;
    D2D1_RECT_F totalRect = {x1, by, x2, by + 24.0f};
    by -= 4.0f;
    by -= 32.0f;
    D2D1_RECT_F currentRect = {x1, by, x2, by + 32.0f};
    by -= 8.0f;
    
    target->FillRoundedRectangle(D2D1::RoundedRect(currentRect, 8.0f, 8.0f), bgBrush.Get());
    std::wstring currStr = std::to_wstring(m_currentPage + 1);
    std::wstring totalStr = std::to_wstring(m_totalPages > 0 ? m_totalPages : 1);
    if (m_textFormat && textBrush && textBrushSecondary) {
        target->DrawTextW(currStr.c_str(), static_cast<UINT32>(currStr.length()), m_textFormat.Get(), currentRect, textBrush.Get());
        target->DrawTextW(totalStr.c_str(), static_cast<UINT32>(totalStr.length()), m_textFormat.Get(), totalRect, textBrushSecondary.Get());
    }
    
    by -= btnSize + 8.0f; // pagedown
    by -= btnSize + 8.0f; // pageup
    

    
    // Draw Active Background for Select Button (so it doesn't get blue border)
    if (m_btnSelect) {
        D2D1_RECT_F selRect = m_btnSelect->GetBounds();
        target->FillRoundedRectangle(D2D1::RoundedRect(selRect, 8.0f, 8.0f), bgBrush.Get());
    }
    
    // Draw the buttons OVER the backgrounds we just drew
    for (auto& child : m_children) {
        if (child && child->IsVisible()) {
            child->Render(target);
        }
    }
    
    // Draw corner triangles for Select and Fit
    ComPtr<ID2D1SolidColorBrush> triangleBrush;
    target->CreateSolidColorBrush(D2D1::ColorF(0.8f, 0.8f, 0.8f), &triangleBrush);
    
    auto drawTriangle = [&](const D2D1_RECT_F& rect) {
        ComPtr<ID2D1PathGeometry> path;
        GraphicsDevice::Instance().GetD2DFactory()->CreatePathGeometry(&path);
        ComPtr<ID2D1GeometrySink> sink;
        if (SUCCEEDED(path->Open(&sink))) {
            sink->BeginFigure({rect.right, rect.bottom}, D2D1_FIGURE_BEGIN_FILLED);
            sink->AddLine({rect.right - 6.0f, rect.bottom});
            sink->AddLine({rect.right, rect.bottom - 6.0f});
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();
            target->FillGeometry(path.Get(), triangleBrush.Get());
        }
    };
    
    if (m_btnSelect) drawTriangle(m_btnSelect->GetBounds());
    if (m_btnFit) drawTriangle(m_btnFit->GetBounds());
}


void StatusBar::OnMouseUp(float x, float y) {
    framework::Panel::OnMouseUp(x, y);
    
    float btnSize = 44.0f;
    float x1 = m_bounds.left + (m_bounds.right - m_bounds.left - btnSize) / 2.0f;
    float x2 = x1 + btnSize;
    
    float by = m_bounds.bottom - 8.0f;
    by -= btnSize + 8.0f; // zoom out
    by -= btnSize + 8.0f; // zoom in
    
    float zoomH = 32.0f;
    by -= zoomH;
    by -= 8.0f; // space before fit
    by -= btnSize + 8.0f; // fit
    by -= btnSize + 8.0f; // select
    
    by -= 24.0f;
    by -= 4.0f;
    by -= 32.0f;
    D2D1_RECT_F currentRect = {x1, by, x2, by + 32.0f};
    
    if (x >= currentRect.left && x <= currentRect.right && y >= currentRect.top && y <= currentRect.bottom) {
        if (onAction) onAction(L"GoToPageDialog");
    }
}
} // namespace components


