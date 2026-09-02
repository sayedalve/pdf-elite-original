#include <string>
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

class Application {
public:
    static Application& Instance();

    void Initialize(HINSTANCE hInstance, int nCmdShow);
    int Run(const std::wstring& openFile = L"");
    void Shutdown();

    HINSTANCE GetHInstance() const { return m_hInstance; }

private:
    Application() = default;
    
    HINSTANCE m_hInstance = nullptr;
    int m_nCmdShow = 0;
};
