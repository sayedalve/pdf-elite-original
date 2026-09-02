#pragma once
#include <string>
#include <windows.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include "ui/layout/TextLayout.h"

using Microsoft::WRL::ComPtr;

namespace ui {
namespace controls {

class TextEditor {
public:
    TextEditor();
    ~TextEditor();
    
    void Initialize(HWND parentHwnd);
    
    void SetText(const std::wstring& text);
    std::wstring GetText() const;
    std::vector<ui::layout::TextLine> GetLines() const;
    
    void SetFont(const std::wstring& fontFamily, float fontSize);
    void ToggleBold();
    void ToggleItalic();
    
    void SetColor(D2D1_COLOR_F color);
    void SetCaretColor(D2D1_COLOR_F color) { m_caretColor = color; }
    
    void SetBounds(const D2D1_RECT_F& bounds);
    D2D1_RECT_F GetBounds() const { return m_bounds; }
    
    void SetTransform(const D2D1_MATRIX_3X2_F& transform) { m_transform = transform; }
    D2D1_MATRIX_3X2_F GetTransform() const { return m_transform; }
    
    void Render(ID2D1RenderTarget* renderTarget);
    
    bool OnKeyDown(WPARAM wParam, bool shiftPressed, bool ctrlPressed);
    bool OnChar(WPARAM wParam);
    bool OnLButtonDown(double x, double y, bool shiftPressed);
    bool OnMouseMove(double x, double y);
    bool OnLButtonUp(double x, double y);
    
    void SetActive(bool active);
    bool IsActive() const { return m_isActive; }

private:
    void UpdateLayout();
    void MoveCaret(int offset, bool shiftPressed);
    void DeleteSelection();
    void UpdateCaretPosition();

    HWND m_hwnd = nullptr;
    bool m_isActive = false;
    
    std::wstring m_text;
    std::wstring m_fontFamily = L"Arial";
    float m_fontSize = 12.0f;
    bool m_isBold = false;
    bool m_isItalic = false;
    D2D1_COLOR_F m_color = D2D1::ColorF(D2D1::ColorF::Black);
    D2D1_COLOR_F m_caretColor = D2D1::ColorF(D2D1::ColorF::Black);
    
    D2D1_RECT_F m_bounds = {0, 0, 0, 0};
    D2D1_MATRIX_3X2_F m_transform = D2D1::Matrix3x2F::Identity();
    
    ui::layout::TextLayout m_layout;
    
    int m_caretPosition = 0;
    int m_selectionStart = 0;
    D2D1_POINT_2F m_caretPt = {0, 0};
    float m_caretHeight = 0;
};

} // namespace controls
} // namespace ui
