#include "OrganizeToolbar.h"
#include "../NativeDesignSystem.h"

namespace components {

OrganizeToolbar::OrganizeToolbar() {
    SetBackgroundColor(design::Colors::Surface);
    
    // Add buttons
    m_buttons.push_back({L"Undo", L"Undo", controls::IconType::Undo, {0}, true, false, false});
    m_buttons.push_back({L"Redo", L"Redo", controls::IconType::Redo, {0}, true, false, false});
    m_buttons.push_back({L"Sep1", L"|", controls::IconType::None, {0}, false, false, false});
    m_buttons.push_back({L"Insert", L"Insert", controls::IconType::Insert, {0}, true, false, false});
    m_buttons.push_back({L"Extract", L"Extract", controls::IconType::Extract, {0}, true, true, false});
    m_buttons.push_back({L"Split", L"Split", controls::IconType::Split, {0}, true, false, false});
    m_buttons.push_back({L"Delete", L"Delete", controls::IconType::Delete, {0}, true, true, false});
    m_buttons.push_back({L"Sep2", L"|", controls::IconType::None, {0}, false, false, false});
    m_buttons.push_back({L"RotateCW", L"Rotate Right", controls::IconType::RotateCW, {0}, true, true, false});
    m_buttons.push_back({L"RotateCCW", L"Rotate Left", controls::IconType::RotateCCW, {0}, true, true, false});
}

void OrganizeToolbar::UpdateState(bool hasSelection, int numSelected) {
    (void)numSelected;
    for (auto& b : m_buttons) {
        if (b.requiresSelection) {
            b.enabled = hasSelection;
        }
    }
}

void OrganizeToolbar::SetDarkMode(bool dark) {
    m_isDarkMode = dark;
}

void OrganizeToolbar::Layout(const D2D1_RECT_F& bounds) {
    UIElement::Layout(bounds);
    
    float x = bounds.left + 20.0f;
    float y = bounds.top + 8.0f;
    float height = 32.0f;
    
    for (auto& b : m_buttons) {
        float width = 80.0f;
        if (b.id.find(L"Sep") == 0) width = 20.0f;
        else if (b.label.length() > 5) width = 100.0f;
        
        b.bounds = {x, y, x + width, y + height};
        x += width + 10.0f;
    }
}

void OrganizeToolbar::Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) {
    Panel::Render(target);
    
    ComPtr<ID2D1SolidColorBrush> textBrush, disabledBrush, hoverBrush;
    target->CreateSolidColorBrush(design::Colors::TextPrimary, &textBrush);
    target->CreateSolidColorBrush(design::Colors::TextMuted, &disabledBrush);
    target->CreateSolidColorBrush(design::Colors::ControlHover, &hoverBrush);
    
    auto textFormat = design::FontManager::Instance().GetMetadata();
    if (textFormat) {
        textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
    
    for (const auto& b : m_buttons) {
        if (b.hover && b.enabled && b.id.find(L"Sep") != 0) {
            D2D1_ROUNDED_RECT rr = {b.bounds, 4.0f, 4.0f};
            target->FillRoundedRectangle(rr, hoverBrush.Get());
        }
        
        if (b.iconType != controls::IconType::None) {
            D2D1_RECT_F iconBounds = {
                b.bounds.left + 8.0f,
                b.bounds.top,
                b.bounds.left + 32.0f,
                b.bounds.bottom
            };
            controls::IconRenderer::DrawIcon(target, b.iconType, iconBounds, 
                b.enabled ? design::Colors::TextPrimary : design::Colors::TextMuted);
            
            D2D1_RECT_F textBounds = {
                b.bounds.left + 32.0f,
                b.bounds.top,
                b.bounds.right,
                b.bounds.bottom
            };
            if (textFormat) textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            target->DrawTextW(
                b.label.c_str(), static_cast<UINT32>(b.label.length()),
                textFormat, textBounds,
                b.enabled ? textBrush.Get() : disabledBrush.Get()
            );
            if (textFormat) textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        } else {
            target->DrawTextW(
                b.label.c_str(), static_cast<UINT32>(b.label.length()),
                textFormat, b.bounds,
                b.enabled ? textBrush.Get() : disabledBrush.Get()
            );
        }
    }
}

int OrganizeToolbar::GetButtonAt(float x, float y) {
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        if (x >= m_buttons[i].bounds.left && x <= m_buttons[i].bounds.right &&
            y >= m_buttons[i].bounds.top && y <= m_buttons[i].bounds.bottom) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void OrganizeToolbar::OnMouseDown(float x, float y) {
    int idx = GetButtonAt(x, y);
    if (idx >= 0 && m_buttons[idx].enabled && m_buttons[idx].id.find(L"Sep") != 0) {
        if (onAction) onAction(m_buttons[idx].id);
    }
}

void OrganizeToolbar::OnMouseMove(float x, float y) {
    int idx = GetButtonAt(x, y);
    bool changed = false;
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        bool h = (static_cast<int>(i) == idx);
        if (m_buttons[i].hover != h) {
            m_buttons[i].hover = h;
            changed = true;
        }
    }
    // We don't have InvalidateView() on Panel directly, but it relies on WM_PAINT. 
    // This is fine for now, or we can just let hover lag to next paint.
}

} // namespace components
