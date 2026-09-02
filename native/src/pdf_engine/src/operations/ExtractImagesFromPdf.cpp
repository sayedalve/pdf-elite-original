#include "pdf_engine/operations/ExtractImagesFromPdf.h"
#include "../commands/CommandUtils.h"
#include "PdfDocument.h"
#include "PdfPage.h"
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <windows.h>
#include <wincodec.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cwctype>

namespace pdf_engine {
namespace operations {

namespace {

bool SaveBmpDirect(const std::wstring& outPath, const void* buffer, int width, int height, int stride) {
    std::ofstream out(outPath, std::ios::binary);
    if (!out.is_open()) return false;

    BITMAPFILEHEADER bfh = {};
    BITMAPINFOHEADER bih = {};

    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = -height; // Top-down
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = stride * height;

    bfh.bfType = 0x4D42; // "BM"
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + bih.biSizeImage;

    out.write(reinterpret_cast<const char*>(&bfh), sizeof(bfh));
    out.write(reinterpret_cast<const char*>(&bih), sizeof(bih));
    out.write(reinterpret_cast<const char*>(buffer), bih.biSizeImage);

    return out.good();
}

bool SaveBitmapWithWic(const std::wstring& outPath, const void* buffer, int width, int height, int stride, const std::wstring& format) {
    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    IWICImagingFactory* pFactory = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pFactory));
    if (FAILED(hr) || !pFactory) {
        if (SUCCEEDED(hrCo)) CoUninitialize();
        return SaveBmpDirect(outPath, buffer, width, height, stride);
    }

    GUID containerFormat = GUID_ContainerFormatPng;
    std::wstring fmtUpper = format;
    for (auto& c : fmtUpper) c = static_cast<wchar_t>(::towupper(c));

    if (fmtUpper == L"JPEG" || fmtUpper == L"JPG") {
        containerFormat = GUID_ContainerFormatJpeg;
    } else if (fmtUpper == L"BMP") {
        containerFormat = GUID_ContainerFormatBmp;
    }

    IWICStream* pStream = nullptr;
    hr = pFactory->CreateStream(&pStream);
    if (FAILED(hr) || !pStream) {
        pFactory->Release();
        if (SUCCEEDED(hrCo)) CoUninitialize();
        return SaveBmpDirect(outPath, buffer, width, height, stride);
    }

    hr = pStream->InitializeFromFilename(outPath.c_str(), GENERIC_WRITE);
    if (FAILED(hr)) {
        pStream->Release();
        pFactory->Release();
        if (SUCCEEDED(hrCo)) CoUninitialize();
        return SaveBmpDirect(outPath, buffer, width, height, stride);
    }

    IWICBitmapEncoder* pEncoder = nullptr;
    hr = pFactory->CreateEncoder(containerFormat, nullptr, &pEncoder);
    if (FAILED(hr) || !pEncoder) {
        pStream->Release();
        pFactory->Release();
        if (SUCCEEDED(hrCo)) CoUninitialize();
        return SaveBmpDirect(outPath, buffer, width, height, stride);
    }

    hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
    if (FAILED(hr)) {
        pEncoder->Release();
        pStream->Release();
        pFactory->Release();
        if (SUCCEEDED(hrCo)) CoUninitialize();
        return SaveBmpDirect(outPath, buffer, width, height, stride);
    }

    IWICBitmapFrameEncode* pFrame = nullptr;
    IPropertyBag2* pProps = nullptr;
    hr = pEncoder->CreateNewFrame(&pFrame, &pProps);
    if (FAILED(hr) || !pFrame) {
        pEncoder->Release();
        pStream->Release();
        pFactory->Release();
        if (SUCCEEDED(hrCo)) CoUninitialize();
        return SaveBmpDirect(outPath, buffer, width, height, stride);
    }

    hr = pFrame->Initialize(pProps);
    hr = pFrame->SetSize(width, height);
    WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat32bppBGRA;
    hr = pFrame->SetPixelFormat(&pixelFormat);

