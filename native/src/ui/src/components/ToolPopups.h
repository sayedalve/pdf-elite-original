#pragma once
#include "../framework/Panel.h"
#include "../controls/IconButton.h"
#include <memory>
#include <vector>
#include <string>

namespace ui::components {

class BaseToolPopup : public framework::Panel {
public:
    BaseToolPopup(const std::wstring& title, const std::wstring& shortcut, const std::wstring& desc);
    void Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) override;

protected:
    std::wstring m_title;
    std::wstring m_shortcut;
    std::wstring m_desc;
};

class HighlightPopup : public BaseToolPopup {
public:
    HighlightPopup();
};

class AreaHighlightPopup : public BaseToolPopup {
public:
    AreaHighlightPopup();
};

class PencilPopup : public BaseToolPopup {
public:
    PencilPopup();
};

class EraserPopup : public BaseToolPopup {
public:
    EraserPopup();
};

class ShapesPopup : public BaseToolPopup {
public:
    ShapesPopup();
};

}
