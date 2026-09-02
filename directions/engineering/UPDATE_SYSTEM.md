# Update System

> Engineering doc for rebuilding the PDF Elite update subsystem as a native C++/Win32 application.

## Current Architecture (Tauri/Web)

### Update Implementation

| Component | Technology | Responsibility |
|-----------|-----------|----------------|
| tauri-plugin-updater | Rust crate | Update checking and downloading |
| GitHub Releases | API endpoint | Source of update metadata |
| Custom dialog | JavaScript UI | User notification (dialog: false) |
| Signature verification | Public key | Authenticates update integrity |
| Passive install | Windows | Background installation with UAC prompt |

> **FACT**: Update system uses `tauri-plugin-updater` with GitHub releases `latest.json` endpoint.

> **FACT**: Custom dialog is used (`dialog: false` in config), meaning no default Tauri update dialog — the app provides its own UI.

> **FACT**: Public key is used for signature verification of downloaded updates.

> **FACT**: Passive install mode on Windows — installation occurs in the background with minimal user interaction beyond UAC elevation.

> **FACT**: MDM provisioning via `stirling-provisioning.json` in `ProgramData/etc/Library`, supporting locked connection modes and update policies.

### Current Update Flow

```
App Startup / Manual Check
  └── tauri-plugin-updater
        └── GET https://github.com/<org>/pdf-elite/releases/latest/latest.json
              │
              ├── Parse response (version, download URL, notes, signature)
              │
              ├── Compare with current version
              │     ├── If up-to-date → silent exit
              │     └── If update available:
              │           │
              │           ├── Download update package (.msi / .exe)
              │           ├── Verify signature with public key
              │           ├── Show custom update notification dialog
              │           │     └── "Update available: v2.1.0 → v2.2.0"
              │           │           [Update Now] [Later] [Skip]
              │           └── On "Update Now":
              │                 ├── Launch installer (passive mode)
              │                 └── Exit application
              │
              └── Error handling:
                    ├── Network error → retry later
                    └── Signature mismatch → abort, log error
```

### MDM Provisioning

```
Provisioning file: C:\ProgramData\etc\Library\stirling-provisioning.json

{
  "connectionMode": "locked",
  "serverUrl": "https://enterprise.example.com",
  "updatePolicy": {
    "enabled": true,
    "channel": "stable",
    "autoUpdate": true,
    "blockedVersions": ["2.0.0", "2.0.1"],
    "minimumVersion": "2.1.0"
  }
}
```

---

## Proposed Native Architecture (C++/Win32)

### Update System Options

| Library | Pros | Cons | Verdict |
|---------|------|------|---------|
| WinSparkle | Mature, proven (used by many apps) | GitHub-only, less customization | ⚠️ Good for initial |
| Custom implementation | Full control, any source | More development effort | ✅ **Recommended** |
| squirrel.windows | Package management + updates | Large dependency, complex | ❌ Overkill |
| Windows MSIX | Native Windows update | Complex packaging, Store-only | ❌ Not suitable |

> **RECOMMENDATION**: Implement a custom update checker that fetches GitHub Releases metadata, downloads the update, verifies signatures, and launches the installer in passive mode. This provides full control over the update experience.

### Update Architecture Overview

```
CUpdateManager
├── Update Checker (background thread)
│     ├── GitHub Releases API
│     ├── Version comparison (semver)
│     └── Channel filtering (stable/beta)
├── Downloader (background thread)
│     ├── HTTPS download with progress
│     ├── Resume support (optional)
│     └── Integrity verification (SHA256 + signature)
├── Installer
│     ├── Passive mode launch
│     ├── UAC elevation
│     └── Previous version backup
├── MDM Integration
│     ├── Read provisioning.json
│     ├── Apply update policies
│     └── Version blocking
└── UI Notifications
      ├── Tray icon notification
      ├── In-app update banner
      └── Update progress dialog
```

### Update Manager

