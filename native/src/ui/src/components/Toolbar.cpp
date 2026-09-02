#include "Toolbar.h"
#include "ToolPopups.h"
#include "PopupManager.h"
#include "ToolPopups.h"
#include "../controls/IconButton.h"
#include "../NativeDesignSystem.h"
#include <algorithm>

namespace components {

using app::AppMode;

Toolbar::Toolbar() {
    ui::commands::CommandManager::Instance().AddObserver(this);
    SetLayoutDirection(framework::LayoutDirection::Horizontal);
    SetBackgroundColor(design::Colors::Control); // Match the target UI toolbar background
    SetPadding(16.0f);
    SetSpacing(8.0f);

    std::vector<AppMode> allModes = {AppMode::View, AppMode::Comment, AppMode::Edit, AppMode::Organize, AppMode::Tools};
    
    // Far Right Utility
    AddButton(L"Search Tools", controls::IconType::Search, true, false, 2, allModes);
    AddButton(L"Print",       controls::IconType::Print, false,  true,  2, allModes);
    AddButton(L"Cloud",       controls::IconType::Cloud, false,  false, 2, allModes);
    AddButton(L"Share",       controls::IconType::Share, false,  false, 2, allModes);

    // Far Left Utility
    AddButton(L"Undo",        controls::IconType::Undo, true, true, 0, allModes);
    AddButton(L"Redo",        controls::IconType::Redo, true, true, 0, allModes);
    AddButton(L"Search",      controls::IconType::Search, false, false, 0, allModes);

    // View mode (fallback)
    AddButton(L"Hand",        controls::IconType::None, true, true, 1, {AppMode::View});
    AddButton(L"Select",      controls::IconType::View, true, true, 1, {AppMode::View});
    AddButton(L"Split View",  controls::IconType::View, true, false, 1, {AppMode::View});
    AddButton(L"New Window",  controls::IconType::View, true, false, 1, {AppMode::View});
    AddButton(L"Dark Mode",   controls::IconType::View, true, false, 1, {AppMode::View});

    // Edit mode: content editing
    AddButton(L"Edit All",    controls::IconType::Edit, true, true,  1, {AppMode::Edit});
    AddButton(L"Add Text",    controls::IconType::AddText, true, true,  1, {AppMode::Edit});
    AddButton(L"Add Link",    controls::IconType::Link, true, true, 1, {AppMode::Edit});
    AddButton(L"Image",       controls::IconType::Image, true, true, 1, {AppMode::Edit});
    AddButton(L"Watermark",   controls::IconType::Watermark, true, true, 1, {AppMode::Edit, AppMode::Organize});
    AddButton(L"Background",  controls::IconType::Background, true, true, 1, {AppMode::Edit, AppMode::Organize});
    AddButton(L"Header & Footer", controls::IconType::AddText, true, true, 1, {AppMode::Edit, AppMode::Organize});
    AddButton(L"...",         controls::IconType::More, false, false, 1, {AppMode::Edit});

    // Organize mode: page management
    AddButton(L"Insert",      controls::IconType::Insert, true, true, 1, {AppMode::Organize});
    AddButton(L"Replace",     controls::IconType::Convert, true, false, 1, {AppMode::Organize});
    AddButton(L"Extract Page", controls::IconType::Extract, true, true, 1, {AppMode::Organize});
    AddButton(L"Extract Images", controls::IconType::Image, true, true, 1, {AppMode::Organize, AppMode::Tools});
    AddButton(L"Combine Files", controls::IconType::Combine, true, true, 1, {AppMode::Organize, AppMode::Tools});
    AddButton(L"Split",       controls::IconType::Split, true, false, 1, {AppMode::Organize});
    AddButton(L"Delete",      controls::IconType::Delete, true, true, 1, {AppMode::Organize});
    AddButton(L"Rotate CW",   controls::IconType::RotateCW, true, true, 1, {AppMode::Organize});
    AddButton(L"Rotate CCW",  controls::IconType::RotateCCW, true, true, 1, {AppMode::Organize});
    
    auto btnHighlight = AddButton(L"Highlight", controls::IconType::Highlight, false, true, 1, {AppMode::Comment}); btnHighlight->SetTooltipText(L"Highlight text");
    auto btnArea = AddButton(L"Area Highlight", controls::IconType::AreaHighlight, false, true, 1, {AppMode::Comment}); btnArea->SetTooltipText(L"Area highlight text");
    auto btnPencil = AddButton(L"Pencil", controls::IconType::Pencil, false, true, 1, {AppMode::Comment}); btnPencil->SetTooltipText(L"Use pencil drawing tool");
    auto btnEraser = AddButton(L"Eraser", controls::IconType::Eraser, false, true, 1, {AppMode::Comment}); btnEraser->SetTooltipText(L"Use eraser to clean pencil drawing");
    auto btnNote = AddButton(L"Note", controls::IconType::Note, false, true, 1, {AppMode::Comment}); btnNote->SetTooltipText(L"Add note");
    auto btnComment = AddButton(L"Comment", controls::IconType::Comment, false, true, 1, {AppMode::Comment}); btnComment->SetTooltipText(L"Add comment");
    auto btnShapes = AddButton(L"Shapes", controls::IconType::Rectangle, false, true, 1, {AppMode::Comment}); btnShapes->SetTooltipText(L"Add shapes");
    auto btnTextBox = AddButton(L"Text Box", controls::IconType::TextBox, false, true, 1, {AppMode::Comment}); btnTextBox->SetTooltipText(L"Add text box");
    auto btnTypeWriter = AddButton(L"Type Writer", controls::IconType::AddText, false, true, 1, {AppMode::Comment}); btnTypeWriter->SetTooltipText(L"Add text without a border");
    auto btnCallout = AddButton(L"Text Callout", controls::IconType::TextCallout, false, true, 1, {AppMode::Comment}); btnCallout->SetTooltipText(L"Add callout text");

    auto setupPopup = [this](std::shared_ptr<controls::IconButton> btn, std::shared_ptr<framework::Panel> popup) {
        btn->onHoverStateChanged = [btn, popup](bool hovered) {
            if (hovered) {
                D2D1_RECT_F btnBounds = btn->GetBounds();
                D2D1_RECT_F popupBounds = D2D1::RectF(btnBounds.left, btnBounds.bottom + 4.0f, btnBounds.left + 250.0f, btnBounds.bottom + 4.0f + 150.0f); // Default sizes
                                ui::components::PopupManager::Instance().ShowPopup(popup, popupBounds, btnBounds);
            } else {
                ui::components::PopupManager::Instance().RequestHide();
            }
        };
    };

    setupPopup(btnHighlight, std::make_shared<ui::components::HighlightPopup>());
    setupPopup(btnArea, std::make_shared<ui::components::AreaHighlightPopup>());
    setupPopup(btnPencil, std::make_shared<ui::components::PencilPopup>());
    setupPopup(btnEraser, std::make_shared<ui::components::EraserPopup>());
    setupPopup(btnShapes, std::make_shared<ui::components::ShapesPopup>());

    // Tools / Misc mode
    AddButton(L"Create PDF",  controls::IconType::PDFDocument, true, true, 1, {AppMode::Tools});
    AddButton(L"Compress",    controls::IconType::Compress, true, false, 1, {AppMode::Tools});
    AddButton(L"Batch PDFs",  controls::IconType::Batch, true, true, 1, {AppMode::Tools});

    m_mode = AppMode::Edit; // Default to Edit to show the faithful toolbar
    ApplyModeVisibility();
}

std::shared_ptr<controls::IconButton> Toolbar::AddButton(
    const std::wstring& text, controls::IconType icon, bool isText,
    bool enabled, int group, std::vector<app::AppMode> modes) {
    auto btn = std::make_shared<controls::IconButton>(isText ? text : L"", icon);
    btn->SetShowText(isText);
    // Use fully transparent color for normal state, and a hover color (like SurfaceElevated)
    btn->SetColors(D2D1::ColorF(0, 0.0f), design::Colors::SurfaceElevated, design::Colors::TextPrimary);
    btn->SetEnabled(enabled);
    
    btn->SetOnClick([this, text]() {
        if (onAction) onAction(text);
        ui::commands::CommandManager::Instance().ExecuteAction(text);
    });
    AddChild(btn);
    m_buttons[text] = btn;
    m_btns.push_back({ btn, std::move(modes), text, isText, group });
    return btn;
}

void Toolbar::ApplyModeVisibility() {
    for (auto& b : m_btns) {
        bool visible = std::find(b.modes.begin(), b.modes.end(), m_mode) != b.modes.end();
        b.button->SetVisible(visible);
    }
}

void Toolbar::SetMode(app::AppMode mode) {
    m_mode = mode;
    ApplyModeVisibility();
    if (m_bounds.right > m_bounds.left) {
        Layout(m_bounds);
    }
}

void Toolbar::SetActiveTool(const std::wstring& toolName) {
    for (auto& pair : m_buttons) {
        pair.second->SetActive(pair.first == toolName);
    }
}

void Toolbar::Layout(const D2D1_RECT_F& bounds) {
    UIElement::Layout(bounds);

    float bx = bounds.left + 16.0f;
    float rx = bounds.right - 16.0f;
    float by = bounds.top + 8.0f;
    float bh = (bounds.bottom - bounds.top) - 16.0f;

    for (int i = (int)m_btns.size() - 1; i >= 0; --i) {
        auto& b = m_btns[i];
        if (!b.button || !b.button->IsVisible()) continue;
        bool isRight = (b.group == 2);
        if (isRight) {
            float bw = b.isText ? (wcslen(b.label.c_str()) * 8.0f + 48.0f) : 40.0f;
            b.button->Layout({rx - bw, by, rx, by + bh});
            rx -= (bw + 8.0f);
        }
    }

    for (auto& b : m_btns) {
        if (!b.button || !b.button->IsVisible()) continue;
        if (b.group == 2) continue;

        float bw = b.isText ? (wcslen(b.label.c_str()) * 8.0f + 48.0f) : 40.0f;
        b.button->Layout({bx, by, bx + bw, by + bh});
        bx += bw + 8.0f;
    }
}

void Toolbar::Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) {
    framework::Panel::Render(target);
    
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> borderBrush;
    if (SUCCEEDED(target->CreateSolidColorBrush(design::Colors::BorderSubtle, &borderBrush))) {
        target->DrawLine(D2D1::Point2F(m_bounds.left, m_bounds.bottom), D2D1::Point2F(m_bounds.right, m_bounds.bottom), borderBrush.Get(), 1.0f);
    }
}

Toolbar::~Toolbar() {
    ui::commands::CommandManager::Instance().RemoveObserver(this);
}

void Toolbar::OnActionStateChanged(const std::wstring& actionId, bool isEnabled, bool isChecked) {
    auto it = m_buttons.find(actionId);
    if (it != m_buttons.end()) {
        it->second->SetEnabled(isEnabled);
        it->second->SetActive(isChecked); // Assuming SetActive is the correct method for "isChecked" based on SetActiveTool
    }
}

} // namespace components






