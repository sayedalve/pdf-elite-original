#pragma once
#include <string>

namespace app {

struct AppSettings {
    bool isDarkTheme = true;
    int lastWindowWidth = 1024;
    int lastWindowHeight = 768;
    std::wstring lastOpenedDir = L"";
};

class Preferences {
public:
    static AppSettings Load();
    static bool Save(const AppSettings& settings);
};

}
