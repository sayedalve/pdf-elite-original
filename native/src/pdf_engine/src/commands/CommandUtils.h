#pragma once

#include <windows.h>
#include <wincodec.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cwctype>

namespace pdf_engine {
namespace commands {

inline std::string WideToNarrow(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";
    std::string str(static_cast<size_t>(len - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, str.data(), len, nullptr, nullptr);
    return str;
}

inline std::vector<int> ParsePageRange(int pageScope, const std::wstring& pageRangeStr, int currentPage, int totalPages) {
    std::vector<int> pages;
    if (totalPages <= 0) return pages;

    if (pageScope == 0) { // All pages
        pages.reserve(totalPages);
        for (int i = 0; i < totalPages; ++i) {
            pages.push_back(i);
        }
        return pages;
    }

    if (pageScope == 1) { // Current page
        int p = currentPage - 1;
        if (p >= 0 && p < totalPages) {
            pages.push_back(p);
        } else {
            pages.push_back(0);
        }
        return pages;
    }

    // Custom range
    if (pageRangeStr.empty() || pageRangeStr == L"all" || pageRangeStr == L"ALL") {
        pages.reserve(totalPages);
        for (int i = 0; i < totalPages; ++i) {
            pages.push_back(i);
        }
        return pages;
    }

    std::wstringstream ss(pageRangeStr);
    std::wstring token;
    while (std::getline(ss, token, L',')) {
        // Trim whitespace
        size_t start = token.find_first_not_of(L" \t\r\n");
        size_t end = token.find_last_not_of(L" \t\r\n");
        if (start == std::wstring::npos) continue;
        token = token.substr(start, end - start + 1);

        size_t dash = token.find(L'-');
        if (dash != std::wstring::npos) {
            std::wstring startStr = token.substr(0, dash);
            std::wstring endStr = token.substr(dash + 1);
            try {
                int s = std::stoi(startStr);
                int e = std::stoi(endStr);
                if (s > e) std::swap(s, e);
                for (int p = s; p <= e; ++p) {
                    if (p >= 1 && p <= totalPages) {
                        pages.push_back(p - 1);
                    }
                }
            } catch (...) {}
        } else {
            try {
                int p = std::stoi(token);
                if (p >= 1 && p <= totalPages) {
                    pages.push_back(p - 1);
                }
            } catch (...) {}
        }
    }

    std::sort(pages.begin(), pages.end());
    pages.erase(std::unique(pages.begin(), pages.end()), pages.end());

    if (pages.empty()) {
        pages.push_back(0);
    }
    return pages;
}

inline bool LoadImageToBgra(const std::wstring& imagePath, std::vector<uint8_t>& outData, int& outWidth, int& outHeight) {
    if (imagePath.empty()) return false;

    // Initialize COM for WIC if not already initialized
    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    IWICImagingFactory* pFactory = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pFactory));
    if (FAILED(hr) || !pFactory) {
        if (SUCCEEDED(hrCo)) CoUninitialize();
        return false;
    }

    IWICBitmapDecoder* pDecoder = nullptr;
    hr = pFactory->CreateDecoderFromFilename(imagePath.c_str(), nullptr, GENERIC_READ,
                                            WICDecodeMetadataCacheOnDemand, &pDecoder);
    if (FAILED(hr) || !pDecoder) {
        pFactory->Release();
        if (SUCCEEDED(hrCo)) CoUninitialize();
        return false;
    }

    IWICBitmapFrameDecode* pFrame = nullptr;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr) || !pFrame) {
        pDecoder->Release();
        pFactory->Release();
        if (SUCCEEDED(hrCo)) CoUninitialize();
        return false;
    }

    UINT width = 0, height = 0;
    pFrame->GetSize(&width, &height);
    if (width == 0 || height == 0) {
        pFrame->Release();
        pDecoder->Release();
        pFactory->Release();
        if (SUCCEEDED(hrCo)) CoUninitialize();
        return false;
    }

    IWICFormatConverter* pConverter = nullptr;
    hr = pFactory->CreateFormatConverter(&pConverter);
    if (FAILED(hr) || !pConverter) {
        pFrame->Release();
        pDecoder->Release();
        pFactory->Release();
        if (SUCCEEDED(hrCo)) CoUninitialize();
        return false;
    }

    hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppBGRA,
                                WICBitmapDitherTypeNone, nullptr, 0.0,
                                WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        pConverter->Release();
        pFrame->Release();
        pDecoder->Release();
        pFactory->Release();
        if (SUCCEEDED(hrCo)) CoUninitialize();
        return false;
    }

    UINT stride = width * 4;
    UINT bufferSize = stride * height;
    outData.resize(bufferSize);

    hr = pConverter->CopyPixels(nullptr, stride, bufferSize, outData.data());

    pConverter->Release();
    pFrame->Release();
    pDecoder->Release();
    pFactory->Release();
    if (SUCCEEDED(hrCo)) CoUninitialize();

    if (FAILED(hr)) {
        outData.clear();
        return false;
    }

    outWidth = static_cast<int>(width);
    outHeight = static_cast<int>(height);
    return true;
}

} // namespace commands
} // namespace pdf_engine
