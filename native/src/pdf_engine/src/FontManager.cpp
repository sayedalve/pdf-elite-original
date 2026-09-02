#include "core/interfaces/dom/FontManager.h"
#include <dwrite.h>
#include <wrl/client.h>
#include <fstream>
#include <iostream>

#pragma comment(lib, "dwrite.lib")

namespace core { namespace interfaces { namespace dom {

using Microsoft::WRL::ComPtr;

struct FontManager::Impl {
    ComPtr<IDWriteFactory> pDWriteFactory;
    ComPtr<IDWriteFontCollection> pFontCollection;
    ComPtr<IDWriteFontFace> pNirmalaFontFace;
    bool initialized = false;

    bool Initialize() {
        if (initialized) return true;

        HRESULT hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(pDWriteFactory.GetAddressOf())
        );

        if (FAILED(hr)) {
            return false;
        }

        hr = pDWriteFactory->GetSystemFontCollection(&pFontCollection);
        if (FAILED(hr)) {
            return false;
        }

        uint32_t index = 0;
        BOOL exists = FALSE;
        hr = pFontCollection->FindFamilyName(L"Nirmala UI", &index, &exists);
        if (SUCCEEDED(hr) && exists) {
            ComPtr<IDWriteFontFamily> pFontFamily;
            hr = pFontCollection->GetFontFamily(index, &pFontFamily);
            if (SUCCEEDED(hr)) {
                ComPtr<IDWriteFont> pFont;
                hr = pFontFamily->GetFirstMatchingFont(
                    DWRITE_FONT_WEIGHT_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    DWRITE_FONT_STYLE_NORMAL,
                    &pFont
                );
                if (SUCCEEDED(hr)) {
                    hr = pFont->CreateFontFace(&pNirmalaFontFace);
                    if (SUCCEEDED(hr)) {
                        initialized = true;
                        return true;
                    }
                }
            }
        }

        return false;
    }
};

FontManager::FontManager() : pImpl(new Impl()) {
}

FontManager::~FontManager() {
    delete pImpl;
}

bool FontManager::Initialize() {
    return pImpl->Initialize();
}

bool FontManager::HasGlyphs(const std::wstring& text) {
    if (!pImpl->initialized && !Initialize()) {
        // Fallback: check if we have font files available on disk
        return !GetFallbackFontData().empty();
    }

    if (!pImpl->pNirmalaFontFace) {
        return !GetFallbackFontData().empty();
    }

    for (size_t i = 0; i < text.length(); ++i) {
        uint32_t codepoint = text[i];
        
        // Handle surrogate pairs
        if (codepoint >= 0xD800 && codepoint <= 0xDBFF && i + 1 < text.length()) {
            uint32_t high = codepoint;
            uint32_t low = text[i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF) {
                codepoint = 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
                i++; // Skip the low surrogate
            }
        }

        uint16_t glyphIndex = 0;
        HRESULT hr = pImpl->pNirmalaFontFace->GetGlyphIndices(&codepoint, 1, &glyphIndex);
        if (FAILED(hr) || glyphIndex == 0) {
            // Check if fallback font data exists on disk to handle general Unicode / CJK
            return !GetFallbackFontData().empty();
        }
    }

    return true;
}

std::vector<uint8_t> FontManager::GetFallbackFontData() {
    const std::vector<std::string> fontPaths = {
        "C:\\Windows\\Fonts\\ARIALUNI.ttf",
        "C:\\Windows\\Fonts\\msyh.ttc",
        "C:\\Windows\\Fonts\\Nirmala.ttc",
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf"
    };

    for (const auto& path : fontPaths) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            std::streamsize size = file.tellg();
            if (size > 0) {
                file.seekg(0, std::ios::beg);
                std::vector<uint8_t> buffer(size);
                if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                    return buffer;
                }
            }
        }
    }
    return {};
}

FPDF_FONT FontManager::LoadFallbackFont(FPDF_DOCUMENT doc) {
    auto data = GetFallbackFontData();
    if (data.empty()) {
        return nullptr;
    }
    
    // Note: PDFium may copy the font data, but we pass the data pointer.
    // FPDFText_LoadFont takes cid=true for Unicode fonts
    return FPDFText_LoadFont(doc, data.data(), static_cast<uint32_t>(data.size()), FPDF_FONT_TRUETYPE, true);
}

} } }
