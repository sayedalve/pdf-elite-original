#include "WicImageLoader.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <iostream>

#pragma comment(lib, "windowscodecs.lib")

using Microsoft::WRL::ComPtr;

namespace ui {
namespace utils {

static ComPtr<IWICImagingFactory> g_wicFactory;

bool WicImageLoader::Initialize() {
    if (g_wicFactory) return true;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&g_wicFactory)
    );
    return SUCCEEDED(hr);
}

void WicImageLoader::Uninitialize() {
    g_wicFactory.Reset();
}

bool WicImageLoader::LoadImageFromFile(const std::wstring& path, std::vector<uint8_t>& outData, int& outWidth, int& outHeight) {
    if (!g_wicFactory) return false;

    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = g_wicFactory->CreateDecoderFromFilename(
        path.c_str(),
        NULL,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &decoder
    );
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) return false;

    ComPtr<IWICFormatConverter> converter;
    hr = g_wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr)) return false;

    hr = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone,
        NULL,
        0.f,
        WICBitmapPaletteTypeMedianCut
    );
    if (FAILED(hr)) return false;

    UINT width, height;
    hr = converter->GetSize(&width, &height);
    if (FAILED(hr)) return false;

    UINT cbStride = width * 4;
    UINT cbBufferSize = cbStride * height;
    outData.resize(cbBufferSize);

    hr = converter->CopyPixels(
        NULL,
        cbStride,
        cbBufferSize,
        outData.data()
    );

    if (SUCCEEDED(hr)) {
        outWidth = static_cast<int>(width);
        outHeight = static_cast<int>(height);
        return true;
    }
    return false;
}

bool WicImageLoader::LoadImageFromClipboard(HWND hwnd, std::vector<uint8_t>& outData, int& outWidth, int& outHeight) {
    if (!g_wicFactory) return false;

    if (!IsClipboardFormatAvailable(CF_DIB) && !IsClipboardFormatAvailable(CF_BITMAP)) {
        return false;
    }

    if (!OpenClipboard(hwnd)) return false;

    HANDLE hData = GetClipboardData(CF_DIB);
    if (!hData) {
        hData = GetClipboardData(CF_BITMAP);
    }
    
    if (!hData) {
        CloseClipboard();
        return false;
    }

    // Since converting raw DIB from clipboard to WIC can be slightly tricky directly,
    // we use IWICImagingFactory::CreateBitmapFromHBITMAP if it's an HBITMAP.
    // If it's a DIB, we can wrap it in an HBITMAP.
    // The easiest way is to let Windows do it if we ask for CF_BITMAP.
    HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
    if (!hBitmap) {
        CloseClipboard();
        return false;
    }

    ComPtr<IWICBitmap> wicBitmap;
    HRESULT hr = g_wicFactory->CreateBitmapFromHBITMAP(hBitmap, NULL, WICBitmapUsePremultipliedAlpha, &wicBitmap);
    CloseClipboard();

    if (FAILED(hr)) return false;

    ComPtr<IWICFormatConverter> converter;
    hr = g_wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr)) return false;

    hr = converter->Initialize(
        wicBitmap.Get(),
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone,
        NULL,
        0.f,
        WICBitmapPaletteTypeMedianCut
    );
    if (FAILED(hr)) return false;

    UINT width, height;
    hr = converter->GetSize(&width, &height);
    if (FAILED(hr)) return false;

    UINT cbStride = width * 4;
    UINT cbBufferSize = cbStride * height;
    outData.resize(cbBufferSize);

    hr = converter->CopyPixels(
        NULL,
        cbStride,
        cbBufferSize,
        outData.data()
    );

    if (SUCCEEDED(hr)) {
        outWidth = static_cast<int>(width);
        outHeight = static_cast<int>(height);
        return true;
    }
    return false;
}

bool WicImageLoader::SaveImageToFile(const std::wstring& path, const std::vector<uint8_t>& bgraData, int width, int height) {
    if (!g_wicFactory) return false;
    
    ComPtr<IWICStream> stream;
    HRESULT hr = g_wicFactory->CreateStream(&stream);
    if (FAILED(hr)) return false;
    
    hr = stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE);
    if (FAILED(hr)) return false;
    
    ComPtr<IWICBitmapEncoder> encoder;
    GUID containerFormat = GUID_ContainerFormatPng;
    if (path.length() >= 4) {
        std::wstring ext = path.substr(path.length() - 4);
        if (ext == L".bmp") containerFormat = GUID_ContainerFormatBmp;
        else if (ext == L".jpg" || ext == L"jpeg") containerFormat = GUID_ContainerFormatJpeg;
    }
    
    hr = g_wicFactory->CreateEncoder(containerFormat, NULL, &encoder);
    if (FAILED(hr)) return false;
    
    hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
    if (FAILED(hr)) return false;
    
    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> props;
    hr = encoder->CreateNewFrame(&frame, &props);
    if (FAILED(hr)) return false;
    
    hr = frame->Initialize(props.Get());
    if (FAILED(hr)) return false;
    
    hr = frame->SetSize(width, height);
    if (FAILED(hr)) return false;
    
    WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
    hr = frame->SetPixelFormat(&format);
    if (FAILED(hr)) return false;
    
    hr = frame->WritePixels(height, width * 4, static_cast<UINT>(bgraData.size()), (BYTE*)bgraData.data());
    if (FAILED(hr)) return false;
    
    hr = frame->Commit();
    if (FAILED(hr)) return false;
    
    hr = encoder->Commit();
    if (FAILED(hr)) return false;
    
    return true;
}

} // namespace utils
} // namespace ui