```cpp
class CUpdateManager {
public:
    struct UpdateInfo {
        std::string latestVersion;        // Semver string "2.2.0"
        std::wstring downloadUrl;
        std::wstring releaseNotesUrl;
        std::wstring releaseNotes;        // Markdown body
        uint64_t packageSizeBytes = 0;
        std::string sha256;                // Hash for verification
        std::string signature;            // Ed25519 signature
        bool isCritical = false;          // Security update flag
    };

    struct UpdateConfig {
        // Check interval
        enum Interval { Disabled, Daily, Weekly, Monthly, OnStart };
        Interval checkInterval = Daily;

        // Channel
        enum Channel { Stable, Beta, Dev };
        Channel channel = Stable;

        // Source
        std::wstring githubOwner = L"pdf-elite";
        std::wstring githubRepo = L"pdf-elite";
        std::wstring updateUrl;  // Override for enterprise

        // Behavior
        bool autoDownload = false;
        bool autoInstall = false;
        bool passiveInstall = true;  // Silent with UAC only
    };

    static CUpdateManager& Instance();

    // Initialization
    void Initialize(const UpdateConfig& config);
    void LoadMDMPolicy();

    // Update checking
    HRESULT CheckForUpdates(bool silent = false);
    HRESULT DownloadUpdate(const UpdateInfo& info);

    // State
    bool IsUpdateAvailable() const { return m_updateAvailable; }
    const UpdateInfo& GetUpdateInfo() const { return m_updateInfo; }
    bool IsDownloading() const { return m_downloading; }
    int GetDownloadProgress() const { return m_downloadProgress; }

    // Installation
    HRESULT InstallUpdate();

    // Rollback
    HRESULT RollbackToPrevious();

    // Callbacks
    using UpdateAvailableCallback = std::function<void(const UpdateInfo&)>;
    using DownloadProgressCallback = std::function<void(int percent)>;
    using UpdateCompleteCallback = std::function<void(HRESULT)>;

    void SetUpdateAvailableCallback(UpdateAvailableCallback cb);
    void SetDownloadProgressCallback(DownloadProgressCallback cb);

private:
    CUpdateManager() = default;

    // Network
    HRESULT FetchLatestRelease();
    HRESULT DownloadFile(const std::wstring& url,
                         const std::wstring& localPath);

    // Verification
    HRESULT VerifySignature(const std::wstring& filePath,
                             const std::string& signature);
    bool VerifySHA256(const std::wstring& filePath,
                      const std::string& expectedHash);

    // Background timer
    void StartAutoCheckTimer();
    void StopAutoCheckTimer();

    // State
    UpdateConfig m_config;
    UpdateInfo m_updateInfo;
    bool m_updateAvailable = false;
    bool m_downloading = false;
    int m_downloadProgress = 0;
    std::wstring m_downloadPath;

    // MDM
    struct MDMPolicy {
        bool updatesEnabled = true;
        std::string minimumVersion;
        std::vector<std::string> blockedVersions;
        std::string forcedChannel;
        bool autoUpdate = false;
    };
    MDMPolicy m_mdmPolicy;

    // Threading
    std::thread m_checkThread;
    std::thread m_downloadThread;
    std::atomic<bool> m_cancelDownload{false};

    // Timer
    HANDLE m_timerHandle = nullptr;

    // Callbacks
    UpdateAvailableCallback m_onUpdateAvailable;
    DownloadProgressCallback m_onDownloadProgress;
};
```

### GitHub Releases Check

```cpp
HRESULT CUpdateManager::FetchLatestRelease() {
    // Build URL based on channel
    std::wstring url;
    switch (m_config.channel) {
        case UpdateConfig::Stable:
            url = L"https://api.github.com/repos/" +
                  m_config.githubOwner + L"/" + m_config.githubRepo +
                  L"/releases/latest";
            break;
        case UpdateConfig::Beta:
            url = L"https://api.github.com/repos/" +
                  m_config.githubOwner + L"/" + m_config.githubRepo +
                  L"/releases/tags/beta";
            break;
    }

    // HTTPS GET request
    std::string response;
    HRESULT hr = HttpGet(url, response);
    if (FAILED(hr)) return hr;

    // Parse JSON response
    json root = json::parse(response);

    m_updateInfo.latestVersion = root["tag_name"];  // e.g. "v2.2.0"
    m_updateInfo.releaseNotes = UTF8ToWide(root["body"].get<std::string>());

    // Find Windows asset (.msi or .exe)
    for (auto& asset : root["assets"]) {
        std::string name = asset["name"];
        if (name.find(".msi") != std::string::npos ||
            name.find(".exe") != std::string::npos) {
            m_updateInfo.downloadUrl = UTF8ToWide(asset["browser_download_url"]);
            m_updateInfo.packageSizeBytes = asset["size"];
            break;
        }
    }

    // Extract signature from release body or separate asset
    // ...

    // Compare versions
    if (CompareVersions(m_updateInfo.latestVersion, GetCurrentVersion()) <= 0) {
        return S_FALSE;  // Already up to date
    }

    return S_OK;  // Update available
}
```

### MDM Provisioning

> **FACT**: MDM provisioning file is currently at `C:\ProgramData\etc\Library\stirling-provisioning.json`.

> **RECOMMENDATION**: Change the provisioning path to `C:\ProgramData\PDF Elite\provisioning.json` for consistency with the native application identity.

