#include "TextEditor.h"
#include "../GraphicsDevice.h"
#include <vector>
#include <algorithm>

namespace ui {
namespace controls {

TextEditor::TextEditor() {}

TextEditor::~TextEditor() {}

void TextEditor::Initialize(HWND parentHwnd) {
    m_hwnd = parentHwnd;
}

void TextEditor::SetText(const std::wstring& text) {
    m_text = text;
    m_layout.SetText(text);
    m_caretPosition = static_cast<int>(m_text.length());
    m_selectionStart = m_caretPosition;
    UpdateLayout();
}

std::wstring TextEditor::GetText() const {
    return m_text;
}

std::vector<ui::layout::TextLine> TextEditor::GetLines() const {
    return m_layout.GetLines();
}

void TextEditor::SetFont(const std::wstring& fontFamily, float fontSize) {
    m_fontFamily = fontFamily;
    m_fontSize = fontSize;
    m_layout.SetFont(fontFamily, fontSize);
    UpdateLayout();
}

void TextEditor::ToggleBold() {
    m_isBold = !m_isBold;
    m_layout.SetFontStyle(m_isBold, m_isItalic);
    UpdateLayout();
}

void TextEditor::ToggleItalic() {
    m_isItalic = !m_isItalic;
    m_layout.SetFontStyle(m_isBold, m_isItalic);
    UpdateLayout();
}

void TextEditor::SetColor(D2D1_COLOR_F color) {
    m_color = color;
}

void TextEditor::SetBounds(const D2D1_RECT_F& bounds) {
    m_bounds = bounds;
    m_layout.SetBounds(bounds);
    UpdateLayout();
}

void TextEditor::UpdateLayout() {
    m_layout.SetText(m_text);
    m_layout.SetFont(m_fontFamily, m_fontSize);
    m_layout.SetFontStyle(m_isBold, m_isItalic);
    m_layout.SetBounds(m_bounds);
    
    auto dwriteFactory = GraphicsDevice::Instance().GetDWriteFactory();
    if (!dwriteFactory) return;
    
    m_layout.UpdateLayout();
    UpdateCaretPosition();
}

void TextEditor::UpdateCaretPosition() {
    if (!m_layout.GetDWriteLayout()) return;
    
    float x = 0, y = 0;
    if (m_layout.HitTestTextPosition(m_caretPosition, false, x, y, m_caretHeight)) {
        m_caretPt.x = m_bounds.left + x;
        m_caretPt.y = m_bounds.top + y;
    }
}

void TextEditor::Render(ID2D1RenderTarget* renderTarget) {
    if (!m_isActive || !m_layout.GetDWriteLayout()) return;
    
    // Save previous transform and apply text object's native matrix
    D2D1_MATRIX_3X2_F oldTransform;
    renderTarget->GetTransform(&oldTransform);
    
    // Combine old transform (viewport/zoom) with the text object's transform
    D2D1_MATRIX_3X2_F combinedTransform = m_transform * oldTransform;
    renderTarget->SetTransform(combinedTransform);

    ComPtr<ID2D1SolidColorBrush> brush;
    renderTarget->CreateSolidColorBrush(m_color, &brush);
    
    D2D1_POINT_2F origin = {m_bounds.left, m_bounds.top};
    
    // Draw selection background
    if (m_selectionStart != m_caretPosition) {
        int start = std::min(m_selectionStart, m_caretPosition);
        int length = std::abs(m_selectionStart - m_caretPosition);
        
        auto rects = m_layout.HitTestTextRange(start, length);
        if (!rects.empty()) {
            ComPtr<ID2D1SolidColorBrush> selectionBrush;
            renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.5f, 1.0f, 0.3f), &selectionBrush);
            
            for (const auto& rect : rects) {
                D2D1_RECT_F selectionRect = {
                    origin.x + rect.left,
                    origin.y + rect.top,
                    origin.x + rect.right,
                    origin.y + rect.bottom
                };
                renderTarget->FillRectangle(selectionRect, selectionBrush.Get());
            }
        }
    }
    
