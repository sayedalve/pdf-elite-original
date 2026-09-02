#include "TabBar.h"
#include "../NativeDesignSystem.h"
#include "../GraphicsDevice.h"
#include "../controls/IconRenderer.h"

namespace components {

TabBar::TabBar() {
}

void TabBar::SetTabs(const std::vector<std::wstring>& titles, int activeIndex) {
    m_titles = titles;
    m_activeIndex = activeIndex;
}

void TabBar::EnsureTabFormat() {
    if (m_tabFormat) return;

    auto factory = GraphicsDevice::Instance().GetDWriteFactory();
    if (!factory) return;

    // 13px medium Segoe UI, matching the toolbar type ramp, but leading-aligned
    // and single-line so titles read left-to-right and trim on the right.
    HRESULT hr = factory->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        design::TypographySize::Toolbar, L"en-us", &m_tabFormat);
    if (FAILED(hr) || !m_tabFormat) { m_tabFormat.Reset(); return; }

    m_tabFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_tabFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    m_tabFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    // "…" trimming sign so overly long file names are shown as "Beginning_of_na…".
    if (SUCCEEDED(factory->CreateEllipsisTrimmingSign(m_tabFormat.Get(), &m_ellipsisSign))) {
        DWRITE_TRIMMING trimming = {};
        trimming.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
        m_tabFormat->SetTrimming(&trimming, m_ellipsisSign.Get());
    }
}

void TabBar::Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) {
    auto bounds = GetBounds();

    EnsureTabFormat();

    // TopBar Background
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> topBarBrush;
    target->CreateSolidColorBrush(design::Colors::SidebarBg, &topBarBrush);
    if (topBarBrush) {
        target->FillRectangle(bounds, topBarBrush.Get());
    }

    // Draw background and bottom border
    ComPtr<ID2D1SolidColorBrush> borderBrush;
    target->CreateSolidColorBrush(design::Colors::BorderSubtle, &borderBrush);
    target->DrawLine(D2D1::Point2F(bounds.left, bounds.bottom), D2D1::Point2F(bounds.right, bounds.bottom), borderBrush.Get(), 1.0f);

    float x = bounds.left + 8.0f; // Padding left
    float tabWidth = 260.0f;
    float tabHeight = bounds.bottom - bounds.top - 16.0f; // topBar.top+8 to topBar.bottom-8
    
    float tabY = bounds.top + 8.0f;

    for (int i = 0; i < (int)m_titles.size(); ++i) {
        D2D1_RECT_F tabRect = {x, tabY, x + tabWidth, tabY + tabHeight};

        // Draw background
        D2D1_COLOR_F bgColor = design::Colors::SurfaceElevated;
        if (i == m_activeIndex) {
            bgColor = design::Colors::Control; // matches toolbar background
        }

        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> bgBrush;
        target->CreateSolidColorBrush(bgColor, &bgBrush);

        D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(tabRect, 8.0f, 8.0f);
        target->FillRoundedRectangle(roundedRect, bgBrush.Get());
        
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> tabBorderBrush;
        target->CreateSolidColorBrush(design::Colors::BorderSubtle, &tabBorderBrush);
        target->DrawRoundedRectangle(roundedRect, tabBorderBrush.Get(), 1.0f);

        // Draw title
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> textBrush;
        target->CreateSolidColorBrush(design::Colors::TextPrimary, &textBrush);

        IDWriteTextFormat* textFormat = m_tabFormat ? m_tabFormat.Get()
                                                    : design::FontManager::Instance().GetToolbar();
        if (textFormat) {
            D2D1_RECT_F textBounds = tabRect;
            textBounds.left += 12.0f;
            textBounds.top -= 2.0f; // Align text properly
            textBounds.right -= 28.0f; // Space for close icon
            if (textBounds.right < textBounds.left) textBounds.right = textBounds.left;

            target->PushAxisAlignedClip(textBounds, D2D1_ANTIALIAS_MODE_ALIASED);
            target->DrawText(m_titles[i].c_str(), static_cast<UINT32>(m_titles[i].length()),
                textFormat, textBounds, textBrush.Get());
            target->PopAxisAlignedClip();
        }

        // Draw close ('X') button icon
        D2D1_RECT_F closeIconRect = D2D1::RectF(
            tabRect.right - 22.0f,
            tabRect.top + (tabHeight - 12.0f) / 2.0f,
            tabRect.right - 10.0f,
            tabRect.top + (tabHeight + 12.0f) / 2.0f
        );
        controls::IconRenderer::DrawIcon(target, controls::IconType::Close, closeIconRect, design::Colors::TextSecondary);

        x += tabWidth + 8.0f;
    }
}

void TabBar::OnMouseDown(float x, float y) { (void)y;
    auto bounds = GetBounds();
    float tabWidth = 260.0f;
    
    float currentX = bounds.left + 8.0f;
    for (int i = 0; i < (int)m_titles.size(); ++i) {
        if (x >= currentX && x <= currentX + tabWidth) {
            // Rightmost 28px of the tab rectangle is the close ('X') button
            if (x >= currentX + tabWidth - 28.0f) {
                if (onTabClosed) onTabClosed(i);
            } else {
                if (onTabSelected) onTabSelected(i);
            }
            return;
        }
        currentX += tabWidth + 8.0f;
    }
}

void TabBar::OnMouseMove(float x, float y) { (void)y;
    auto bounds = GetBounds();
    float tabWidth = 260.0f;
    
    int newHover = -1;
    float currentX = bounds.left + 8.0f;
    for (int i = 0; i < (int)m_titles.size(); ++i) {
        if (x >= currentX && x <= currentX + tabWidth) {
            newHover = i;
            break;
        }
        currentX += tabWidth + 8.0f;
    }
    
    if (newHover != m_hoverIndex) {
        m_hoverIndex = newHover;
    }
    
}

} // namespace components