```cpp
void CUpdateManager::LoadMDMPolicy() {
    std::wstring path = L"C:\\ProgramData\\PDF Elite\\provisioning.json";

    // Check if file exists
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return;  // No MDM policy, use defaults
    }

    // Read and parse
    std::string content = ReadFileToString(path);
    json root = json::parse(content);

    if (root.contains("updatePolicy")) {
        auto& policy = root["updatePolicy"];
        m_mdmPolicy.updatesEnabled = policy.value("enabled", true);
        m_mdmPolicy.minimumVersion = policy.value("minimumVersion", "");
        m_mdmPolicy.autoUpdate = policy.value("autoUpdate", false);

        if (policy.contains("blockedVersions")) {
            for (auto& v : policy["blockedVersions"]) {
                m_mdmPolicy.blockedVersions.push_back(v.get<std::string>());
            }
        }
    }
}

bool CUpdateManager::IsVersionBlocked(const std::string& version) {
    // Check MDM blocked versions
    for (const auto& blocked : m_mdmPolicy.blockedVersions) {
        if (CompareVersions(version, blocked) == 0) return true;
    }

    // Check minimum version requirement
    if (!m_mdmPolicy.minimumVersion.empty()) {
        if (CompareVersions(version, m_mdmPolicy.minimumVersion) < 0) {
            return true;
        }
    }

    return false;
}
```

### MDM Policy Enforcement

| Policy | Effect |
|--------|--------|
| `updatesEnabled: false` | All update checks disabled |
| `minimumVersion: "2.1.0"` | Versions below 2.1.0 are force-updated |
| `blockedVersions: ["2.2.0"]` | Specific versions cannot be installed |
| `channel: "stable"` | Users cannot switch to beta/dev channel |
| `autoUpdate: true` | Updates downloaded and installed automatically |

### Signature Verification

```cpp
HRESULT CUpdateManager::VerifySignature(const std::wstring& filePath,
                                          const std::string& signature) {
    // 1. Read the update package
    std::vector<uint8_t> packageData = ReadFileToBytes(filePath);

    // 2. Decode signature (base64)
    std::vector<uint8_t> sigBytes = Base64Decode(signature);

    // 3. Verify with embedded public key (Ed25519)
    // Public key is compiled into the binary
    const uint8_t PUBLIC_KEY[] = { /* 32-byte Ed25519 public key */ };

    bool valid = Ed25519Verify(PUBLIC_KEY, sizeof(PUBLIC_KEY),
                               packageData.data(), packageData.size(),
                               sigBytes.data(), sigBytes.size());

    return valid ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_SIGNATURE);
}
```

> **ASSUMPTION**: Continue using Ed25519 signature verification, matching the current Tauri implementation.

### Download with Progress

```cpp
HRESULT CUpdateManager::DownloadUpdate(const UpdateInfo& info) {
    m_downloading = true;
    m_downloadProgress = 0;

    std::wstring tempPath = GetTempDir() + L"\\pdf-elite-update-" +
                            GetCurrentVersion() + L".msi";

    m_downloadThread = std::thread([this, info, tempPath]() {
        HRESULT hr = DownloadFileWithProgress(
            info.downloadUrl,
            tempPath,
            [this](int percent) {
                m_downloadProgress = percent;
                if (m_onDownloadProgress) {
                    m_onDownloadProgress(percent);
                }
            }
        );

        if (SUCCEEDED(hr)) {
            // Verify signature
            hr = VerifySignature(tempPath, info.signature);
            if (FAILED(hr)) {
                DeleteFileW(tempPath.c_str());
                return;
            }

            m_downloadPath = tempPath;
        }
    });

    return S_OK;
}
```

### Passive Installation

```cpp
HRESULT CUpdateManager::InstallUpdate() {
    if (m_downloadPath.empty()) return E_INVALIDARG;

    // Launch installer in passive mode (no user interaction beyond UAC)
    SHELLEXECUTEINFOW shex = { 0 };
    shex.cbSize = sizeof(SHELLEXECUTEINFOW);
    shex.fMask = SEE_MASK_NOCLOSEPROCESS;
    shex.lpVerb = L"open";  // or L"runas" to force UAC
    shex.lpFile = m_downloadPath.c_str();
    shex.lpParameters = L"/passive /norestart";

    if (!ShellExecuteExW(&shex)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Wait for installer to start, then exit application
    WaitForInputIdle(shex.hProcess, 5000);
    CloseHandle(shex.hProcess);

    // Exit application for update
    PostQuitMessage(0);

    return S_OK;
}
```

### Rollback Capability

> **RECOMMENDATION**: Keep the previous version's files in a backup directory. On rollback, restore from backup.

```
Installation Directory:
  C:\Program Files\PDF Elite\
  ├── PDFElite.exe          (v2.2.0)
  ├── pdfium.dll
  └── ...

  C:\Program Files\PDF Elite\updates\
  ├── v2.1.0-backup\
  │     ├── PDFElite.exe
  │     ├── pdfium.dll
  │     └── ...
  ├── v2.0.0-backup\
  │     └── ...
  └── rollback-marker.txt  (contains previous version string)

Rollback Flow:
  1. Check for rollback-marker.txt
  2. Read previous version
  3. Replace current files with backup
  4. Delete rollback marker
  5. Restart application
```

