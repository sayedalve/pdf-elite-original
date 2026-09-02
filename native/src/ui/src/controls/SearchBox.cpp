#include "SearchBox.h"
#include "../GraphicsDevice.h"
#include "../NativeDesignSystem.h"

namespace controls {

SearchBox::SearchBox() {
}

void SearchBox::Render(ComPtr<ID2D1RenderTarget> target) {
    ComPtr<ID2D1SolidColorBrush> bgBrush;
    target->CreateSolidColorBrush(design::Colors::Control, &bgBrush);
    D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(m_bounds, design::Radius::R6, design::Radius::R6);
    target->FillRoundedRectangle(roundedRect, bgBrush.Get());

    ComPtr<ID2D1SolidColorBrush> borderBrush;
    target->CreateSolidColorBrush(m_isFocused ? design::Colors::AccentPrimary : (m_isHovered ? design::Colors::BorderStrong : design::Colors::Border), &borderBrush);
    target->DrawRoundedRectangle(roundedRect, borderBrush.Get(), m_isFocused ? 2.0f : 1.0f);

    D2D1_RECT_F iconBounds = {m_bounds.left + 8.0f, m_bounds.top + 8.0f, m_bounds.left + 24.0f, m_bounds.bottom - 8.0f};
    IconRenderer::DrawIcon(target, IconType::Search, iconBounds, design::Colors::TextSecondary);

    auto textFormat = design::FontManager::Instance().GetBody();
    
    if (textFormat) {
        ComPtr<ID2D1SolidColorBrush> textBrush;
        target->CreateSolidColorBrush(design::Colors::TextMuted, &textBrush);
        D2D1_RECT_F textRect = {m_bounds.left + 32.0f, m_bounds.top + 8.0f, m_bounds.right - 8.0f, m_bounds.bottom - 8.0f};
        target->DrawText(m_placeholder.c_str(), static_cast<UINT32>(m_placeholder.length()), textFormat, textRect, textBrush.Get());
    }
}

void SearchBox::OnMouseMove(float /*x*/, float /*y*/) {
    m_isHovered = true;
}

void SearchBox::OnMouseDown(float /*x*/, float /*y*/) {
    m_isFocused = true;
}

void SearchBox::OnMouseUp(float /*x*/, float /*y*/) {
}

void SearchBox::OnMouseLeave() {
    m_isHovered = false;
    // Keep focus until clicked elsewhere
}

} // namespace controls
