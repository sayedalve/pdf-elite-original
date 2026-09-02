# Settings System

> Engineering doc for rebuilding the PDF Elite settings subsystem as a native C++/Win32 application.

## Current Architecture (Tauri/Web)

### Settings Sources

| Layer | Storage | Format | Scope |
|-------|---------|--------|-------|
| User Preferences | `localStorage` via `preferencesService` | JSON | Theme, language, hidden tools, sidebar, reading mode, zoom |
| Connection Config | Tauri Store (JSON file) | JSON | Connection mode, server URL, auth tokens |
| App Config | Backend API endpoint | REST response | Feature flags |
| Desktop Config | Tauri Store (JSON) | JSON | Desktop-specific settings |

> **FACT**: PreferencesContext stores user preferences (theme, language, hidden tools, sidebar visibility, reading mode, default zoom) in `localStorage` via `preferencesService`.

> **FACT**: Desktop-specific settings (connection configuration, auth tokens) use Tauri Store JSON persistence.

> **FACT**: App-level configuration (feature flags) is fetched dynamically from the backend endpoint `/api/v1/config/app-config`.

### Current Settings Categories

```
PreferencesContext
├── theme                    // "light" | "dark" | "system"
├── language                 // locale string, e.g. "en-US"
├── hiddenTools              // string[] of hidden tool IDs
├── sidebarVisible           // boolean
├── readingMode              // "single" | "double" | "scroll"
└── defaultZoom              // number (percentage)
```

### Settings Flow

```
App Launch
  ├── preferencesService.loadFromLocalStorage()
  │     └── PreferencesContext initialized
  ├── Tauri Store.load("connection-config")
  │     └── Desktop connection settings
  └── fetch("/api/v1/config/app-config")
        └── Feature flags applied
```

---

## Proposed Native Architecture (C++/Win32)

### Storage Strategy

> **RECOMMENDATION**: Use a JSON file in `%APPDATA%\PDF Elite\settings.json` for persistent settings.

| Option | Pros | Cons | Verdict |
|--------|------|------|---------|
| Windows Registry (HKCU) | Native, fast, per-user | Not portable, not human-readable, harder to backup | ❌ Not recommended |
| JSON file in %APPDATA% | Portable, human-readable, easy backup/migrate | Slightly slower than registry | ✅ **Recommended** |
| JSON file next to EXE | True portable mode | Needs write permissions in Program Files | ⚠️ Portable mode only |

### Settings File Structure

```json
{
  "version": 2,
  "lastModified": "2025-01-15T10:30:00Z",
  "general": {
    "language": "en-US",
    "autoUpdate": true,
    "updateChannel": "stable",
    "showSplashScreen": true,
    "checkUpdatesInterval": 24,
    "telemetryEnabled": false,
    "portableMode": false
  },
  "display": {
    "theme": "system",
    "defaultZoom": 100,
    "readingMode": "continuous",
    "sidebarVisible": true,
    "sidebarWidth": 280,
    "toolbarPosition": "top",
    "showPageNumbers": true,
    "smoothScrolling": true,
    "renderQuality": "high",
    "gpuAcceleration": true
  },
  "editor": {
    "defaultFontFamily": "Arial",
    "defaultFontSize": 12,
    "defaultTextColor": "#000000",
    "snapToGrid": false,
    "gridSize": 10,
    "autoSave": true,
    "autoSaveInterval": 300,
    "maxUndoLevels": 100,
    "hiddenTools": []
  },
  "advanced": {
    "maxMemoryMB": 512,
    "cacheSizeMB": 256,
    "threadCount": 0,
    "logLevel": "warning",
    "hardwareAcceleration": "auto",
    "customTempDir": "",
    "proxySettings": {
      "enabled": false,
      "host": "",
      "port": 0,
      "username": "",
      "password": ""
    }
  },
  "connection": {
    "mode": "local",
    "serverUrl": "",
    "authToken": "",
    "refreshToken": ""
  },
  "recentFiles": {
    "maxEntries": 20,
    "entries": [
      {
        "path": "C:\\Users\\user\\Documents\\report.pdf",
        "lastOpened": "2025-01-15T10:00:00Z",
        "page": 5,
        "zoom": 125
      }
    ]
  }
}
```

### Settings Schema Table

| Category | Setting | Type | Default | Description |
|----------|---------|------|---------|-------------|
| General | language | string | "en-US" | UI locale |
| General | autoUpdate | bool | true | Check for updates automatically |
| General | updateChannel | enum | "stable" | stable / beta / dev |
| General | showSplashScreen | bool | true | Show splash on startup |
| General | telemetryEnabled | bool | false | Send anonymous usage data |
| Display | theme | enum | "system" | light / dark / system |
| Display | defaultZoom | int | 100 | Default zoom level (%) |
| Display | readingMode | enum | "continuous" | single-page / continuous / double |
| Display | sidebarVisible | bool | true | Show sidebar on open |
| Display | sidebarWidth | int | 280 | Sidebar width in pixels |
| Display | renderQuality | enum | "high" | low / medium / high |
| Display | gpuAcceleration | bool | true | Use GPU for rendering |
| Editor | defaultFontFamily | string | "Arial" | Default annotation font |
| Editor | defaultFontSize | int | 12 | Default annotation font size |
| Editor | autoSave | bool | true | Auto-save documents |
| Editor | autoSaveInterval | int | 300 | Seconds between auto-saves |
| Editor | maxUndoLevels | int | 100 | Maximum undo steps |
| Editor | hiddenTools | string[] | [] | Hidden tool IDs |
| Advanced | maxMemoryMB | int | 512 | Max memory for PDF rendering |
| Advanced | cacheSizeMB | int | 256 | Page cache size |
| Advanced | threadCount | int | 0 | 0 = auto (CPU cores) |
| Advanced | logLevel | enum | "warning" | trace / debug / info / warning / error |

