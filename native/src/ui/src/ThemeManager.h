#pragma once
#include <string>
#include <functional>
#include <d2d1_1.h>
#include <thread>
#include <atomic>
#include <windows.h>
#include "core/Event.h"

struct ThemeColors {
    D2D1_COLOR_F bgPrimary = D2D1::ColorF(D2D1::ColorF::White);
    D2D1_COLOR_F bgSecondary = D2D1::ColorF(0.9f, 0.9f, 0.9f);
    D2D1_COLOR_F text = D2D1::ColorF(D2D1::ColorF::Black);
};

class ThemeManager {
public:
    static ThemeManager& Instance();

    void Initialize(const std::wstring& watchDir);
    void StartWatching();
    void StopWatching();

    void SetDevMode(bool devMode) { m_devMode = devMode; }
    bool IsDevMode() const { return m_devMode; }

    const ThemeColors& GetColors() const { return m_colors; }
    
    Event<> OnThemeChanged;

private:
    ThemeManager();
    ~ThemeManager();

    void WatchLoop();
    void LoadTheme();
    bool ParseTheme(const std::wstring& content);

    std::wstring m_watchDir;
    ThemeColors m_colors;
    std::atomic<bool> m_watching;
    std::atomic<bool> m_devMode{false};
    std::thread m_watchThread;
    HANDLE m_hDir = INVALID_HANDLE_VALUE;
    HANDLE m_hEvent = NULL;
};
