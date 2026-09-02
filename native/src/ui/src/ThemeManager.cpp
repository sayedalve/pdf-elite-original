#include "ThemeManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

ThemeManager::ThemeManager() : m_watching(false) {
}

ThemeManager::~ThemeManager() {
    StopWatching();
}

ThemeManager& ThemeManager::Instance() {
    static ThemeManager instance;
    return instance;
}

void ThemeManager::Initialize(const std::wstring& watchDir) {
    m_watchDir = watchDir;
}

void ThemeManager::StartWatching() {
    if (!m_devMode) return;
    if (m_watching) return;
    if (m_watchDir.empty()) return;
    
    m_hDir = CreateFileW(m_watchDir.c_str(), FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

    if (m_hDir == INVALID_HANDLE_VALUE) {
        printf("[DEV] Failed to open watch dir\n"); fflush(stdout);
        return;
    }

    m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_watching = true;
    m_watchThread = std::thread(&ThemeManager::WatchLoop, this);
    
    printf("[DEV] Theme watcher started\n"); fflush(stdout);
    LoadTheme();
}

void ThemeManager::StopWatching() {
    if (!m_watching) return;
    m_watching = false;
    if (m_hDir != INVALID_HANDLE_VALUE) CancelIoEx(m_hDir, NULL);
    SetEvent(m_hEvent);
    if (m_watchThread.joinable()) m_watchThread.join();
    
    if (m_hDir != INVALID_HANDLE_VALUE) CloseHandle(m_hDir);
    if (m_hEvent) CloseHandle(m_hEvent);
    m_hDir = INVALID_HANDLE_VALUE;
    m_hEvent = NULL;
}

D2D1_COLOR_F ParseColor(const std::wstring& hexStr) {
    if (hexStr.length() == 7 && hexStr[0] == L'#') {
        int r = 0, g = 0, b = 0;
        swscanf_s(hexStr.c_str(), L"#%02x%02x%02x", &r, &g, &b);
        return D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f);
    }
    return D2D1::ColorF(D2D1::ColorF::Black);
}

bool ThemeManager::ParseTheme(const std::wstring& content) {
    std::wsmatch match;
    std::wregex rBgPrimary(L"\"bgPrimary\"\\s*:\\s*\"(#[0-9a-fA-F]{6})\"");
    std::wregex rBgSecondary(L"\"bgSecondary\"\\s*:\\s*\"(#[0-9a-fA-F]{6})\"");
    std::wregex rText(L"\"text\"\\s*:\\s*\"(#[0-9a-fA-F]{6})\"");

    bool parsedAny = false;
    ThemeColors newColors = m_colors;

    if (std::regex_search(content, match, rBgPrimary)) {
        newColors.bgPrimary = ParseColor(match[1].str());
        parsedAny = true;
    }
    if (std::regex_search(content, match, rBgSecondary)) {
        newColors.bgSecondary = ParseColor(match[1].str());
        parsedAny = true;
    }
    if (std::regex_search(content, match, rText)) {
        newColors.text = ParseColor(match[1].str());
        parsedAny = true;
    }

    if (parsedAny) {
        m_colors = newColors;
    }
    return parsedAny;
}

void ThemeManager::LoadTheme() {
    std::wstring themePath = m_watchDir + L"\\theme.json";
    std::wifstream f(themePath);
    if (!f.is_open()) return;

    std::wstringstream buffer;
    buffer << f.rdbuf();
    std::wstring content = buffer.str();
    
    if (ParseTheme(content)) {
        printf("[DEV] Theme reloaded successfully.\n"); fflush(stdout);
        OnThemeChanged.Invoke();
    } else {
        printf("[DEV] Theme reload failed: invalid format.\n"); fflush(stdout);
    }
}

void ThemeManager::WatchLoop() {
    char buffer[1024];
    DWORD bytesReturned;
    OVERLAPPED overlapped = {};
    overlapped.hEvent = m_hEvent;

    while (m_watching) {
        ResetEvent(m_hEvent);
        BOOL success = ReadDirectoryChangesW(
            m_hDir, buffer, sizeof(buffer), FALSE,
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
            &bytesReturned, &overlapped, NULL);

        if (!success) break;

        DWORD waitRes = WaitForSingleObject(m_hEvent, INFINITE);
        if (!m_watching) break;

        if (waitRes == WAIT_OBJECT_0) {
            DWORD bytesTransferred = 0;
            BOOL res = GetOverlappedResult(m_hDir, &overlapped, &bytesTransferred, FALSE);

            if (!res || bytesTransferred == 0) continue;

            FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
            bool themeChanged = false;
            do {
                std::wstring filename(fni->FileName, fni->FileNameLength / sizeof(WCHAR));
                if (filename == L"theme.json") {
                    themeChanged = true;
                }
                if (fni->NextEntryOffset == 0) break;
                fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<char*>(fni) + fni->NextEntryOffset);
            } while (true);

            if (themeChanged) {
                Sleep(50); // Small delay to avoid file lock contention with the editor
                LoadTheme();
            }
        }
    }
}
