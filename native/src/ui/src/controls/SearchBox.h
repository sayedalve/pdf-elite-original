#pragma once
#include "../framework/UIElement.h"
#include <string>
#include <functional>
#include "IconRenderer.h"

namespace controls {

class SearchBox : public framework::UIElement {
public:
    SearchBox();

    void Render(ComPtr<ID2D1RenderTarget> target) override;

    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    void OnMouseLeave() override;

private:
    bool m_isHovered = false;
    bool m_isFocused = false;
    std::wstring m_placeholder = L"Search PDF Elite...";
};

} // namespace controls
