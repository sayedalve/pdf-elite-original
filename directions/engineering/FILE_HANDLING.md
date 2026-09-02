# File Handling

> Engineering doc for rebuilding the PDF Elite file handling subsystem as a native C++/Win32 application.

## Current Architecture (Tauri/Web)

### File Handling Layers

| Component | Technology | Responsibility |
|-----------|-----------|----------------|
| Tauri fs plugin | Rust + system APIs | Native file system access |
| FileContext | TypeScript/React | Manages file state in frontend |
| IndexedDB | Browser API | Persists document data in WebView |
| EmbedPDF | JavaScript library | PDF loading and rendering |
| ViewerContext | TypeScript/React | Coordinates file viewing state |

> **FACT**: Tauri fs plugin provides native file access. FileContext manages file state in the frontend. Documents are persisted in IndexedDB.

> **FACT**: Drag-drop is disabled in `tauri.conf.json` (`dragDropEnabled: false`).

> **FACT**: File associations registered at install with `.pdf` extension (role: Editor).

### Current File Open Flow

```
User Action (open file)
  ├── File Association (.pdf double-click)
  │     └── OS launches PDF Elite with file path as argument
  ├── Command Line
  │     └── tauri CLI argument parsing
  ├── Menu (File > Open)
  │     └── Tauri invoke('open_files_in_new_window')
  └── Drag-Drop
        └── DISABLED (dragDropEnabled: false)
              │
              ▼
        FileContext.addFile(file)
              │
              ▼
        ViewerContext.setActive(file.id)
              │
              ▼
        EmbedPDF loads document
              │
              ▼
        PDF rendered in WebView canvas
```

### Current Save Flow

```
User clicks Save/Export
  └── EmbedPDF ExportPlugin
        └── Creates modified PDF blob
              └── Browser download API
                    └── File saved to Downloads
```

> **FACT**: Save is handled by EmbedPDF's ExportPlugin which downloads the modified PDF via the browser download mechanism.

> **FACT**: Temp files on the backend are managed by Java `TempFileManager`.

> **FACT**: Recent files list is **MISSING** in the current application (marked in feature audit).

---

## Proposed Native Architecture (C++/Win32)

### File Open Flow

```
User Action (open file)
  ├── File Association (.pdf)
  │     └── ShellExecute → PDF Elite.exe "C:\path\file.pdf"
  ├── Command Line
  │     └── PDF Elite.exe /open "C:\path\file.pdf"
  │     └── PDF Elite.exe /print "C:\path\file.pdf"
  ├── Menu (File > Open)
  │     └── IFileOpenDialog (COM) → returns path(s)
  └── Drag-Drop
        └── IDropTarget::Drop → STGMEDIUM with HDROP
              │
              ▼
        CDocumentManager::OpenFile(path)
              │
              ├── Validate file (exists, extension, size)
              ├── Check if already open (reuse tab)
              ├── Create CDocument instance
              │     └── PDFium FPDF_LoadMemDocument / FPDF_LoadDocument
              ├── Add to tab model
              └── Render first page
```

### File Open Implementation

```cpp
// document_manager.h
class CDocumentManager {
public:
    static CDocumentManager& Instance();

    // Open
    HRESULT OpenFile(const std::wstring& path, bool addToRecent = true);
    HRESULT OpenFiles(const std::vector<std::wstring>& paths);
    HRESULT OpenFromCommandLine(int argc, wchar_t* argv[]);

    // Close
    HRESULT CloseDocument(DocId id, bool savePrompt = true);
    HRESULT CloseAllDocuments();

    // Save
    HRESULT SaveDocument(DocId id);
    HRESULT SaveDocumentAs(DocId id, const std::wstring& newPath);

    // Query
    CDocument* GetDocument(DocId id);
    bool IsDocumentOpen(const std::wstring& path);
    DocId GetActiveDocumentId();
    size_t OpenDocumentCount() const;

    // Recent files
    void AddToRecent(const std::wstring& path, int page = 0, int zoom = 100);
    std::vector<RecentFileEntry> GetRecentFiles() const;

private:
    CDocumentManager();
    std::map<DocId, std::unique_ptr<CDocument>> m_documents;
    DocId m_activeDocId = 0;
    DocId m_nextDocId = 1;
    std::mutex m_mutex;
};
```

### File Access Strategy

| Operation | Win32 API | Notes |
|-----------|-----------|-------|
| Open for read | `CreateFileW` with `GENERIC_READ`, `FILE_SHARE_READ` | Share mode allows other apps to read |
| Open for write | `CreateFileW` with `GENERIC_READ \| GENERIC_WRITE` | Exclusive access for saving |
| Large file read | Memory-mapped file (`CreateFileMappingW` + `MapViewOfFile`) | For files >100MB, avoid loading entire file into heap |
| Small file read | `ReadFile` into `std::vector<uint8_t>` | For files <100MB, simpler approach |
| Directory listing | `FindFirstFileW` / `FindNextFileW` | For folder browsing |
| File existence | `GetFileAttributesW` | Check `INVALID_FILE_ATTRIBUTES` |

