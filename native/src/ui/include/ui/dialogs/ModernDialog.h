#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>
#include <vector>

namespace ui::dialogs {

class ModernDialog {
public:
    ModernDialog(HWND parent, const std::wstring& title, int width, int height);
    virtual ~ModernDialog();

    bool DoModal();
    HWND CreateModeless(int x, int y);

protected:
    virtual void OnLayout(float w, float h) = 0;
    virtual void OnRender() = 0;
    virtual void OnMouseMove(float x, float y) = 0;
    virtual void OnMouseDown(float x, float y) = 0;
    virtual void OnMouseUp(float x, float y) = 0;
    virtual void OnCreate() {}

    HWND m_parent;
    HWND m_hwnd;
    std::wstring m_title;
    int m_width;
    int m_height;
    bool m_resultOk = false;
    bool m_running = false;
    float m_dpiScale = 1.0f;
    HFONT m_editFont = nullptr;

    // Drawing resources
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> m_target;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_bgBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_surfaceBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_textBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_textDarkBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_borderBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_primaryBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_hoverBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_closeHoverBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_inputBgBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_accentBrush;
    
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_formatTitle;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_formatHeader;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_formatLabel;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_formatValue;

    // Standard regions
    D2D1_RECT_F m_rectClose;
    bool m_closeHover = false;
    bool m_closeDown = false;

    void CreateDeviceResources();
    void RenderBase();
    void DrawPrimaryButton(const D2D1_RECT_F& r, const wchar_t* txt, bool hover, bool down);
    void DrawSecondaryButton(const D2D1_RECT_F& r, const wchar_t* txt, bool hover, bool down);
    void DrawRadio(const D2D1_RECT_F& r, const wchar_t* txt, int len, bool checked);
    void DrawCheckbox(const D2D1_RECT_F& r, const wchar_t* txt, int len, bool checked);
    void DrawEditBorder(const D2D1_RECT_F& r);
    bool PtInR(float x, float y, const D2D1_RECT_F& r);
    
    void SetChildPos(HWND child, float x, float y, float w, float h);
    void SetPlaceholder(HWND edit, const std::wstring& placeholder);
    void Invalidate() { InvalidateRect(m_hwnd, nullptr, FALSE); }
    HFONT GetEditFont();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

} // namespace ui::dialogs