    // Draw text
    renderTarget->DrawTextLayout(origin, m_layout.GetDWriteLayout().Get(), brush.Get());
    
    // Draw caret (blinking could be added via a timer)
    ComPtr<ID2D1SolidColorBrush> caretBrush;
    renderTarget->CreateSolidColorBrush(m_caretColor, &caretBrush);
    
    D2D1_RECT_F caretRect = {
        m_caretPt.x,
        m_caretPt.y,
        m_caretPt.x + 1.5f,
        m_caretPt.y + m_caretHeight
    };
    renderTarget->FillRectangle(caretRect, caretBrush.Get());
    
    // Restore the viewport transform
    renderTarget->SetTransform(oldTransform);
}

void TextEditor::SetActive(bool active) {
    m_isActive = active;
    if (m_isActive) {
        UpdateLayout();
    }
}

void TextEditor::MoveCaret(int offset, bool shiftPressed) {
    m_caretPosition += offset;
    m_caretPosition = std::max(0, std::min(m_caretPosition, static_cast<int>(m_text.length())));
    
    if (!shiftPressed) {
        m_selectionStart = m_caretPosition;
    }
    UpdateCaretPosition();
}

void TextEditor::DeleteSelection() {
    if (m_selectionStart != m_caretPosition) {
        int start = std::min(m_selectionStart, m_caretPosition);
        int length = std::abs(m_selectionStart - m_caretPosition);
        m_text.erase(start, length);
        m_caretPosition = start;
        m_selectionStart = start;
    }
}

