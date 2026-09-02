#include "NativeDesignSystem.h"

namespace design {

FontManager& FontManager::Instance() {
    static FontManager instance;
    return instance;
}

void FontManager::Initialize(IDWriteFactory* writeFactory) {
    if (!writeFactory) return;

    // Use "Segoe UI Variable" if available, fallback to "Segoe UI"
    const wchar_t* fontFamily = L"Segoe UI";
    
    // Application Title: Large, SemiBold
    writeFactory->CreateTextFormat(
        fontFamily, nullptr, 
        DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
        TypographySize::AppTitle, L"en-us", &m_appTitle
    );

    // Page Title: Medium-Large, SemiBold
    writeFactory->CreateTextFormat(
        fontFamily, nullptr, 
        DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
        TypographySize::PageTitle, L"en-us", &m_pageTitle
    );

    // Section Heading: Medium, Bold
    writeFactory->CreateTextFormat(
        fontFamily, nullptr, 
        DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
        TypographySize::SectionHeading, L"en-us", &m_sectionHeading
    );

    // Body: Normal
    writeFactory->CreateTextFormat(
        fontFamily, nullptr, 
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
        TypographySize::Body, L"en-us", &m_body
    );

    // Secondary: Small, Normal
    writeFactory->CreateTextFormat(
        fontFamily, nullptr, 
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
        TypographySize::Secondary, L"en-us", &m_secondary
    );

    // Toolbar: Small, Medium
    writeFactory->CreateTextFormat(
        fontFamily, nullptr, 
        DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
        TypographySize::Toolbar, L"en-us", &m_toolbar
    );
    if (m_toolbar) {
        m_toolbar->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_toolbar->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    // Navigation: Normal, Medium
    writeFactory->CreateTextFormat(
        fontFamily, nullptr, 
        DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
        TypographySize::Navigation, L"en-us", &m_navigation
    );
    if (m_navigation) {
        Microsoft::WRL::ComPtr<IDWriteInlineObject> trimmingSign;
        writeFactory->CreateEllipsisTrimmingSign(m_navigation.Get(), &trimmingSign);
        DWRITE_TRIMMING trimming = {DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
        m_navigation->SetTrimming(&trimming, trimmingSign.Get());
        m_navigation->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    }

    // Metadata: XSmall, Normal
    writeFactory->CreateTextFormat(
        fontFamily, nullptr, 
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
        TypographySize::Metadata, L"en-us", &m_metadata
    );

    // Caption: XXSmall, Medium
    writeFactory->CreateTextFormat(
        fontFamily, nullptr, 
        DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
        TypographySize::Caption, L"en-us", &m_caption
    );
    
    // Tooltip: XSmall, Normal
    writeFactory->CreateTextFormat(
        fontFamily, nullptr, 
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 
        TypographySize::Tooltip, L"en-us", &m_tooltip
    );
}

} // namespace design