    UINT bufferSize = static_cast<UINT>(stride * height);
    hr = pFrame->WritePixels(height, stride, bufferSize, const_cast<BYTE*>(static_cast<const BYTE*>(buffer)));
    if (SUCCEEDED(hr)) {
        hr = pFrame->Commit();
        if (SUCCEEDED(hr)) {
            hr = pEncoder->Commit();
        }
    }

    if (pProps) pProps->Release();
    pFrame->Release();
    pEncoder->Release();
    pStream->Release();
    pFactory->Release();
    if (SUCCEEDED(hrCo)) CoUninitialize();

    if (FAILED(hr)) {
        return SaveBmpDirect(outPath, buffer, width, height, stride);
    }
    return true;
}

} // anonymous namespace

int ExtractImagesFromDocument(core::interfaces::dom::IDocument* doc, const ExtractImagesParams& params) {
    if (!doc || params.outputDir.empty()) return 0;

    PdfDocument* pdfDoc = dynamic_cast<PdfDocument*>(doc);
    std::unique_lock<std::recursive_mutex> lock;
    if (pdfDoc) {
        lock = std::unique_lock<std::recursive_mutex>(pdfDoc->GetMutex());
    }

    std::error_code ec;
    std::filesystem::create_directories(params.outputDir, ec);

    int totalPages = doc->PageCount();
    if (totalPages <= 0) return 0;

    std::vector<int> targetPages = commands::ParsePageRange(params.pageScope, params.pageRange, params.currentPage, totalPages);
    if (targetPages.empty()) return 0;

    std::wstring ext = L"png";
    std::wstring fmtUpper = params.format;
    for (auto& c : fmtUpper) c = static_cast<wchar_t>(::towupper(c));
    if (fmtUpper == L"JPEG" || fmtUpper == L"JPG") ext = L"jpg";
    else if (fmtUpper == L"BMP") ext = L"bmp";

    std::wstring prefix = params.prefix.empty() ? L"img_p" : params.prefix;

    int extractedCount = 0;

    for (int pageIdx : targetPages) {
        auto page = doc->GetPage(pageIdx);
        if (!page) continue;

        PdfPage* pdfPage = dynamic_cast<PdfPage*>(page.get());
        if (!pdfPage) continue;

        FPDF_PAGE pageHandle = pdfPage->GetHandle();
        int objCount = FPDFPage_CountObjects(pageHandle);

        for (int objIdx = 0; objIdx < objCount; ++objIdx) {
            FPDF_PAGEOBJECT obj = FPDFPage_GetObject(pageHandle, objIdx);
            if (!obj) continue;

            if (FPDFPageObj_GetType(obj) == FPDF_PAGEOBJ_IMAGE) {
                FPDF_BITMAP bmp = FPDFImageObj_GetBitmap(obj);
                if (bmp) {
                    int width = FPDFBitmap_GetWidth(bmp);
                    int height = FPDFBitmap_GetHeight(bmp);
                    int stride = FPDFBitmap_GetStride(bmp);
                    void* buffer = FPDFBitmap_GetBuffer(bmp);

                    if (width > 0 && height > 0 && buffer) {
                        std::wstring outPath = params.outputDir;
                        if (outPath.back() != L'\\' && outPath.back() != L'/') {
                            outPath += L"\\";
                        }
                        outPath += prefix + std::to_wstring(pageIdx + 1) + L"_" + std::to_wstring(objIdx + 1) + L"." + ext;

                        if (SaveBitmapWithWic(outPath, buffer, width, height, stride, ext)) {
                            extractedCount++;
                        }
                    }

                    FPDFBitmap_Destroy(bmp);
                }
            }
        }
    }

    return extractedCount;
}

int ExtractImagesFromDocument(std::shared_ptr<core::interfaces::dom::IDocument> doc, const ExtractImagesParams& params) {
    return ExtractImagesFromDocument(doc.get(), params);
}

int ExtractImagesFromPdf(const std::wstring& pdfPath, const ExtractImagesParams& params) {
    auto res = PdfDocument::LoadFromFile(pdfPath.c_str());
    if (!res.has_value() || !res.value) return 0;

    return ExtractImagesFromDocument(res.value.get(), params);
}

} // namespace operations
} // namespace pdf_engine
