#include "RecentFileRow.h"
#include "../GraphicsDevice.h"
#include "../NativeDesignSystem.h"

namespace controls {

RecentFileRow::RecentFileRow(const std::wstring& filename, const std::wstring& modified, const std::wstring& size) 
    : m_filename(filename), m_modified(modified), m_size(size) {
}

void RecentFileRow::Render(ComPtr<ID2D1RenderTarget> target) {
    if (m_isHovered || m_isPressed) {
        ComPtr<ID2D1SolidColorBrush> bgBrush;
        D2D1_COLOR_F bg = m_isPressed ? design::Colors::ControlActive : design::Colors::ControlHover;
        target->CreateSolidColorBrush(bg, &bgBrush);
        D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(m_bounds, design::Radius::R6, design::Radius::R6);
        target->FillRoundedRectangle(roundedRect, bgBrush.Get());
    }

    // PDF Icon
    D2D1_RECT_F iconBounds = {m_bounds.left + 16.0f, m_bounds.top + 10.0f, m_bounds.left + 36.0f, m_bounds.top + 30.0f};
    IconRenderer::DrawIcon(target, IconType::PDFDocument, iconBounds, design::Colors::Error);

    auto nameFormat = design::FontManager::Instance().GetBody();
    auto metaFormat = design::FontManager::Instance().GetSecondary();

    if (nameFormat && metaFormat) {
        ComPtr<ID2D1SolidColorBrush> textBrush, metaBrush;
        target->CreateSolidColorBrush(design::Colors::TextPrimary, &textBrush);
        target->CreateSolidColorBrush(design::Colors::TextSecondary, &metaBrush);

        float w = m_bounds.right - m_bounds.left;
        float nameW = w * 0.5f;
        float modW = w * 0.3f;
        float sizeW = w * 0.15f;

        // Draw Name
        D2D1_RECT_F nameRect = {m_bounds.left + 48.0f, m_bounds.top + 10.0f, m_bounds.left + 48.0f + nameW, m_bounds.bottom};
        target->DrawText(m_filename.c_str(), static_cast<UINT32>(m_filename.length()), nameFormat, nameRect, textBrush.Get());

        // Draw Modified
        D2D1_RECT_F modRect = {nameRect.right, m_bounds.top + 10.0f, nameRect.right + modW, m_bounds.bottom};
        target->DrawText(m_modified.c_str(), static_cast<UINT32>(m_modified.length()), metaFormat, modRect, metaBrush.Get());

        // Draw Size
        D2D1_RECT_F sizeRect = {modRect.right, m_bounds.top + 10.0f, modRect.right + sizeW, m_bounds.bottom};
        target->DrawText(m_size.c_str(), static_cast<UINT32>(m_size.length()), metaFormat, sizeRect, metaBrush.Get());
        
        // Draw More Action
        if (m_isHovered) {
            D2D1_RECT_F moreBounds = {m_bounds.right - 36.0f, m_bounds.top + 10.0f, m_bounds.right - 16.0f, m_bounds.top + 30.0f};
            IconRenderer::DrawIcon(target, IconType::More, moreBounds, design::Colors::TextSecondary);
        }
    }
}

void RecentFileRow::OnMouseMove(float /*x*/, float /*y*/) {
    m_isHovered = true;
}

void RecentFileRow::OnMouseDown(float /*x*/, float /*y*/) {
    m_isPressed = true;
}

void RecentFileRow::OnMouseUp(float /*x*/, float /*y*/) {
    if (m_isPressed) {
        m_isPressed = false;
        if (m_isHovered && m_onClick) {
            m_onClick();
        }
    }
}

void RecentFileRow::OnMouseLeave() {
    m_isHovered = false;
    m_isPressed = false;
}

} // namespace controls
