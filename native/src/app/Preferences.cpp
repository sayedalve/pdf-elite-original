#include "Preferences.h"

namespace app {

AppSettings Preferences::Load() {
    AppSettings settings;
    // Stub: load from %APPDATA%\PDFElite\settings.json
    return settings;
}

bool Preferences::Save(const AppSettings& settings) {
    (void)settings;
    // Stub: write to %APPDATA%\PDFElite\settings.json
    return true;
}

}