> **RECOMMENDATION**: Use memory-mapped files for documents larger than 100MB. PDFium supports loading from memory buffers, so `MapViewOfFile` can provide the buffer directly.

### Atomic Save Pattern

```cpp
HRESULT CDocumentManager::SaveDocument(DocId id) {
    auto* doc = GetDocument(id);
    if (!doc) return E_INVALIDARG;

    // 1. Generate modified PDF via PDFium
    std::vector<uint8_t> pdfData = doc->GetModifiedPdfData();

    // 2. Write to temp file
    std::wstring tempPath = doc->GetPath() + L".tmp";
    HRESULT hr = WriteFileAtomic(tempPath, pdfData);
    if (FAILED(hr)) return hr;

    // 3. Create backup of original
    std::wstring backupPath = doc->GetPath() + L".bak";
    if (!doc->HasBackup()) {
        CopyFileW(doc->GetPath().c_str(), backupPath.c_str(), FALSE);
        doc->SetHasBackup(true);
    }

    // 4. Atomic rename: temp → original
    if (!MoveFileExW(tempPath.c_str(), doc->GetPath().c_str(),
                     MOVEFILE_REPLACE_EXISTING)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    doc->MarkClean();
    return S_OK;
}
```

### Save As Dialog

```cpp
HRESULT CDocumentManager::SaveDocumentAs(DocId id, const std::wstring& suggestedPath) {
    COMPtr<IFileSaveDialog> pDialog;
    HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr,
                                   CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDialog));
    if (FAILED(hr)) return hr;

    // Set PDF filter
    COMDLG_FILTERSPEC filters[] = {
        { L"PDF Documents", L"*.pdf" },
        { L"All Files", L"*.*" }
    };
    pDialog->SetFileTypes(2, filters);
    pDialog->SetDefaultExtension(L"pdf");

    if (!suggestedPath.empty()) {
        pDialog->SetFileName(suggestedPath.c_str());
    }

    hr = pDialog->Show(m_hWnd);
    if (FAILED(hr)) return hr;  // User cancelled

    COMPtr<IShellItem> pItem;
    pDialog->GetResult(&pItem);

    PWSTR pPath = nullptr;
    pItem->GetDisplayName(SIGDN_FILESYSPATH, &pPath);
    std::wstring newPath(pPath);
    CoTaskMemFree(pPath);

    // Copy document to new location
    return SaveCopyTo(id, newPath);
}
```

### Multiple Document Management (Tabs)

```
CTabManager
├── Tab 1: CDocument (C:\reports\Q4.pdf)
│     ├── FPDF_DOCUMENT (PDFium handle)
│     ├── Modified flag
│     ├── Undo stack
│     └── Per-doc settings (zoom, page, scroll pos)
├── Tab 2: CDocument (C:\invoices\001.pdf)
│     └── ...
└── Tab N: ...
```

> **RECOMMENDATION**: Each tab owns its own `CDocument` instance with an independent PDFium document handle. This isolates undo stacks, dirty state, and per-document settings.

### Drag-Drop Implementation

> **FACT**: Drag-drop is currently disabled in the Tauri app. The native version should fully support it.

```cpp
class CDropTarget : public IDropTarget {
public:
    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject* pDataObj, DWORD grfKeyState,
                           POINTL pt, DWORD* pdwEffect) override;
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
    STDMETHODIMP DragLeave() override;
    STDMETHODIMP Drop(IDataObject* pDataObj, DWORD grfKeyState,
                      POINTL pt, DWORD* pdwEffect) override;

private:
    std::vector<std::wstring> ExtractFileNames(IDataObject* pDataObj);
    bool AcceptsFormat(IDataObject* pDataObj);
};
```

```cpp
// Register drop target on main window
HRESULT RegisterDropTarget(HWND hWnd) {
    CDropTarget* pDropTarget = new CDropTarget();
    HRESULT hr = RegisterDragDrop(hWnd, pDropTarget);
    // Store reference for cleanup
    return hr;
}
```

### Encrypted PDF Handling

> **FACT**: Current app uses a queue-based password prompt system (`encryptedQueue`, `activeEncryptedFileId`, `EncryptedPdfUnlockModal`).

```
Proposed Native Flow:
  1. FPDF_LoadMemDocument fails with FPDF_ERR_PASSWORD
  2. Show modal dialog: "Enter password for [filename]"
  3. User enters password → retry FPDF_LoadMemDocument with password
  4. If fails again → show dialog again with error message
  5. If user cancels → abort opening, remove tab
```

