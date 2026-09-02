#include "Clipboard.h"

namespace core {

bool Clipboard::SetText(HWND hwnd, const std::wstring& text) {
    bool opened = false;
    for (int i = 0; i < 10; ++i) {
        if (OpenClipboard(hwnd)) {
            opened = true;
            break;
        }
        Sleep(5);
    }
    if (!opened) return false;
    EmptyClipboard();

    size_t cbStr = (text.length() + 1) * sizeof(wchar_t);
    HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE, cbStr);
    if (!hData) {
        CloseClipboard();
        return false;
    }

    memcpy(GlobalLock(hData), text.c_str(), cbStr);
    GlobalUnlock(hData);
    SetClipboardData(CF_UNICODETEXT, hData);
    CloseClipboard();
    return true;
}

std::wstring Clipboard::GetText(HWND hwnd) {
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) return L"";
    bool opened = false;
    for (int i = 0; i < 10; ++i) {
        if (OpenClipboard(hwnd)) {
            opened = true;
            break;
        }
        Sleep(5);
    }
    if (!opened) return L"";

    std::wstring result;
    HGLOBAL hData = GetClipboardData(CF_UNICODETEXT);
    if (hData) {
        wchar_t* pText = static_cast<wchar_t*>(GlobalLock(hData));
        if (pText) {
            result = pText;
            GlobalUnlock(hData);
        }
    }
    CloseClipboard();
    return result;
}

} // namespace core