bool TextEditor::OnKeyDown(WPARAM wParam, bool shiftPressed, bool ctrlPressed) {
    if (!m_isActive) return false;
    
    if (ctrlPressed) {
        if (wParam == 'A') {
            m_selectionStart = 0;
            m_caretPosition = static_cast<int>(m_text.length());
            UpdateCaretPosition();
            return true;
        } else if (wParam == 'B') {
            ToggleBold();
            return true;
        } else if (wParam == 'I') {
            ToggleItalic();
            return true;
        } else if (wParam == 'C') {
            if (m_selectionStart != m_caretPosition) {
                int start = std::min(m_selectionStart, m_caretPosition);
                int length = std::abs(m_selectionStart - m_caretPosition);
                std::wstring selectedText = m_text.substr(start, length);
                if (OpenClipboard(m_hwnd)) {
                    EmptyClipboard();
                    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, (selectedText.length() + 1) * sizeof(wchar_t));
                    if (hGlob) {
                        memcpy(GlobalLock(hGlob), selectedText.c_str(), (selectedText.length() + 1) * sizeof(wchar_t));
                        GlobalUnlock(hGlob);
                        SetClipboardData(CF_UNICODETEXT, hGlob);
                    }
                    CloseClipboard();
                }
            }
            return true;
        } else if (wParam == 'X') {
            if (m_selectionStart != m_caretPosition) {
                int start = std::min(m_selectionStart, m_caretPosition);
                int length = std::abs(m_selectionStart - m_caretPosition);
                std::wstring selectedText = m_text.substr(start, length);
                if (OpenClipboard(m_hwnd)) {
                    EmptyClipboard();
                    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, (selectedText.length() + 1) * sizeof(wchar_t));
                    if (hGlob) {
                        memcpy(GlobalLock(hGlob), selectedText.c_str(), (selectedText.length() + 1) * sizeof(wchar_t));
                        GlobalUnlock(hGlob);
                        SetClipboardData(CF_UNICODETEXT, hGlob);
                    }
                    CloseClipboard();
                }
                DeleteSelection();
                UpdateLayout();
            }
            return true;
        } else if (wParam == 'V') {
            if (OpenClipboard(m_hwnd)) {
                HANDLE hData = GetClipboardData(CF_UNICODETEXT);
                if (hData) {
                    wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
                    if (pszText) {
                        std::wstring pastedText(pszText);
                        GlobalUnlock(hData);
                        DeleteSelection();
                        m_text.insert(m_caretPosition, pastedText);
                        m_caretPosition += static_cast<int>(pastedText.length());
                        m_selectionStart = m_caretPosition;
                        UpdateLayout();
                    }
                }
                CloseClipboard();
            }
            return true;
        }
    }
    
    switch (wParam) {
        case VK_LEFT:
            MoveCaret(-1, shiftPressed);
            return true;
        case VK_RIGHT:
            MoveCaret(1, shiftPressed);
            return true;
        case VK_UP: {
            if (m_layout.GetDWriteLayout()) {
                float x = m_caretPt.x - m_bounds.left;
                float y = m_caretPt.y - m_bounds.top - m_caretHeight * 0.5f; // half a line up
                bool isTrailingHit;
                if (m_layout.HitTestPoint(x, y, m_caretPosition, isTrailingHit)) {
                    if (!shiftPressed) m_selectionStart = m_caretPosition;
                    UpdateCaretPosition();
                }
            }
            return true;
        }
        case VK_DOWN: {
            if (m_layout.GetDWriteLayout()) {
                float x = m_caretPt.x - m_bounds.left;
                float y = m_caretPt.y - m_bounds.top + m_caretHeight * 1.5f; // line down
                bool isTrailingHit;
                if (m_layout.HitTestPoint(x, y, m_caretPosition, isTrailingHit)) {
                    if (!shiftPressed) m_selectionStart = m_caretPosition;
                    UpdateCaretPosition();
                }
            }
            return true;
        }
        case VK_HOME:
            m_caretPosition = 0;
            if (!shiftPressed) m_selectionStart = m_caretPosition;
            UpdateCaretPosition();
            return true;
        case VK_END:
            m_caretPosition = static_cast<int>(m_text.length());
            if (!shiftPressed) m_selectionStart = m_caretPosition;
            UpdateCaretPosition();
            return true;
        case VK_DELETE:
            if (m_selectionStart != m_caretPosition) {
                DeleteSelection();
            } else if (m_caretPosition < static_cast<int>(m_text.length())) {
                m_text.erase(m_caretPosition, 1);
            }
            UpdateLayout();
            return true;
        case VK_BACK:
            if (m_selectionStart != m_caretPosition) {
                DeleteSelection();
            } else if (m_caretPosition > 0) {
                m_text.erase(m_caretPosition - 1, 1);
                m_caretPosition--;
                m_selectionStart = m_caretPosition;
            }
            UpdateLayout();
            return true;
        case VK_RETURN:
            DeleteSelection();
            m_text.insert(m_caretPosition, 1, L'\n');
            m_caretPosition++;
            m_selectionStart = m_caretPosition;
            UpdateLayout();
            return true;
    }
    return false;
}

bool TextEditor::OnChar(WPARAM wParam) {
    if (!m_isActive) return false;
    
    // Ignore control characters (including Enter/Backspace which are handled in OnKeyDown)
    if (wParam >= 32) { 
        DeleteSelection();
        m_text.insert(m_caretPosition, 1, static_cast<wchar_t>(wParam));
        m_caretPosition++;
        m_selectionStart = m_caretPosition;
        UpdateLayout();
        return true;
    }
    return false;
}

bool TextEditor::OnLButtonDown(double x, double y, bool shiftPressed) {
    if (!m_isActive || !m_layout.GetDWriteLayout()) return false;
    
    bool isTrailingHit;
    if (m_layout.HitTestPoint(
        static_cast<float>(x - m_bounds.left),
        static_cast<float>(y - m_bounds.top),
        m_caretPosition,
        isTrailingHit
    )) {
        if (!shiftPressed) {
            m_selectionStart = m_caretPosition;
        }
        UpdateCaretPosition();
    }
    return true;
}

bool TextEditor::OnMouseMove(double x, double y) {
    (void)x;
    (void)y;
    if (!m_isActive || !m_layout.GetDWriteLayout()) return false;
    return false;
}

bool TextEditor::OnLButtonUp(double x, double y) {
    (void)x;
    (void)y;
    if (!m_isActive) return false;
    return false;
}

} // namespace controls
} // namespace ui