```cpp
// PDFium password callback
int PasswordCallback(void* param, int pdfDocIndex, const char* unused1,
                     const char* unused2, char* buf, int bufLen) {
    auto* prompter = static_cast<PasswordPrompter*>(param);
    std::wstring password = prompter->ShowPasswordDialog();

    if (password.empty()) return 0;  // No password / cancel

    // Convert to UTF-8 for PDFium
    std::string utf8 = WideToUTF8(password);
    int copyLen = std::min(static_cast<int>(utf8.size()), bufLen - 1);
    memcpy(buf, utf8.c_str(), copyLen);
    buf[copyLen] = '\0';
    return copyLen;
}
```

### Recent Files (MRU List)

> **FACT**: Current app has NO recent files list (marked MISSING in feature audit).

> **RECOMMENDATION**: Implement MRU (Most Recently Used) list with max 20 entries, persisted in settings.json, integrated with Windows Jump List.

```cpp
struct RecentFileEntry {
    std::wstring path;
    FILETIME lastOpened;
    int page = 0;
    int zoom = 100;
};

class CRecentFiles {
public:
    static constexpr size_t MAX_ENTRIES = 20;

    void Add(const std::wstring& path, int page, int zoom);
    void Remove(const std::wstring& path);
    const std::vector<RecentFileEntry>& GetList() const;

    void UpdateJumpList();  // Windows 7+ taskbar integration

private:
    std::vector<RecentFileEntry> m_entries;
    void Prune();  // Remove entries pointing to deleted files
};
```

```
Jump List Integration:
  ┌──────────────────────┐
  │ PDF Elite (taskbar)  │
  │   Recent             │
  │   ├── report.pdf     │ ← ICustomDestinationList
  │   ├── invoice.pdf    │
  │   └── notes.pdf      │
  │   Tasks              │
  │   ├── New PDF         │
  │   └── Open File       │
  └──────────────────────┘
```

### File Associations

> **FACT**: Current app registers `.pdf` file association at install (role: Editor).

```cpp
// Register file association at install (not at runtime)
// Written to registry during installer:
HKEY_CLASSES_ROOT\.pdf
    (Default) = "PDFElite.Document"
HKEY_CLASSES_ROOT\PDFElite.Document
    (Default) = "PDF Document"
    DefaultIcon = "C:\Program Files\PDF Elite\PDFElite.exe,0"
    shell\open\command = "\"C:\Program Files\PDF Elite\PDFElite.exe\" /open \"%1\""
    shell\print\command = "\"C:\Program Files\PDF Elite\PDFElite.exe\" /print \"%1\""
```

> **RECOMMENDATION**: Register file associations during installer execution, not at runtime. Use a proper WiX or NSIS installer that handles protocol and file type registration correctly.

### Command-Line Arguments

```
PDF Elite.exe [options] [file...]
  /open "path"      Open file(s) in new window/tab
  /print "path"     Print file(s) and exit
  /quiet            Suppress UI (batch operations)
  /portable         Force portable mode
  /config "path"    Use specific config file
  /debug            Enable debug logging
```

```cpp
class CCommandLineParser {
public:
    struct Options {
        std::vector<std::wstring> filesToOpen;
        std::vector<std::wstring> filesToPrint;
        bool quiet = false;
        bool portable = false;
        std::wstring configPath;
        bool debug = false;
    };

    static Options Parse(int argc, wchar_t* argv[]);
};
```

### File Watching (Optional)

> **RECOMMENDATION**: Use `ReadDirectoryChangesW` or `FindFirstChangeNotificationW` to detect external modifications to open documents.

```
File Watch Flow:
  1. On document open → start watching parent directory
  2. Notification received → compare file timestamp with known timestamp
  3. If file changed externally:
     a. If document is clean → reload automatically
     b. If document is dirty → prompt user: "Reload / Keep mine / Save mine"
```

### Backup Strategy

| Scenario | Backup Action |
|----------|--------------|
| First save of opened file | Copy original to `.bak` |
| Subsequent saves | Overwrite `.bak` only if previous `.bak` is older than 5 min |
| Save As to new path | No backup created |
| Auto-save | Write to `.autosave` file, don't modify original |
| Crash recovery | On next launch, scan for `.autosave` files, prompt recovery |

---

## Implementation Checklist

- [ ] Implement `CDocumentManager` with open/close/save/save-as
- [ ] Implement `CreateFileW` + memory-mapped file loading for PDFium
- [ ] Implement atomic save pattern (temp → rename) with backup
- [ ] Implement `IFileOpenDialog` and `IFileSaveDialog` integration
- [ ] Implement tab-based multi-document management
- [ ] Implement `IDropTarget` for drag-drop support
- [ ] Implement encrypted PDF password dialog with PDFium callback
- [ ] Implement `CRecentFiles` with MRU list and Windows Jump List
- [ ] Implement command-line argument parser (`/open`, `/print`, `/quiet`)
- [ ] Implement file change detection (optional)
- [ ] Define file association registry entries for installer
- [ ] Implement crash recovery (`.autosave` scanning on startup)
