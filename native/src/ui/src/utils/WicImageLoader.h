#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace ui {
namespace utils {

class WicImageLoader {
public:
    static bool Initialize();
    static void Uninitialize();

    // Loads an image from a file, decodes it, and converts it to a BGRA buffer.
    // Returns true if successful. width and height are populated.
    static bool LoadImageFromFile(const std::wstring& path, std::vector<uint8_t>& outData, int& outWidth, int& outHeight);
    
    // Loads an image from clipboard (if available) into a BGRA buffer.
    static bool LoadImageFromClipboard(HWND hwnd, std::vector<uint8_t>& outData, int& outWidth, int& outHeight);
    static bool SaveImageToFile(const std::wstring& path, const std::vector<uint8_t>& bgraData, int width, int height);
};

} // namespace utils
} // namespace ui

