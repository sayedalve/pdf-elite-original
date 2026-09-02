#include "ModeRail.h"
#include "../NativeDesignSystem.h"
#include "../GraphicsDevice.h"

namespace components {

ModeRail::ModeRail() {
    // Visual top-to-bottom order. Modes without a real backend are disabled.
    // Exact order from provided snippet
    m_items = {
        { app::AppMode::Home,     L"Home",     controls::IconType::Home,     true  },
        { app::AppMode::Comment,  L"Comment",  controls::IconType::Comment,  true  },
        { app::AppMode::Edit,     L"Edit",     controls::IconType::Edit,     true  },
        { app::AppMode::View,     L"View",     controls::IconType::View,     true  },
        { app::AppMode::Organize, L"Organize", controls::IconType::Organize, true  },
        { app::AppMode::Tools,    L"Tools",    controls::IconType::Tools,    true  },
    };
}

void ModeRail::EnsureFormat() {
    if (m_labelFormat) return;

    auto factory = GraphicsDevice::Instance().GetDWriteFactory();
    if (!factory) return;

    HRESULT hr = factory->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        design::TypographySize::Caption, L"en-us", &m_labelFormat);
    if (FAILED(hr) || !m_labelFormat) { m_labelFormat.Reset(); return; }

    m_labelFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_labelFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    m_labelFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
}

int ModeRail::ItemAt(float x, float y) const {
    if (x < m_bounds.left || x > m_bounds.right) return -1;
    if (y < m_bounds.top || y > m_bounds.bottom) return -1;
    float adjustedY = y - m_bounds.top;
    
    // Home button is at top: 8 to 8+56
    if (adjustedY >= 8.0f && adjustedY <= 64.0f) {
        return 0; // Home
    }
    
    // Others start at 64, spaced by 56
    if (adjustedY >= 64.0f) {
        float relY = adjustedY - 64.0f;
        int idx = 1 + static_cast<int>(relY / 64.0f);
        if (idx >= 1 && idx < static_cast<int>(m_items.size())) {
            float itemTop = 64.0f + (idx - 1) * 64.0f;
            if (adjustedY >= itemTop && adjustedY <= itemTop + 56.0f) {
                return idx;
            }
        }
    }
    return -1;
}

void ModeRail::Render(ComPtr<ID2D1RenderTarget> target) {
    EnsureFormat();

    // Rail background
    ComPtr<ID2D1SolidColorBrush> bgBrush;
    target->CreateSolidColorBrush(design::Colors::SidebarBg, &bgBrush);
    target->FillRectangle(m_bounds, bgBrush.Get());

    // Right border of the rail
    ComPtr<ID2D1SolidColorBrush> borderBrush;
    target->CreateSolidColorBrush(design::Colors::BorderSubtle, &borderBrush);
    if (borderBrush) {
        target->DrawLine(D2D1::Point2F(m_bounds.right, m_bounds.top), D2D1::Point2F(m_bounds.right, m_bounds.bottom), borderBrush.Get(), 1.0f);
    }

    float lr_width = m_bounds.right - m_bounds.left;

    auto drawLeftRailItem = [&](int idx, float y, bool active) {
        D2D1_RECT_F itemRect = D2D1::RectF(m_bounds.left+4, y, m_bounds.right-4, y+56);
        
        ComPtr<ID2D1SolidColorBrush> accent;
        target->CreateSolidColorBrush(design::Colors::AccentPrimary, &accent);
        ComPtr<ID2D1SolidColorBrush> white;
        target->CreateSolidColorBrush(design::Colors::TextPrimary, &white);
        ComPtr<ID2D1SolidColorBrush> surface;
        target->CreateSolidColorBrush(design::Colors::Surface, &surface);
        
        auto brush = active ? accent : white;
        
        if (active && surface) {
            D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(itemRect, 4, 4);
            target->FillRoundedRectangle(rr, surface.Get());
        } else if (idx == m_hoverIndex && surface && m_items[idx].enabled) {
            D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(itemRect, 4, 4);
            target->FillRoundedRectangle(rr, surface.Get());
        }

        float iconSize = 24.0f;
        float ix = m_bounds.left + (lr_width - iconSize) / 2.0f;
        D2D1_RECT_F iconRect = D2D1::RectF(ix, itemRect.top+8, ix+iconSize, itemRect.top+8+iconSize);
        controls::IconRenderer::DrawIcon(target, m_items[idx].icon, iconRect, active ? design::Colors::AccentPrimary : design::Colors::TextPrimary);

        if (m_labelFormat) {
            float labelW = wcslen(m_items[idx].label.c_str()) * 6.0f;
            float ctx = m_bounds.left + (lr_width - labelW) / 2.0f;
            D2D1_RECT_F labelRect = D2D1::RectF(ctx, itemRect.top+34, m_bounds.right, itemRect.bottom);
            target->DrawText(m_items[idx].label.c_str(), static_cast<UINT32>(m_items[idx].label.length()), m_labelFormat.Get(), labelRect, brush.Get());
        }
    };

    drawLeftRailItem(0, m_bounds.top + 8, false);
    
    if (borderBrush) {
        target->DrawLine(D2D1::Point2F(m_bounds.left+12, m_bounds.top + 64), D2D1::Point2F(m_bounds.right-12, m_bounds.top + 64), borderBrush.Get(), 1.0f);
    }

    float ly = m_bounds.top + 64;
    for (int i = 1; i < static_cast<int>(m_items.size()); ++i) {
        bool active = (m_items[i].mode == m_activeMode);
        drawLeftRailItem(i, ly, active);
        ly += 64; // Uniform spacing
    }
}

void ModeRail::OnMouseMove(float x, float y) {
    int idx = ItemAt(x, y);
    if (idx >= 0 && !m_items[idx].enabled) idx = -1; // don't hover disabled modes
    m_hoverIndex = idx;
}

void ModeRail::OnMouseDown(float x, float y) {
    int idx = ItemAt(x, y);
    m_pressedIndex = (idx >= 0 && m_items[idx].enabled) ? idx : -1;
}

void ModeRail::OnMouseUp(float x, float y) {
    int idx = ItemAt(x, y);
    if (idx >= 0 && idx == m_pressedIndex && m_items[idx].enabled) {
        m_activeMode = m_items[idx].mode;
        if (m_onModeSelected) m_onModeSelected(m_items[idx].mode);
    }
    m_pressedIndex = -1;
}

void ModeRail::OnMouseLeave() {
    m_hoverIndex = -1;
    m_pressedIndex = -1;
}

} // namespace components
