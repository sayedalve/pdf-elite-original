// GraphicsDevice.h - Direct2D device management
#pragma once
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

namespace PdfElite {

class GraphicsDevice {
public:
    HRESULT Initialize(HWND hwnd);
    void Release();

    ID2D1Factory* GetD2DFactory() const { return m_d2dFactory.Get(); }
    ID2D1HwndRenderTarget* GetRenderTarget() const { return m_renderTarget.Get(); }
    IDWriteFactory* GetDWriteFactory() const { return m_dwriteFactory.Get(); }

private:
    Microsoft::WRL::ComPtr<ID2D1Factory> m_d2dFactory;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
    Microsoft::WRL::ComPtr<IDWriteFactory> m_dwriteFactory;
};

} // namespace PdfElite
