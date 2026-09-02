#pragma once
#include "AppShell.h"
#include "Theme.h"
#include "CommandManager.h"
#include "AppMode.h"
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <vector>
#include <string>

namespace PdfElite {

struct RecentFile {
    std::wstring name;
    std::wstring path;
    std::wstring size;
    std::wstring modifiedLabel;
    std::wstring modified;
    int pages;
    bool starred;
    std::wstring type;
};

struct QuickTool {
    std::wstring id;
    std::wstring name;
    std::wstring desc;
    uint32_t accent;
    std::wstring count;
    D2D1_COLOR_F gradientStart;
    D2D1_COLOR_F gradientEnd;
};

class MainWindow {
public:
    MainWindow();
    ~MainWindow();
    HRESULT Initialize(HINSTANCE hInstance);
    int Run();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    HRESULT CreateDeviceResources();
    void DiscardDeviceResources();
    HRESULT OnRender();
    void OnResize(UINT width, UINT height);

    void RenderHome(ID2D1RenderTarget* rt, const D2D1_RECT_F& clientRect);
    void RenderHomeSidebar(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect);
    void RenderHomeMain(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect);
    void RenderViewer(ID2D1RenderTarget* rt, const D2D1_RECT_F& clientRect);

    void OnMouseMove(int x, int y);
    void OnLButtonDown(int x, int y);

    HWND m_hwnd = nullptr;
    HINSTANCE m_hInst = nullptr;

    Microsoft::WRL::ComPtr<ID2D1Factory> m_d2dFactory;
    Microsoft::WRL::ComPtr<IDWriteFactory> m_dwFactory;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> m_rt;

    Theme m_theme;
    AppShell m_shell;
    CommandManager m_cmd;

    AppMode m_mode = AppMode::Home;
    ViewerMode m_viewerMode = ViewerMode::View;

    int m_hoveredFile = -1;
    int m_hoveredTool = -1;
    int m_selectedNav = 0;

    int m_curPage = 1;
    int m_totalPages = 10;
    float m_zoom = 1.0f;

    std::vector<RecentFile> m_recent;
};

}