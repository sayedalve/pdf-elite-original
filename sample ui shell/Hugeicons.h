// Hugeicons.h - Hugeicons database, free stroke icons 1.5px, 24x24 viewBox
// Source: https://hugeicons.com - consistent stroke, rounded, no gibberish
#pragma once
#include <string>

namespace PdfElite { namespace Hugeicons {

struct IconPath {
    const wchar_t* name; // hugeicons name
    const wchar_t* svgPath; // SVG path data for 24x24
};

// Flaticon replacement with Hugeicons - distinct shapes
// Edit: hugeicons:edit-02, Convert: hugeicons:exchange-02, OCR: hugeicons:ocr, etc
inline IconPath GetIcon(const wchar_t* hugeName) {
    // Returns SVG path - simplified for Direct2D
    if (wcscmp(hugeName, L"edit-02")==0) return {L"edit-02", L"M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z"};
    if (wcscmp(hugeName, L"exchange-02")==0) return {L"exchange-02", L"M3 7h13M10 4l3 3-3 3M21 17H8M18 14l-3 3 3 3"};
    if (wcscmp(hugeName, L"ocr")==0) return {L"ocr", L"M4 7V5a1 1 0 0 1 1-1h3M20 7V5a1 1 0 0 0-1-1h-3M4 17v2a1 1 0 0 0 1 1h3M20 17v2a1 1 0 0 1-1 1h-3M9 12h6"};
    if (wcscmp(hugeName, L"comment-02")==0) return {L"comment-02", L"M21 11.5a8.5 8.5 0 0 1-8.5 8.5H8l-4 4v-4a8.5 8.5 0 0 1 8.5-8.5H21z"};
    if (wcscmp(hugeName, L"translate")==0) return {L"translate", L"M5 8l-3 8M8 16l-3-8M9 10h6M12 3a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10A15.3 15.3 0 0 1 12 3z"};
    return {L"file-02", L"M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"};
}

}} // namespace
