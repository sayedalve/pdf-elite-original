// PreviewMain.cpp - Entry point for the PDF Elite UI Preview application
#include <windows.h>
#include "MainWindow.h"

using namespace PdfElite;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) { MessageBoxW(NULL, L"CoInitializeEx FAILED", L"Debug", MB_OK); return 0; }

    MainWindow window;
    hr = window.Initialize(hInstance);
    if (FAILED(hr)) {
        CoUninitialize();
        return 0;
    }

    int ret = window.Run();

    CoUninitialize();
    return ret;
}
