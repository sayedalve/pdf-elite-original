#include "PropertiesPanel.h"
#include "../NativeDesignSystem.h"
#include "../GraphicsDevice.h"
#include "../interaction/TextSelectableObject.h"
#include <iomanip>
#include <sstream>

namespace components {

PropertiesPanel::PropertiesPanel() {
    SetBackgroundColor(design::Colors::SurfaceElevated);
}

void PropertiesPanel::EnsureFormat() {
    if (m_formatHeader) return;
    auto factory = GraphicsDevice::Instance().GetDWriteFactory();
    if (!factory) return;
    
    factory->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        design::TypographySize::SectionHeading, L"en-us", &m_formatHeader);
        
    factory->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        design::TypographySize::Body, L"en-us", &m_formatLabel);
}

void PropertiesPanel::SetSelectedObject(std::shared_ptr<ui::interaction::ISelectableObject> obj) {
    m_selectedObj = obj;
    if (auto textObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(obj)) {
        if (auto t = textObj->GetTextObject()) {
            m_currentText = t->GetText();
            m_currentFontSize = t->GetFontSize();
            uint8_t a;
            t->GetColor(m_colorR, m_colorG, m_colorB, a);
        }
    }
}

void PropertiesPanel::Layout(const D2D1_RECT_F& bounds) {
    UIElement::Layout(bounds);
}

void PropertiesPanel::Render(ComPtr<ID2D1RenderTarget> target) {
    EnsureFormat();
    
    ComPtr<ID2D1SolidColorBrush> bgBrush;
    target->CreateSolidColorBrush(design::Colors::SurfaceElevated, &bgBrush);
    target->FillRectangle(m_bounds, bgBrush.Get());
    
    ComPtr<ID2D1SolidColorBrush> borderBrush;
    target->CreateSolidColorBrush(design::Colors::Border, &borderBrush);
    target->DrawLine(D2D1::Point2F(m_bounds.left, m_bounds.top), D2D1::Point2F(m_bounds.left, m_bounds.bottom), borderBrush.Get(), 1.0f);
    
    if (!m_selectedObj || !m_formatHeader) return;
    
    ComPtr<ID2D1SolidColorBrush> textBrush;
    target->CreateSolidColorBrush(design::Colors::TextPrimary, &textBrush);
    
    ComPtr<ID2D1SolidColorBrush> textSecBrush;
    target->CreateSolidColorBrush(design::Colors::TextSecondary, &textSecBrush);
    
    float padding = design::Spacing::Medium2;
    float y = m_bounds.top + padding;
    
    // Header
    std::wstring title = L"Properties";
    D2D1_RECT_F titleRect = { m_bounds.left + padding, y, m_bounds.right - padding, y + 30.0f };
    target->DrawTextW(title.c_str(), static_cast<UINT32>(title.length()), m_formatHeader.Get(), titleRect, textBrush.Get());
    y += 40.0f;
    
    if (auto textObj = std::dynamic_pointer_cast<ui::interaction::TextSelectableObject>(m_selectedObj)) {
        // Font Size
        std::wstring sizeLbl = L"Font Size: ";
        D2D1_RECT_F lblRect = { m_bounds.left + padding, y, m_bounds.right - padding, y + 20.0f };
        target->DrawTextW(sizeLbl.c_str(), static_cast<UINT32>(sizeLbl.length()), m_formatLabel.Get(), lblRect, textSecBrush.Get());
        
        std::wstringstream wss;
        wss << std::fixed << std::setprecision(1) << m_currentFontSize << L" pt";
        std::wstring sizeVal = wss.str();
        D2D1_RECT_F valRect = { m_bounds.left + padding + 80.0f, y, m_bounds.right - padding, y + 20.0f };
        target->DrawTextW(sizeVal.c_str(), static_cast<UINT32>(sizeVal.length()), m_formatLabel.Get(), valRect, textBrush.Get());
        y += 30.0f;
        
        // Color
        std::wstring colorLbl = L"Color:";
        D2D1_RECT_F colorLblRect = { m_bounds.left + padding, y, m_bounds.right - padding, y + 20.0f };
        target->DrawTextW(colorLbl.c_str(), static_cast<UINT32>(colorLbl.length()), m_formatLabel.Get(), colorLblRect, textSecBrush.Get());
        
        D2D1_RECT_F colorSwatch = { m_bounds.left + padding + 80.0f, y + 2.0f, m_bounds.left + padding + 100.0f, y + 22.0f };
        ComPtr<ID2D1SolidColorBrush> swatchBrush;
        target->CreateSolidColorBrush(D2D1::ColorF(m_colorR/255.0f, m_colorG/255.0f, m_colorB/255.0f), &swatchBrush);
        target->FillRectangle(colorSwatch, swatchBrush.Get());
        target->DrawRectangle(colorSwatch, borderBrush.Get());
        y += 30.0f;
        
        // Text
        std::wstring textLbl = L"Text Content:";
        D2D1_RECT_F textLblRect = { m_bounds.left + padding, y, m_bounds.right - padding, y + 20.0f };
        target->DrawTextW(textLbl.c_str(), static_cast<UINT32>(textLbl.length()), m_formatLabel.Get(), textLblRect, textSecBrush.Get());
        y += 20.0f;
        
        D2D1_RECT_F textValRect = { m_bounds.left + padding, y, m_bounds.right - padding, m_bounds.bottom - padding };
        target->DrawTextW(m_currentText.c_str(), static_cast<UINT32>(m_currentText.length()), m_formatLabel.Get(), textValRect, textBrush.Get());
    }
}

} // namespace components
