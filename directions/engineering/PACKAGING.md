# Packaging & Distribution — PDF Elite Native (C++/Win32/PDFium)

> **Status:** Proposed | **Applies to:** Native C++ rebuild | **Last updated:** 2025-01

---

## Table of Contents

1. [Current Packaging Analysis](#current-packaging-analysis)
2. [Proposed Packaging Strategy](#proposed-packaging-strategy)
3. [Installer Design](#installer-design)
4. [Code Signing](#code-signing)
5. [Portable Distribution](#portable-distribution)
6. [System Integration](#system-integration)
7. [Auto-Update Mechanism](#auto-update-mechanism)
8. [Versioning Scheme](#versioning-scheme)
9. [Crash Reporting](#crash-reporting)
10. [CI/CD Pipeline](#cicd-pipeline)

---

## Current Packaging Analysis

### Current Installer Configurations

| Platform | Format | Tool | Current Config |
|----------|--------|------|----------------|
| **Windows** | NSIS | Tauri bundler | Primary installer format |
| **Windows** | WiX/MSI | Tauri bundler | Secondary format, upgrade code `536b4036-0d30-4fa5-b98e-515bda99f8b7` |
| **Linux** | .deb | Tauri bundler | dpkg-based distributions |
| **Linux** | .rpm | Tauri bundler | RPM-based distributions |
| **Linux** | .AppImage | Tauri bundler | Portable Linux format |
| **macOS** | .dmg | Tauri bundler | Disk image installer |
| **macOS** | .app | Tauri bundler | Application bundle, min macOS 10.15 |

### Current Windows Signing

| Aspect | Current Setting |
|--------|-----------------|
| Hash algorithm | SHA-256 |
| Timestamp server | DigiCert (`http://timestamp.digicert.com`) |
| Certificate type | EV Code Signing (inferred) |
| Sign tool | `signtool.exe` (Windows SDK) |

### Current Size Breakdown

| Component | Estimated Size | Notes |
|-----------|----------------|-------|
| JRE (bundled) | ~200MB | `runtime/jre/**/*` in bundle config |
| Java JARs | ~80-100MB | `libs/*.jar` from Gradle build |
| Frontend (JS/CSS/WASM) | ~30-50MB | Vite build output with PDF.js WASM |
| Tauri binary + Rust runtime | ~10-15MB | Compiled Rust code + plugins |
| Resources (icons, fonts) | ~5MB | App assets |
| **NSIS installer total** | **~300-500MB** | Compressed download |
| **Installed size** | **~400-600MB+** | Full extraction on disk |

> **FACT:** The current WiX configuration specifies upgrade code `536b4036-0d30-4fa5-b98e-515bda99f8b7` and SHA-256 digest. The native rebuild needs a new upgrade code since the application identity fundamentally changes.

---

## Proposed Packaging Strategy

### Format Selection

| Format | Recommendation | Rationale |
|--------|---------------|-----------|
| **NSIS** | ✅ Primary | Familiar to users, good compression, flexible scripting |
| **MSI (WiX)** | ⚠️ Optional | Enterprise deployment, GPO support, but more complex to maintain |
| **Portable ZIP** | ✅ Yes | Power users, USB drives, no admin required |
| **MSIX** | ❌ No | Overhead for a single-purpose desktop app |
| **AppX/Store** | ❌ No | Not targeting Microsoft Store initially |

### Size Targets

| Metric | Target | Rationale |
|--------|--------|-----------|
| Installer download | < 80MB | 5x smaller than current, fast download |
| Installed footprint | < 150MB (with PDB) / < 40MB (EXE only) | Minimal disk usage |
| Install time | < 15 seconds | Near-instant user experience |
| Uninstall cleanup | Complete | No orphaned files or registry entries |

### Package Contents

```
pdf-elite-3.0.0-setup.exe
├── pdf-elite.exe              ~30MB (with static PDFium linked)
│   └── Includes: PDFium static lib, all C++ modules
├── pdf-elite.pdb              ~15MB (optional, RelWithDebInfo only)
├── resources/
│   ├── icons/
│   │   ├── app.ico            Application icon
│   │   └── pdf-file.ico       File type icon
│   ├── fonts/
│   │   └── Inter-Regular.ttf  UI font (if not using system font)
│   └── strings/
│       └── en-US.rc           Localized strings
├── VCRedist/                  Optional (0-15MB)
│   └── vc_redist.x64.exe     Only if targeting older Win10
└── license.txt                Apache 2.0 + third-party notices
```

---

## Installer Design

### NSIS Script Structure

```nsi
; pdf-elite.nsi
!include "MUI2.nsh"
!include "FileFunc.nsh"

!define PRODUCT_NAME "PDF Elite"
!define PRODUCT_VERSION "3.0.0"
!define PRODUCT_PUBLISHER "PDF Elite Contributors"
!define PRODUCT_URL "https://pdf-elite.com"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UPGRADE_GUID "A1B2C3D4-E5F6-7890-ABCD-EF1234567890"

; === Installer Attributes ===
OutFile "pdf-elite-${PRODUCT_VERSION}-setup.exe"
InstallDir "$PROGRAMFILES64\PDF Elite"
InstallDirRegKey HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
Unicode true

; === Version Info ===
VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey "FileDescription" "PDF Elite Installer"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION}"

; === Pages ===
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; === Install Sections ===
Section "Main Application" SEC_MAIN
    SetOutPath "$INSTDIR"
    File "..\build\ninja-release\pdf-elite.exe"
    File "..\LICENSE"
    File "..\NOTICES.txt"

    ; Resources
    SetOutPath "$INSTDIR\resources"
    File /r "..\resources\*.*"

    ; Debug symbols (optional)
    ${If} ${SectionIsSelected} ${SEC_PDB}
        File "..\build\ninja-release\pdf-elite.pdb"
    ${EndIf}

    ; Uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; Registry entries
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_URL}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoRepair" 1
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "EstimatedSize" 35000  ; ~35MB

    ; File size for Add/Remove Programs
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "EstimatedSize" $0
SectionEnd

Section "Debug Symbols" SEC_PDB
    ; PDB files for crash analysis
SectionEnd

Section "File Association" SEC_ASSOC
    ; Register .pdf file association
    WriteRegStr HKCR ".pdf" "" "PDFElite.Document"
    WriteRegStr HKCR ".pdf" "Content Type" "application/pdf"
    WriteRegStr HKCR "PDFElite.Document" "" "PDF Document"
    WriteRegStr HKCR "PDFElite.Document\shell\open\command" "" \
        '"$INSTDIR\pdf-elite.exe" "%1"'
    WriteRegStr HKCR "PDFElite.Document\DefaultIcon" "" \
        '"$INSTDIR\resources\icons\pdf-file.ico,0"'
SectionEnd

; === Uninstall Section ===
Section "Uninstall"
    ; Kill running instances
    nsExec::ExecToLog 'taskkill /F /IM pdf-elite.exe'

    ; Remove files
    Delete "$INSTDIR\pdf-elite.exe"
    Delete "$INSTDIR\pdf-elite.pdb"
    Delete "$INSTDIR\uninstall.exe"
    Delete "$INSTDIR\LICENSE"
    Delete "$INSTDIR\NOTICES.txt"
    RMDir /r "$INSTDIR\resources"

    ; Remove install directory if empty
    RMDir "$INSTDIR"

    ; Remove registry
    DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"

    ; Remove file association
    DeleteRegKey HKCR "PDFElite.Document"

    ; Remove Start Menu
    Delete "$SMPROGRAMS\PDF Elite\*.lnk"
    RMDir "$SMPROGRAMS\PDF Elite"

    ; Remove Desktop shortcut
    Delete "$DESKTOP\PDF Elite.lnk"
SectionEnd

; === Section Descriptions ===
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAIN} "Core application files"
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_PDB} "Debug symbols for crash analysis (adds ~15MB)"
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_ASSOC} "Open PDF files with PDF Elite by default"
!insertmacro MUI_FUNCTION_DESCRIPTION_END
```

### WiX XML (Alternative)

| WiX Element | Value | Notes |
|-------------|-------|-------|
| `Product` Id | `A1B2C3D4-E5F6-7890-ABCD-EF1234567890` | New upgrade GUID (different from current) |
| `Product` Version | `3.0.0` | Semver major.minor.patch |
| `Package` Compressed | `yes` | Cabinet compression |
| `Media` Cabinet | `1.cab` | Single cabinet for small app |
| `Feature` | MainApplication | Core files + optional PDB + association |
| `Component` | FileAssociation | ProgId, Extension, DefaultIcon |
| `WixUI` | `WixUI_Minimal` | Simple install wizard |

---

## Code Signing

### Signing Configuration

| Aspect | Setting |
|--------|---------|
| **Hash algorithm** | SHA-256 (required by Windows 10+) |
| **Timestamp algorithm** | SHA-256 RFC 3161 |
| **Timestamp server** | `http://timestamp.digicert.com` |
| **Certificate type** | EV Code Signing recommended |
| **Sign tool** | `signtool.exe sign /fd SHA-256 /tr ... /td SHA-256` |
| **Dual signing** | Not needed (no Win7 support) |

### Sign Command

```bash
# Sign the executable
signtool sign \
    /fd SHA-256 \
    /tr http://timestamp.digicert.com \
    /td SHA-256 \
    /f "$CERT_PATH" \
    /p "$CERT_PASSWORD" \
    pdf-elite.exe

# Sign the installer (after building)
signtool sign \
    /fd SHA-256 \
    /tr http://timestamp.digicert.com \
    /td SHA-256 \
    /f "$CERT_PATH" \
    /p "$CERT_PASSWORD" \
    pdf-elite-3.0.0-setup.exe
```

### EV Code Signing Considerations

| Aspect | Detail |
|--------|--------|
| **Immediate SmartScreen** | EV certs bypass SmartScreen reputation check on first download |
| **Hardware token** | EV signing requires USB token (e.g., SafeNet) in CI |
| **CI integration** | Use cloud signing service (DigiCert KeyLocker, Azure Key Vault) |
| **Cost** | ~$300-500/year for standard, ~$500-900/year for EV |

> **RECOMMENDATION:** Use EV Code Signing to avoid SmartScreen warnings on fresh downloads. Budget for a cloud signing service for CI automation.

---

## Portable Distribution

### ZIP Package

```
pdf-elite-3.0.0-portable.zip
├── pdf-elite.exe
├── resources/
│   ├── icons/
│   ├── fonts/
│   └── strings/
├── LICENSE
└── NOTICES.txt
```

| Aspect | Setting |
|--------|---------|
| **Target size** | < 40MB compressed, < 50MB extracted |
| **No admin required** | Runs from any directory |
| **Settings location** | Same directory as EXE (portable.ini flag) |
| **File association** | Manual registration via menu option |
| **Auto-update** | Disabled by default (opt-in) |

### Portable Detection

```cpp
// Check if running in portable mode
bool IsPortableMode() {
    wchar_t exeDir[MAX_PATH];
    GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exeDir, L'\\');
    if (lastSlash) *lastSlash = L'\0';
    
    std::wstring marker = std::wstring(exeDir) + L"\\portable.ini";
    DWORD attrs = GetFileAttributesW(marker.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES);
}
```

---

## System Integration

### File Associations

| Extension | ProgId | Description | Icon |
|-----------|--------|-------------|------|
| `.pdf` | `PDFElite.Document` | PDF Document | `pdf-file.ico` |

### Registration Points

| Location | Entries |
|----------|---------|
| `HKCR\.pdf` | Default = `PDFElite.Document`, `Content Type` = `application/pdf` |
| `HKCR\PDFElite.Document` | Default = "PDF Document" |
| `HKCR\PDFElite.Document\shell\open\command` | `"<path>\pdf-elite.exe" "%1"` |
| `HKCR\PDFElite.Document\DefaultIcon` | `"<path>\resources\icons\pdf-file.ico,0"` |
| `HKLM\SOFTWARE\PDF Elite` | `InstallPath`, `Version` |
| `HKCU\SOFTWARE\PDF Elite` | User settings (portable: app directory) |

### Shortcuts

| Shortcut | Location | Target | Arguments |
|----------|----------|--------|----------|
| Start Menu | `%ProgramData%\Microsoft\Windows\Start Menu\Programs\PDF Elite\` | `pdf-elite.exe` | — |
| Desktop | `%USERPROFILE%\Desktop\` | `pdf-elite.exe` | — |

### Add/Remove Programs

| Registry Value | Example |
|----------------|---------|
| `DisplayName` | `PDF Elite` |
| `DisplayVersion` | `3.0.0` |
| `Publisher` | `PDF Elite Contributors` |
| `InstallLocation` | `C:\Program Files\PDF Elite` |
| `UninstallString` | `C:\Program Files\PDF Elite\uninstall.exe` |
| `DisplayIcon` | `C:\Program Files\PDF Elite\resources\icons\app.ico` |
| `EstimatedSize` | `35000` (KB, ~35MB) |
| `NoModify` | `1` |
| `NoRepair` | `1` |
| `URLInfoAbout` | `https://pdf-elite.com` |

---

## Auto-Update Mechanism

### Strategy: GitHub Releases with Differential Updates

| Aspect | Design |
|--------|--------|
| **Check frequency** | Every 24 hours (configurable) |
| **Update source** | GitHub Releases API (`/repos/{owner}/{repo}/releases/latest`) |
| **Delta updates** | Binary patching via bsdiff/zstd (optional, v2+) |
| **User prompt** | "Update available" notification with changelog |
| **Silent install** | Background download, prompt for restart |
| **Rollback** | Keep previous version binary until verified |

### Update Flow

```
1. App startup → check GitHub Releases API
2. Compare remote version > local version?
   └─ No → done
   └─ Yes →
      3. Show update notification with release notes
      4. User clicks "Update" →
         5. Download new EXE + resources to temp directory
         6. Verify SHA-256 hash of download
         7. Verify code signature of new binary
         8. Launch updater process (pdf-elite-updater.exe)
         9. Current process exits
        10. Updater replaces old files, cleans up
        11. Updater launches new pdf-elite.exe
        12. New version verifies startup, deletes updater
```

### Update Binary

| Component | Purpose |
|-----------|---------|
| `pdf-elite-updater.exe` | Standalone ~100KB binary that replaces files while main app is not running |
| Written in | C++ (Win32, no dependencies) or NSIS `/DUPDATE` mode |
| Verification | SHA-256 hash check, Authenticode signature verification |

### Version Check API

```cpp
// Minimal HTTP check using WinHTTP
struct UpdateInfo {
    std::wstring latestVersion;
    std::wstring downloadUrl;
    std::wstring sha256;
    std::wstring releaseNotes;
    uint64_t fileSize;
};

std::optional<UpdateInfo> CheckForUpdates(const std::wstring& currentVersion);
```

---

## Versioning Scheme

### Semver Compliance

| Format | Example | Meaning |
|--------|---------|---------|
| `MAJOR.MINOR.PATCH` | `3.0.0` | Major version 3, minor 0, patch 0 |
| `MAJOR.MINOR.PATCH-BUILD` | `3.0.0-rc.1` | Pre-release identifier |
| CMake `PROJECT_VERSION` | `3.0.0` | Embedded in binary via `/D` |
| `VS_VERSION_INFO` | `3.0.0.0` | Four-part version in resources |

> **FACT:** Current version is 2.14.2. The native rebuild starts at 3.0.0 to signal the architectural change.

### Version Embedding

```cmake
# Auto-generate version resource
configure_file(
    ${CMAKE_SOURCE_DIR}/resources/version.rc.in
    ${CMAKE_BINARY_DIR}/version.rc
    @ONLY
)
```

```rc
// version.rc.in
#define VER_MAJOR ${PROJECT_VERSION_MAJOR}
#define VER_MINOR ${PROJECT_VERSION_MINOR}
#define VER_PATCH ${PROJECT_VERSION_PATCH}
#define VER_STR "${PROJECT_VERSION}"
#include "version.rc2"  // Standard VS_VERSION_INFO template
```

---

## Crash Reporting

### Windows Error Reporting (WER) Integration

| Aspect | Setting |
|--------|---------|
| **WER registration** | Via `WerRegisterExcludedModule` / `WerRegisterRuntimeExceptionModule` |
| **Minidump** | Custom `MiniDumpWriteDump` handler for richer data |
| **Upload** | Optional: POST minidump to crash server |
| **Local storage** | `%LOCALAPPDATA%\PDF Elite\Crashes\` |
| **Symbols** | PDB files (shipped in installer or uploaded to symbol server) |

### Crash Handler

```cpp
// Install crash handler at startup
void InstallCrashHandler() {
    SetUnhandledExceptionFilter(CrashHandler);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
}

LONG WINAPI CrashHandler(EXCEPTION_POINTERS* exceptionInfo) {
    // Write minidump
    wchar_t crashPath[MAX_PATH];
    GetLocalAppDataPath(crashPath, L"PDF Elite\\Crashes");
    CreateDirectoryW(crashPath, nullptr);
    
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t dumpFile[MAX_PATH];
    swprintf_s(dumpFile, L"%s\\crash-%04d%02d%02d-%02d%02d%02d.dmp",
        crashPath, st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);
    
    HANDLE hFile = CreateFileW(dumpFile, GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei;
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = exceptionInfo;
        mei.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
            hFile, MiniDumpNormal, &mei, nullptr, nullptr);
        CloseHandle(hFile);
    }
    
    // Show user-friendly dialog
    MessageBoxW(nullptr,
        L"PDF Elite has encountered an error and needs to close. "
        L"A crash report has been saved.",
        L"PDF Elite", MB_OK | MB_ICONERROR);
    
    return EXCEPTION_EXECUTE_HANDLER;
}
```

---

## CI/CD Pipeline

### Build & Package Pipeline

```yaml
# .github/workflows/build-windows.yml
name: Build Windows
on:
  push:
    branches: [main]
    tags: ['v*']
    paths: ['src/**', 'cmake/**', 'resources/**', 'CMakeLists.txt']

jobs:
  build:
    runs-on: windows-2022
    steps:
      - uses: actions/checkout@v4
      
      - name: Configure
        run: cmake --preset ninja-release
      
      - name: Build
        run: cmake --build build/ninja-release
      
      - name: Test
        run: cd build/ninja-release && ctest --output-on-failure
      
      - name: Sign
        if: github.ref_type == 'tag'
        run: |
          signtool sign /fd SHA-256 /tr http://timestamp.digicert.com /td SHA-256 \
            /f ${{ secrets.CERT_FILE }} /p ${{ secrets.CERT_PASSWORD }} \
            build/ninja-release/pdf-elite.exe
      
      - name: Build NSIS
        run: makensis installer/pdf-elite.nsi
      
      - name: Sign Installer
        if: github.ref_type == 'tag'
        run: |
          signtool sign /fd SHA-256 /tr http://timestamp.digicert.com /td SHA-256 \
            /f ${{ secrets.CERT_FILE }} /p ${{ secrets.CERT_PASSWORD }} \
            pdf-elite-${{ env.VERSION }}-setup.exe
      
      - name: Upload Artifact
        uses: actions/upload-artifact@v4
        with:
          name: pdf-elite-windows
          path: pdf-elite-*-setup.exe
      
      - name: Create Release
        if: github.ref_type == 'tag'
        uses: softprops/action-gh-release@v2
        with:
          files: pdf-elite-*-setup.exe
```

> **FACT:** The current project has 40+ GitHub Actions workflows with path-based dispatch. The native rebuild consolidates this to approximately 5-8 focused workflows (build, test, lint, release, sign).

> **RECOMMENDATION:** Unlike the current 3-stage Docker build (Gradle+JDK → JAR extract → pre-built base image), the Windows CI runs directly on `windows-2022` runners with no Docker overhead. Build time drops from ~15-30 minutes to ~2-5 minutes.