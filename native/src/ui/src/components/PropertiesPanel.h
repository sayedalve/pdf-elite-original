#pragma once
#include "../framework/Panel.h"
#include <string>
#include <functional>
#include <dwrite_3.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace ui {
namespace interaction {
    class ISelectableObject;
}
}

namespace components {

class PropertiesPanel : public framework::Panel {
public:
    PropertiesPanel();
    
    void Layout(const D2D1_RECT_F& bounds) override;
    void Render(ComPtr<ID2D1RenderTarget> target) override;

    void SetSelectedObject(std::shared_ptr<ui::interaction::ISelectableObject> obj);
    
    std::function<void(const std::wstring& text)> onTextChanged;
    std::function<void(float size)> onFontSizeChanged;
    std::function<void(uint8_t r, uint8_t g, uint8_t b)> onColorChanged;

private:
    void EnsureFormat();

    std::shared_ptr<ui::interaction::ISelectableObject> m_selectedObj;
    ComPtr<IDWriteTextFormat> m_formatHeader;
    ComPtr<IDWriteTextFormat> m_formatLabel;
    
    // Very simple state for the prototype
    std::wstring m_currentText;
    float m_currentFontSize = 0.0f;
    uint8_t m_colorR = 0, m_colorG = 0, m_colorB = 0;
};

} // namespace components
