#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d2d1_1.h>
#include <d2d1_3.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <dxgi1_2.h>

using Microsoft::WRL::ComPtr;

class GraphicsDevice {
public:
    static GraphicsDevice& Instance();

    bool Initialize();
    bool CreateRenderTarget(HWND hwnd, int width, int height, ComPtr<ID2D1DeviceContext5>& target);
    
    ComPtr<ID2D1Factory1> GetD2DFactory() const { return m_d2dFactory; }
    ComPtr<IDWriteFactory> GetDWriteFactory() const { return m_dwriteFactory; }
    ComPtr<IWICImagingFactory> GetWicFactory() const { return m_wicFactory; }
    
    // New methods for D3D11/DXGI swapchain resizing
    void Resize(int width, int height);
    void Present();

private:
    GraphicsDevice() = default;
    
    ComPtr<ID2D1Factory1> m_d2dFactory;
    ComPtr<IDWriteFactory> m_dwriteFactory;
    ComPtr<IWICImagingFactory> m_wicFactory;
    
    // D3D11 / DXGI objects
    ComPtr<IDXGISwapChain1> m_swapChain;
    ComPtr<ID2D1DeviceContext5> m_deviceContext;
};
