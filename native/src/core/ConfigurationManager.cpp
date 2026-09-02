#include "ConfigurationManager.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>

namespace core {

ConfigurationManager& ConfigurationManager::Instance() {
    static ConfigurationManager instance;
    return instance;
}

ConfigurationManager::ConfigurationManager() {
    Load();
}

std::wstring ConfigurationManager::GetConfigFilePath() const {
    WCHAR path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
        std::wstring dir = std::wstring(path) + L"\\PDFElite";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir + L"\\config.ini";
    }
    return L"config.ini";
}

void ConfigurationManager::Load() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::wifstream file(GetConfigFilePath());
    if (!file.is_open()) return;

    std::wstring line;
    while (std::getline(file, line)) {
        size_t eqPos = line.find(L'=');
        if (eqPos != std::wstring::npos) {
            std::wstring key = line.substr(0, eqPos);
            std::wstring val = line.substr(eqPos + 1);
            if (key == L"theme") m_config.theme = val;
            else if (key == L"hardwareAcceleration") m_config.hardwareAcceleration = (val == L"1" || val == L"true");
            else if (key == L"maxThreads") {
                try { m_config.maxThreads = std::stoi(val); } catch (...) {}
            }
        }
    }
}

void ConfigurationManager::Save() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::wofstream file(GetConfigFilePath());
    if (!file.is_open()) return;

    file << L"theme=" << m_config.theme << L"\n";
    file << L"hardwareAcceleration=" << (m_config.hardwareAcceleration ? L"1" : L"0") << L"\n";
    file << L"maxThreads=" << m_config.maxThreads << L"\n";
}

AppConfig ConfigurationManager::GetConfig() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

void ConfigurationManager::UpdateConfig(const AppConfig& config) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config = config;
    }
    Save();
}

} // namespace core
