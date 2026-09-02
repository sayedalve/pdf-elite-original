#include "ui/layout/TextLayout.h"
#include <algorithm>
#include <cmath>

namespace ui {
namespace layout {

TextLayout::TextLayout() {}

TextLayout::~TextLayout() {}

void TextLayout::Initialize(Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory) {
    m_dwriteFactory = dwriteFactory;
}

void TextLayout::SetText(const std::wstring& text) {
    m_text = text;
}

void TextLayout::SetFont(const std::wstring& fontFamily, float fontSize) {
    m_fontFamily = fontFamily;
    m_fontSize = fontSize;
}

void TextLayout::SetFontStyle(bool isBold, bool isItalic) {
    m_fontWeight = isBold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
    m_fontStyle = isItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
}

void TextLayout::SetBounds(const D2D1_RECT_F& bounds) {
    m_bounds = bounds;
}

void TextLayout::UpdateLayout() {
    m_textLayout.Reset();
    m_textFormat.Reset();
    
    if (!m_dwriteFactory) return;
    
    m_dwriteFactory->CreateTextFormat(
        m_fontFamily.c_str(),
        nullptr,
        m_fontWeight,
        m_fontStyle,
        DWRITE_FONT_STRETCH_NORMAL,
        m_fontSize,
        L"en-us",
        &m_textFormat
    );
    
    if (!m_textFormat) return;
    
    float width = m_bounds.right - m_bounds.left;
    if (width <= 0) width = 1000.0f;
    float height = m_bounds.bottom - m_bounds.top;
    if (height <= 0) height = 1000.0f;
    
    m_dwriteFactory->CreateTextLayout(
        m_text.c_str(),
        static_cast<UINT32>(m_text.length()),
        m_textFormat.Get(),
        width,
        height,
        &m_textLayout
    );
}

float TextLayout::GetMeasuredWidth() const {
    if (!m_textLayout) return 0.0f;
    DWRITE_TEXT_METRICS metrics;
    m_textLayout->GetMetrics(&metrics);
    return metrics.widthIncludingTrailingWhitespace;
}

float TextLayout::GetMeasuredHeight() const {
    if (!m_textLayout) return 0.0f;
    DWRITE_TEXT_METRICS metrics;
    m_textLayout->GetMetrics(&metrics);
    return metrics.height;
}

std::vector<TextLine> TextLayout::GetLines() const {
    std::vector<TextLine> lines;
    if (!m_textLayout) {
        // Headless mode fallback
        TextLine line;
        line.text = m_text;
        line.x = 0;
        line.y = 0;
        line.width = 100;
        line.height = m_fontSize;
        if (!line.text.empty()) {
            lines.push_back(line);
        }
        return lines;
    }
    
    UINT32 lineCount = 0;
    m_textLayout->GetLineMetrics(nullptr, 0, &lineCount);
    if (lineCount == 0) return lines;
    
    std::vector<DWRITE_LINE_METRICS> lineMetrics(lineCount);
    m_textLayout->GetLineMetrics(lineMetrics.data(), lineCount, &lineCount);
    
    UINT32 textPosition = 0;
    float currentY = 0;
    
    for (UINT32 i = 0; i < lineCount; ++i) {
        TextLine line;
        line.text = m_text.substr(textPosition, lineMetrics[i].length);
        
        // Remove trailing carriage return / newline from the substring if present
        while (!line.text.empty() && (line.text.back() == L'\n' || line.text.back() == L'\r')) {
            line.text.pop_back();
        }
        
        line.y = currentY;
        line.height = lineMetrics[i].height;
        
        // Find x and width for this line by hit testing its range
        UINT32 hitTestCount = 0;
        m_textLayout->HitTestTextRange(textPosition, lineMetrics[i].length, 0, 0, nullptr, 0, &hitTestCount);
        if (hitTestCount > 0) {
            std::vector<DWRITE_HIT_TEST_METRICS> hitMetrics(hitTestCount);
            m_textLayout->HitTestTextRange(textPosition, lineMetrics[i].length, 0, 0, hitMetrics.data(), hitTestCount, &hitTestCount);
            
            float minX = 99999.0f;
            float maxX = -99999.0f;
            for (UINT32 j = 0; j < hitTestCount; ++j) {
                if (hitMetrics[j].top >= currentY && hitMetrics[j].top < currentY + line.height) {
                    minX = std::min(minX, hitMetrics[j].left);
                    maxX = std::max(maxX, hitMetrics[j].left + hitMetrics[j].width);
                }
            }
            line.x = minX == 99999.0f ? 0 : minX;
            line.width = maxX == -99999.0f ? 0 : (maxX - minX);
        } else {
            line.x = 0;
            line.width = 0;
        }
        
        lines.push_back(line);
        
        textPosition += lineMetrics[i].length;
        currentY += lineMetrics[i].height;
    }
    
    return lines;
}

bool TextLayout::HitTestPoint(float x, float y, int& textPosition, bool& isTrailingHit) const {
    if (!m_textLayout) return false;
    
    BOOL trailingHit;
    BOOL isInside;
    DWRITE_HIT_TEST_METRICS metrics;
    
    HRESULT hr = m_textLayout->HitTestPoint(x, y, &trailingHit, &isInside, &metrics);
    if (SUCCEEDED(hr)) {
        textPosition = metrics.textPosition;
        isTrailingHit = trailingHit;
        return true;
    }
    return false;
}

bool TextLayout::HitTestTextPosition(int textPosition, bool isTrailingHit, float& x, float& y, float& height) const {
    if (!m_textLayout) return false;
    
    DWRITE_HIT_TEST_METRICS metrics;
    HRESULT hr = m_textLayout->HitTestTextPosition(textPosition, isTrailingHit, &x, &y, &metrics);
    if (SUCCEEDED(hr)) {
        height = metrics.height;
        return true;
    }
    return false;
}

std::vector<D2D1_RECT_F> TextLayout::HitTestTextRange(int startPosition, int length) const {
    std::vector<D2D1_RECT_F> rects;
    if (!m_textLayout || length <= 0) return rects;
    
    UINT32 actualHitTestCount = 0;
    m_textLayout->HitTestTextRange(startPosition, length, 0, 0, nullptr, 0, &actualHitTestCount);
    
    if (actualHitTestCount > 0) {
        std::vector<DWRITE_HIT_TEST_METRICS> hitTestMetrics(actualHitTestCount);
        m_textLayout->HitTestTextRange(startPosition, length, 0, 0, hitTestMetrics.data(), actualHitTestCount, &actualHitTestCount);
        
        for (UINT32 i = 0; i < actualHitTestCount; ++i) {
            D2D1_RECT_F rect = {
                hitTestMetrics[i].left,
                hitTestMetrics[i].top,
                hitTestMetrics[i].left + hitTestMetrics[i].width,
                hitTestMetrics[i].top + hitTestMetrics[i].height
            };
            rects.push_back(rect);
        }
    }
    
    return rects;
}

} // namespace layout
} // namespace ui