### C++ Class Design

```cpp
// settings_manager.h
class SettingsManager {
public:
    static SettingsManager& Instance();

    // Load/save
    bool Load();
    bool Save();

    // Accessors
    const GeneralSettings&   General()   const;
    const DisplaySettings&   Display()   const;
    const EditorSettings&    Editor()    const;
    const AdvancedSettings&  Advanced()  const;
    const ConnectionSettings& Connection() const;

    // Mutators (trigger change notifications)
    void SetGeneral(const GeneralSettings& settings);
    void SetDisplay(const DisplaySettings& settings);
    void SetEditor(const EditorSettings& settings);

    // Change notification
    using ChangeCallback = std::function<void(const std::string& key)>;
    void RegisterObserver(const std::string& key, ChangeCallback cb);
    void UnregisterObserver(const std::string& key);

    // Portable mode
    bool IsPortableMode() const;
    void SetPortableMode(bool enabled);

private:
    SettingsManager();
    ~SettingsManager();

    std::wstring GetSettingsPath() const;
    bool ParseJson(const std::string& json);
    std::string SerializeJson() const;

    GeneralSettings   m_general;
    DisplaySettings   m_display;
    EditorSettings    m_editor;
    AdvancedSettings  m_advanced;
    ConnectionSettings m_connection;
    RecentFilesList   m_recentFiles;

    std::mutex m_mutex;
    std::unordered_map<std::string, std::vector<ChangeCallback>> m_observers;
    bool m_portableMode = false;
};
```

### Observer Pattern for Change Notification

```
SettingsManager                    UI Components
┌──────────────────┐              ┌──────────────────┐
│ SetTheme("dark") │──notify────▶│ ThemeManager::    │
│                  │              │   OnSettingChanged│
│ SetZoom(150)     │──notify────▶│ ViewerFrame::     │
│                  │              │   OnSettingChanged│
│ Save()           │              │                   │
└──────────────────┘              └──────────────────┘
```

```cpp
// Observer usage example
SettingsManager::Instance().RegisterObserver("display.theme",
    [](const std::string& key) {
        auto theme = SettingsManager::Instance().Display().theme;
        ThemeManager::Apply(theme);
    });
```

### Portable Mode

> **RECOMMENDATION**: Support portable mode where settings are stored next to the EXE in `settings.json`, enabling use from USB drives or network shares.

```
Portable Mode Detection:
  1. On startup, check for "portable.dat" marker file next to EXE
  2. If found → settings path = exe_dir\settings.json
  3. If not found → settings path = %APPDATA%\PDF Elite\settings.json
```

### Migration Strategy

> **ASSUMPTION**: Migration from the Tauri web app settings is not required for the initial native release. Users start fresh.

| Phase | Action |
|-------|--------|
| v1.0 | No migration; fresh settings file |
| v1.1 | Optional: import tool reads Tauri Store JSON and localStorage exports |
| v2.0 | Auto-detect legacy Tauri installation, offer migration wizard |

### File Watch for External Changes

```cpp
// Watch for settings.json changes from another instance
void SettingsManager::StartFileWatch() {
    m_fileWatchHandle = FindFirstChangeNotificationW(
        GetSettingsDir().c_str(), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
    // Background thread polls and reloads if changed
}
```

### Thread Safety

> **RECOMMENDATION**: All settings access goes through `SettingsManager::Instance()` with a `std::mutex`. Read-heavy workloads can use a reader-writer lock (`std::shared_mutex` in C++17).

### App Config (Feature Flags)

> **FACT**: App-level config comes from backend `/api/v1/config/app-config`. In the native version, feature flags should support both online (API fetch) and offline (bundled defaults) modes.

```cpp
struct FeatureFlags {
    bool cloudSyncEnabled = false;
    bool aiAssistEnabled = false;
    bool collaborativeEditing = false;
    bool ocrEnabled = true;
    bool formsEnabled = true;
    bool digitalSignatureEnabled = true;
    int  maxFileSizeMB = 500;

    void LoadFromApi(const json& response);
    void LoadDefaults();  // Bundled fallback
};
```

---

## Implementation Checklist

- [ ] Define all settings structs with serialization/deserialization
- [ ] Implement JSON file I/O with atomic save (write temp + rename)
- [ ] Implement SettingsManager singleton with thread-safe access
- [ ] Implement observer pattern for UI change notifications
- [ ] Implement portable mode detection and path resolution
- [ ] Create Settings UI dialog with category tabs
- [ ] Implement feature flags system with API + fallback
- [ ] Add settings file validation and error recovery
- [ ] Add command-line override flags for settings
