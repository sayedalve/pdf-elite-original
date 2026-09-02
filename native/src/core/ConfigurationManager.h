#pragma once
#include <string>
#include <mutex>

namespace core {

struct AppConfig {
    std::wstring theme = L"dark";
    bool hardwareAcceleration = true;
    int maxThreads = 0; // 0 means auto
};

class ConfigurationManager {
public:
    static ConfigurationManager& Instance();

    void Load();
    void Save();

    AppConfig GetConfig();
    void UpdateConfig(const AppConfig& config);

    std::wstring GetConfigFilePath() const;

private:
    ConfigurationManager();
    ~ConfigurationManager() = default;
    ConfigurationManager(const ConfigurationManager&) = delete;
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;

    AppConfig m_config;
    std::mutex m_mutex;
};

} // namespace core