### Update UI

#### System Tray Notification

```
Windows System Tray:
  ┌─────────────────────────────┐
  │ 🔔 PDF Elite               │
  │                             │
  │ Update available v2.2.0     │
  │ [Download Update]          │
  │ [View Release Notes]       │
  │ [Remind me later]          │
  └─────────────────────────────┘
```

#### In-App Update Banner

```
┌──────────────────────────────────────────────────────────┐
│ 🔄 Update Available: PDF Elite v2.2.0                    │
│ New features: faster rendering, improved search          │
│                                    [Update] [Skip] [×]  │
└──────────────────────────────────────────────────────────┘
```

#### Update Progress Dialog

```
┌──────────────────────────────────────────────────────────┐
│ Downloading Update...                                     │
│                                                            │
│ ████████████████████░░░░░░░░░░░░░░░░░░  67%              │
│                                                            │
│ Downloaded: 45.2 MB / 67.5 MB                             │
│ Speed: 2.3 MB/s                                           │
│                                                            │
│                                        [Cancel]           │
└──────────────────────────────────────────────────────────┘
```

### Update Intervals

| Interval | Check Behavior |
|----------|---------------|
| Disabled | Never auto-check; manual only |
| On Start | Check once at application launch |
| Daily | Check every 24 hours |
| Weekly | Check every 7 days |
| Monthly | Check every 30 days |

```cpp
void CUpdateManager::StartAutoCheckTimer() {
    if (m_config.checkInterval == UpdateConfig::Disabled) return;

    DWORD intervalMs = 0;
    switch (m_config.checkInterval) {
        case UpdateConfig::OnStart:  intervalMs = 0; break;
        case UpdateConfig::Daily:    intervalMs = 24 * 60 * 60 * 1000; break;
        case UpdateConfig::Weekly:   intervalMs = 7 * 24 * 60 * 60 * 1000; break;
        case UpdateConfig::Monthly:  intervalMs = 30 * 24 * 60 * 60 * 1000; break;
    }

    if (intervalMs > 0) {
        // Use CreateTimerQueueTimer for reliable background timer
        CreateTimerQueueTimer(&m_timerHandle, nullptr,
                              UpdateTimerCallback, this,
                              intervalMs, intervalMs, 0);
    }

    // Always check on start
    if (m_config.checkInterval != UpdateConfig::Disabled) {
        CheckForUpdates(true);  // Silent check
    }
}
```

### Channel Support

| Channel | Update Source | Risk Level | User Opt-In |
|--------|-------------|------------|-------------|
| Stable | GitHub Releases (latest non-prerelease) | Low | Default |
| Beta | GitHub Releases (prerelease) | Medium | Explicit opt-in |
| Dev | GitHub Releases (dev builds) | High | Developer mode |

```
Channel Selection Flow:
  1. MDM policy specifies forced channel → use that
  2. Settings file specifies channel → use that
  3. Default → Stable
  4. Beta/Dev require explicit opt-in in settings
```

### Update Blocking Flow

```
New version detected (v2.3.0)
  │
  ├── MDM Policy Check:
  │     ├── updatesEnabled = false? → BLOCK, silent
  │     ├── blockedVersions contains "2.3.0"? → BLOCK, show message
  │     ├── minimumVersion > current? → FORCE UPDATE to minimum
  │     └── channel forced? → override user's channel selection
  │
  ├── Signature Check:
  │     └── Invalid? → BLOCK, log error, alert
  │
  └── All checks pass → PROCEED with download/install
```

---

## Implementation Checklist

- [ ] Implement `CUpdateManager` singleton with configuration
- [ ] Implement GitHub Releases API fetcher
- [ ] Implement semver version comparison
- [ ] Implement HTTPS file downloader with progress callback
- [ ] Implement Ed25519 signature verification
- [ ] Implement SHA256 hash verification
- [ ] Implement passive installer launch (`/passive /norestart`)
- [ ] Implement MDM provisioning file reader (`provisioning.json`)
- [ ] Implement MDM policy enforcement (blocking, forcing)
- [ ] Implement auto-check timer (daily/weekly/monthly)
- [ ] Implement system tray notification for available updates
- [ ] Implement in-app update banner UI
- [ ] Implement update progress dialog
- [ ] Implement channel selection (stable/beta/dev)
- [ ] Implement rollback to previous version (backup/restore)
- [ ] Implement update download caching (resume support)
- [ ] Test update flow end-to-end (check → download → install → verify)
