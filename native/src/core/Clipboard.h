#pragma once
#include <string>
#include <windows.h>

namespace core {

class Clipboard {
public:
    static bool SetText(HWND hwnd, const std::wstring& text);
    static std::wstring GetText(HWND hwnd);
};

} // namespace core
