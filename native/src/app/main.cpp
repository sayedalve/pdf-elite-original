#include "Application.h"
#include "CrashHandler.h"
#include <windows.h>
#include <string>
#include <iostream>
#include "ui/src/ThemeManager.h"
#include "../utils/Logger.h"
#include "../utils/PerfLog.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLine, int nCmdShow) {
    (void)nCmdShow;
    (void)hPrevInstance;
    (void)pCmdLine;
    PERF_SCOPE("App_Initialization");
    
    // Always force SW_SHOW for tests
    nCmdShow = SW_SHOW;
    
    // 1. Set DPI Awareness
    // We try to use the Windows 10 Creators Update API for Per Monitor V2.
    // If that fails, it falls back gracefully depending on the OS version.
    HMODULE hUser32 = LoadLibraryW(L"user32.dll");
    if (hUser32) {
        typedef BOOL(WINAPI* SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
        SetProcessDpiAwarenessContextProc setDpiAwarenessContext = 
            (SetProcessDpiAwarenessContextProc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        
        if (setDpiAwarenessContext) {
            setDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        } else {
            typedef BOOL(WINAPI* SetProcessDPIAwareProc)(VOID);
            SetProcessDPIAwareProc setDpiAware = (SetProcessDPIAwareProc)GetProcAddress(hUser32, "SetProcessDPIAware");
            if (setDpiAware) setDpiAware();
        }
        FreeLibrary(hUser32);
    }
    
    // Log resulting DPI Awareness
    if ((hUser32 = LoadLibraryW(L"user32.dll"))) {
        typedef DPI_AWARENESS_CONTEXT(WINAPI* GetThreadDpiAwarenessContextProc)();
        typedef DPI_AWARENESS(WINAPI* GetAwarenessFromDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
        typedef UINT(WINAPI* GetDpiForSystemProc)();
        
        GetThreadDpiAwarenessContextProc getCtx = (GetThreadDpiAwarenessContextProc)GetProcAddress(hUser32, "GetThreadDpiAwarenessContext");
        GetAwarenessFromDpiAwarenessContextProc getAwareness = (GetAwarenessFromDpiAwarenessContextProc)GetProcAddress(hUser32, "GetAwarenessFromDpiAwarenessContext");
        GetDpiForSystemProc getSysDpi = (GetDpiForSystemProc)GetProcAddress(hUser32, "GetDpiForSystem");
        
        DPI_AWARENESS awareness = DPI_AWARENESS_INVALID;
        if (getCtx && getAwareness) {
            DPI_AWARENESS_CONTEXT ctx = getCtx();
            awareness = getAwareness(ctx);
        }
        
        UINT sysDpi = getSysDpi ? getSysDpi() : 0;
        
        utils::Logger::Log("PROCESS STARTUP DPI REPORT:");
        utils::Logger::Log(std::string("DPI_AWARENESS: ") + std::to_string((int)awareness));
        utils::Logger::Log(std::string("System DPI: ") + std::to_string(sysDpi));
        
        FreeLibrary(hUser32);
    }

    app::CrashHandler::Initialize();

    if (pCmdLine) {
        std::wstring cmdLine(pCmdLine);
        if (cmdLine.find(L"--dev") != std::wstring::npos) {
            if (cmdLine.find(L"--test-crash") != std::wstring::npos) {
                // Force a controlled crash to test crash handler
                int* volatile badPtr = nullptr;
                *badPtr = 42;
            }
            
            // Set up hot reload
            ThemeManager::Instance().SetDevMode(true);
            
            // Resolve absolute path to native/resources
            WCHAR exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            std::wstring baseDir = exePath;
            size_t pos = baseDir.find(L"\\build\\");
            if (pos != std::wstring::npos) {
                std::wstring resourcesDir = baseDir.substr(0, pos) + L"\\resources";
                ThemeManager::Instance().Initialize(resourcesDir);
            }
        }
    }

    std::wstring openFile;
    if (pCmdLine) {
        std::wstring cmdLine(pCmdLine);
        size_t pos = cmdLine.find(L"--open=");
        if (pos != std::wstring::npos) {
            size_t end = cmdLine.find(L" ", pos);
            if (end == std::wstring::npos) end = cmdLine.length();
            openFile = cmdLine.substr(pos + 7, end - pos - 7);
            
            // Remove quotes if present
            if (openFile.length() >= 2 && openFile.front() == L'"' && openFile.back() == L'"') {
                openFile = openFile.substr(1, openFile.length() - 2);
            }
        }
    }

    // AllocConsole();
    FILE* fp;
    freopen_s(&fp, "out.txt", "w", stdout);
    printf("App starting...\n");

    auto& app = Application::Instance();
    {
        PERF_SCOPE("Application Initialization");
        app.Initialize(hInstance, SW_SHOW); // Force SHOW
    }
    printf("Initialized app\n");
    int result = 0;
    {
        PERF_SCOPE("Application Run / Session");
        result = app.Run(openFile);
    }
    printf("Run finished with result %d\n", result);
    app.Shutdown();
    return result;
}
