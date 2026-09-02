#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <fpdfview.h>
#include <fpdf_edit.h>

namespace core {
namespace interfaces {
namespace dom {

class FontManager {
public:
    FontManager();
    ~FontManager();

    // Initialize DirectWrite
    bool Initialize();

    // Check if the fallback font or current font has glyphs for the text
    bool HasGlyphs(const std::wstring& text);

    // Returns the raw TTF/TTC file data for the fallback font (Nirmala UI)
    std::vector<uint8_t> GetFallbackFontData();

    // Loads the fallback font into the document
    FPDF_FONT LoadFallbackFont(FPDF_DOCUMENT doc);

private:
    struct Impl;
    Impl* pImpl;
};

} // namespace dom
} // namespace interfaces
} // namespace core
