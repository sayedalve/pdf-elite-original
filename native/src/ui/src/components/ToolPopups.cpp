#include "ToolPopups.h"
#include "../NativeDesignSystem.h"

namespace ui::components {

BaseToolPopup::BaseToolPopup(const std::wstring& title, const std::wstring& shortcut, const std::wstring& desc)
    : m_title(title), m_shortcut(shortcut), m_desc(desc) {
    SetBackgroundColor(design::Colors::Surface);
}

void BaseToolPopup::Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) {
    framework::Panel::Render(target);
    // Render text
    // Basic render for the popup
    ID2D1SolidColorBrush* bgBrush = nullptr;
    target->CreateSolidColorBrush(D2D1::ColorF(0.15f, 0.15f, 0.15f, 1.0f), &bgBrush);
    
    ID2D1SolidColorBrush* borderBrush = nullptr;
    target->CreateSolidColorBrush(D2D1::ColorF(0.3f, 0.3f, 0.3f, 1.0f), &borderBrush);
    
    ID2D1SolidColorBrush* textBrush = nullptr;
    target->CreateSolidColorBrush(D2D1::ColorF(0.9f, 0.9f, 0.9f, 1.0f), &textBrush);
    
    if (bgBrush) {
        D2D1_ROUNDED_RECT rrect = D2D1::RoundedRect(m_bounds, 6.0f, 6.0f);
        target->FillRoundedRectangle(&rrect, bgBrush);
        if (borderBrush) target->DrawRoundedRectangle(&rrect, borderBrush, 1.0f);
    }
    
    if (bgBrush) bgBrush->Release();
    if (borderBrush) borderBrush->Release();
    if (textBrush) textBrush->Release();
}

HighlightPopup::HighlightPopup() : BaseToolPopup(L"Highlight", L"Alt + Shift + 1", L"Highlight text.") {}
AreaHighlightPopup::AreaHighlightPopup() : BaseToolPopup(L"Area Highlight", L"Alt + Shift + 0", L"Area highlight text.") {}
PencilPopup::PencilPopup() : BaseToolPopup(L"Pencil", L"Alt + Shift + P", L"Use pencil drawing tool.") {}
EraserPopup::EraserPopup() : BaseToolPopup(L"Eraser", L"Alt + Shift + E", L"Use eraser to clean pencil drawing.") {}
ShapesPopup::ShapesPopup() : BaseToolPopup(L"Shapes", L"", L"Add shapes.") {}

}


