#include "Application.h"
#include "ui/MainWindow.h"
#include "PdfiumLibrary.h"
#include "CrashHandler.h"
#include "Preferences.h"
#include <shlobj.h>
#include <commctrl.h>

Application& Application::Instance() {
    static Application instance;
    return instance;
}

void Application::Initialize(HINSTANCE hInstance, int nCmdShow) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    m_hInstance = hInstance;
    m_nCmdShow = nCmdShow;
    
    INITCOMMONCONTROLSEX iccex = {};
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS | ICC_LISTVIEW_CLASSES | ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&iccex);

    app::CrashHandler::Initialize();
    app::Preferences::Load();

    // Initialize PDFium
    PdfiumLibrary::Instance().Initialize();
}


int Application::Run(const std::wstring& openFile) {
    MainWindow mainWindow;
    if (!mainWindow.Create(L"PDF Elite", 1200, 800)) {
        printf("Failed to create MainWindow!\n");
        return 0;
    }
    
    mainWindow.Show(m_nCmdShow);

    if (!openFile.empty()) {
        mainWindow.OpenFileDirect(openFile);
    }
    
    printf("Entering message loop...\n");
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    printf("Exited message loop. wParam = %d\n", static_cast<int>(msg.wParam));
    return static_cast<int>(msg.wParam);
}

void Application::Shutdown() {
    // PdfiumLibrary destructor cleans up PDFium.
    CoUninitialize();
}
