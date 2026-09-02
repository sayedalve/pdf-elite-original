#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <wrl/client.h>

namespace ui {
namespace layout {

struct TextLine {
    std::wstring text;
    float x;
    float y;
    float width;
    float height;
};

class TextLayout {
public:
    TextLayout();
    ~TextLayout();

    void Initialize(Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory);
    
    void SetText(const std::wstring& text);
    std::wstring GetText() const { return m_text; }
    
    void SetFont(const std::wstring& fontFamily, float fontSize);
    void SetFontStyle(bool isBold, bool isItalic);
    void SetBounds(const D2D1_RECT_F& bounds);
    
    void UpdateLayout();
    
    float GetMeasuredWidth() const;
    float GetMeasuredHeight() const;
    
    std::vector<TextLine> GetLines() const;
    
    bool HitTestPoint(float x, float y, int& textPosition, bool& isTrailingHit) const;
    bool HitTestTextPosition(int textPosition, bool isTrailingHit, float& x, float& y, float& height) const;
    std::vector<D2D1_RECT_F> HitTestTextRange(int startPosition, int length) const;
    
    Microsoft::WRL::ComPtr<IDWriteTextLayout> GetDWriteLayout() const { return m_textLayout; }

private:
    Microsoft::WRL::ComPtr<IDWriteFactory> m_dwriteFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
    Microsoft::WRL::ComPtr<IDWriteTextLayout> m_textLayout;
    
    std::wstring m_text;
    std::wstring m_fontFamily = L"Arial";
    float m_fontSize = 12.0f;
    DWRITE_FONT_WEIGHT m_fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
    DWRITE_FONT_STYLE m_fontStyle = DWRITE_FONT_STYLE_NORMAL;
    D2D1_RECT_F m_bounds = {0, 0, 1000.0f, 1000.0f};
};

} // namespace layout
} // namespace ui
