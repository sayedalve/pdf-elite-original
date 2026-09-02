# Crash Recovery

> Engineering doc for PDF Elite native C++/Win32/PDFium rebuild
> Target: File 31 of 31 in engineering documentation set

---

## 1. Current State

**FACT: The current application has no structured crash handling whatsoever.**

| Aspect | Current State |
|--------|---------------|
| Crash detection | Tauri's built-in panic handler only |
| Crash reporting | None |
| Auto-save | None — edits are lost on crash |
| Recovery on restart | None — no recovery prompt |
| Crash dumps | None |
| Document safety | No atomic save; corruption possible on crash during save |
| Update crash recovery | `tauri-plugin-updater` handles install failures, not data recovery |

### 1.1 What Tauri Provides

Tauri's built-in panic handler:
- Catches Rust panics
- Logs the panic message
- Terminates the application
- Does **not** create crash dumps
- Does **not** save in-progress work
- Does **not** report crashes to a server

This is the **absolute minimum** — acceptable for a v0.1 prototype but not for a production application.

---

## 2. Crash Recovery Architecture

### 2.1 Overview

```
┌──────────────────────────────────────────────────────┐
│                  Crash Prevention                    │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────┐ │
│  │  Auto-Save   │  │  Atomic Save │  │  SEH Guard  │ │
│  │  (30s timer) │  │  (temp+rename)│ │  (top-level) │ │
│  └──────┬───────┘  └──────┬───────┘  └─────┬──────┘ │
└─────────┼─────────────────┼────────────────┼────────┘
          │                 │                │
          ▼                 ▼                ▼
┌──────────────────────────────────────────────────────┐
│                  Crash Detection                      │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────┐ │
│  │   SEH Filter  │  │  WER Report  │  │  Minidump  │ │
│  └──────┬───────┘  └──────┬───────┘  └─────┬──────┘ │
└─────────┼─────────────────┼────────────────┼────────┘
          │                 │                │
          ▼                 ▼                ▼
┌──────────────────────────────────────────────────────┐
│                  Recovery on Restart                  │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────┐ │
│  │ Detect Saves │  │  User Prompt  │  │  Restore   │ │
│  │  (on launch) │  │  (recover?)   │  │  (open doc)│ │
│  └──────────────┘  └──────────────┘  └────────────┘ │
└──────────────────────────────────────────────────────┘
```

---

## 3. Structured Exception Handling (SEH)

### 3.1 SEH Policy

**RECOMMENDATION: Use SEH only at the top-level message loop.** Do not use `__try`/`__except` throughout the codebase — that encourages sloppy error handling and hides real bugs.

```cpp
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmd, int nShow) {
    // ... initialization ...

    // Top-level SEH guard around the message loop
    __try {
        MessageLoop();
    } __except (CrashFilter(GetExceptionInformation())) {
        HandleCrash(GetExceptionCode());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

LONG CrashFilter(PEXCEPTION_POINTERS ep) {
    // Log crash information
    WriteMinidump(ep);
    WriteDiagnosticDump();
    return EXCEPTION_EXECUTE_HANDLER;
}
```

### 3.2 SEH Rules

| Rule | Rationale |
|------|-----------|
| SEH only at WinMain top level | Catches truly unexpected crashes without hiding bugs |
| No `__try` in library code | Library code should use `Result<T>`, not SEH |
| No `__except` with `EXCEPTION_CONTINUE_EXECUTION` | Never resume after access violations — state is corrupted |
| Log before exiting | Ensure crash info is captured |

---

## 4. Minidump Generation

### 4.1 Implementation

```cpp
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

void WriteMinidump(PEXCEPTION_POINTERS ep) {
    auto dump_path = GetCrashDumpPath();

    HANDLE hFile = CreateFileW(
        dump_path.c_str(), GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr
    );
    if (hFile == INVALID_HANDLE_VALUE) return;

    MINIDUMP_EXCEPTION_INFORMATION mei = {};
    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = ep;
    mei.ClientPointers = FALSE;

    MiniDumpWriteDump(
        GetCurrentProcess(), GetCurrentProcessId(),
        hFile, MINIDUMP_TYPE(MiniDumpNormal | MiniDumpWithIndirectlyReferencedMemory),
        &mei, nullptr, nullptr
    );

    CloseHandle(hFile);
}
```

### 4.2 Dump Location

```
%LOCALAPPDATA%\PDF Elite\CrashDumps\
├── crash-2025-01-15-103045-a3f2b1c4.dmp
├── crash-2025-01-16-091523-7d4e8f2a.dmp
└── ...
```

### 4.3 Minidump Types

| Type | Size | Info Captured |
|------|------|---------------|
| `MiniDumpNormal` | Small (~few MB) | Stack traces, thread info |
| `MiniDumpWithDataSegs` | Medium | + global/static data |
| `MiniDumpWithIndirectlyReferencedMemory` | Medium-Large | + heap data referenced by stacks |

**RECOMMENDATION:** Use `MiniDumpNormal | MiniDumpWithIndirectlyReferencedMemory` for a good balance of size and diagnostic value.

---

## 5. Auto-Save System

### 5.1 Auto-Save Strategy

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Trigger | Every 30 seconds while document is modified | Balance between safety and performance |
| Also on | Focus loss, minimize, before tool operations | Preserve work at key moments |
| Location | `%LOCALAPPDATA%\PDF Elite\AutoSave\` | Standard app data location |
| Format | Full PDF file (via PDFium save) | No proprietary format needed |
| Cleanup | On successful user-initiated save | Remove auto-save when user saves manually |

### 5.2 Auto-Save Implementation

```cpp
class AutoSaveManager {
public:
    void Start(std::shared_ptr<IDocument> doc, const std::filesystem::path& original_path) {
        doc_ = doc;
        original_path_ = original_path;
        auto_save_path_ = GetAutoSavePath(original_path);
        timer_ = CreateTimerQueueTimer(
            &timer_handle_, nullptr,
            AutoSaveCallback, this,
            30000, 30000,  // 30 seconds
            WT_EXECUTEINTIMERTHREAD
        );
    }

    void TriggerSave() {
        if (!doc_ || !doc_->IsModified()) return;
        auto result = doc_->SaveAs(auto_save_path_.string());
        if (result.IsOk()) {
            last_save_time_ = std::chrono::system_clock::now();
        }
    }

    void OnUserSaved() {
        // User saved successfully — clean up auto-save
        std::error_code ec;
        std::filesystem::remove(auto_save_path_, ec);
        Stop();
    }

private:
    static void CALLBACK AutoSaveCallback(PVOID param, BOOLEAN fired) {
        static_cast<AutoSaveManager*>(param)->TriggerSave();
    }
};
```

### 5.3 Auto-Save File Naming

```
%LOCALAPPDATA%\PDF Elite\AutoSave\
├── autosave-<hash-of-original-path>.pdf
└── autosave-untitled-<timestamp>.pdf
```

---

## 6. Atomic Save

**FACT: The current web app has no atomic save mechanism.** A crash during save could corrupt the original file.

### 6.1 Atomic Save Protocol

```
1. Save to temp file:  original.pdf.tmp
2. Verify temp file:   Check file size > 0, try to open with PDFium
3. Atomic rename:      MoveFileEx(original.pdf.tmp → original.pdf, MOVEFILE_REPLACE_EXISTING)
4. If step 3 fails:    Delete temp file, report error
```

```cpp
Result<void> AtomicSave(IDocument& doc, const std::filesystem::path& path) {
    auto temp_path = path;
    temp_path += ".tmp";

    // Step 1: Save to temp
    auto result = doc.SaveAs(temp_path.string());
    if (result.IsErr()) {
        std::filesystem::remove(temp_path, ec);
        return result;
    }

    // Step 2: Verify
    auto verify = VerifyPdf(temp_path);
    if (verify.IsErr()) {
        std::filesystem::remove(temp_path, ec);
        return Result::Err(ErrorCode::SaveFailed, "Saved file is corrupt");
    }

    // Step 3: Atomic rename
    if (!MoveFileExW(temp_path.c_str(), path.c_str(),
                      MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DWORD err = GetLastError();
        std::filesystem::remove(temp_path, ec);
        return Result::Err(ErrorCode::SaveFailed, fmt::format("Rename failed: {}", err));
    }

    return Result::Ok();
}
```

### 6.2 Why This Works on Windows

`MoveFileEx` with `MOVEFILE_REPLACE_EXISTING` is **atomic on NTFS** for files on the same volume. The file system journal ensures that either the old or new data is visible after a crash — never a partial state.

---

## 7. Recovery on Restart

### 7.1 Startup Recovery Flow

```
Application Launch
       │
       ▼
Scan AutoSave directory
       │
       ├── No auto-save files found
       │       │
       │       ▼
       │   Normal startup
       │
       └── Auto-save files found
               │
               ▼
         Show Recovery Dialog
         ┌─────────────────────────────────────┐
         │  PDF Elite - Recovery               │
         │                                      │
         │  The following files have unsaved    │
         │  changes from a previous session:    │
         │                                      │
         │  ☑ invoice.pdf (modified 10:30:45)  │
         │  ☐ report.pdf (modified 09:15:23)   │
         │                                      │
         │  [Recover Selected] [Discard All]   │
         │  [Don't show recovery on startup]    │
         └─────────────────────────────────────┘
               │
               ├── User clicks "Recover Selected"
               │       │
               │       ▼
               │   Open auto-save files
               │   Set document as "modified" (prompt to save on close)
               │
               ├── User clicks "Discard All"
               │       │
               │       ▼
               │   Delete auto-save files
               │   Normal startup
               │
               └── User clicks "Don't show again"
                       │
                       ▼
                   Set preference, skip on future launches
```

### 7.2 Recovery Dialog Details

| Element | Specification |
|---------|--------------|
| Title | "PDF Elite - Document Recovery" |
| List | Checkbox list view with filename + modification time |
| Buttons | "Recover Selected", "Discard All", close (X) |
| Behavior | Default checkboxes ON for all files |
| Persistence | "Don't show again" saved in settings |

---

## 8. PDFium Crash Isolation

### 8.1 The Problem

PDFium is a large C/C++ library that processes untrusted input (PDF files). A malformed PDF could trigger a null pointer dereference, buffer overflow, or other crash inside PDFium, taking down the entire application.

### 8.2 Options

| Option | Complexity | Isolation | Performance | Recommendation |
|--------|-----------|-----------|-------------|----------------|
| In-process (current) | None | None | Best | ✅ **v1.0 — acceptable risk** |
| SEH around PDFium calls | Low | Minimal | Best | ✅ **v1.0 — implement** |
| Process-per-document | High | Full | Moderate | ⚠️ **v2.0 — if needed** |
| PDFium in separate process, shared memory rendering | Very High | Full | Moderate | ❌ Overkill |

**RECOMMENDATION for v1.0:**
1. Wrap all PDFium calls in SEH at the operation level (not individual calls)
2. Use a dedicated worker thread for PDFium operations
3. If the worker thread crashes, the main thread can detect it and recover

### 8.3 Worker Thread Watchdog

```cpp
class PdfiumWorker {
public:
    void Execute(std::function<void()> operation) {
        std::promise<WorkerResult> promise;
        auto future = promise.get_future();

        worker_thread_ = std::thread([&promise, op = std::move(operation)]() {
            try {
                op();
                promise.set_value({.success = true});
            } catch (...) {
                promise.set_value({.success = false, .crashed = true});
            }
        });

        // Wait with timeout
        if (future.wait_for(std::chrono::seconds(30)) == std::future_status::timeout) {
            // Thread is stuck or crashed — terminate it
            // Report to user, recover what we can
        }
    }
};
```

**ASSUMPTION:** PDFium crashes from malformed PDFs are rare in practice. The SEH + watchdog approach handles the 99% case. Process isolation is reserved for v2.0 if crash frequency is unacceptable.

---

## 9. Crash Reporting

### 9.1 Windows Error Reporting (WER)

**RECOMMENDATION: Register with WER for automatic crash collection.**

```cpp
// In application manifest or code:
WerRegisterRuntimeExceptionModule(
    &CustomWerModule,
    GetModuleHandle(nullptr)
);
```

WER provides:
- Automatic crash detection
- Minidump collection on Microsoft servers
- Access via Windows Dev Center dashboard
- No custom server infrastructure needed

### 9.2 Optional Custom Upload

For richer crash data (including our diagnostic dump):

| Component | Purpose |
|-----------|---------|
| Crash dump | Minidump for debugging |
| Diagnostic dump | Structured log of recent operations |
| System info | OS version, memory, CPU, GPU |
| User preference | Opt-in/opt-out in settings |

---

## 10. Implementation Priority

| Priority | Feature | Effort | Impact |
|----------|---------|--------|--------|
| P0 | Atomic save (temp + rename) | Low | Critical — prevents data loss on save |
| P0 | Top-level SEH with minidump | Low | Critical — crash diagnostics |
| P1 | Auto-save (30s timer) | Medium | High — prevents loss of unsaved edits |
| P1 | Recovery dialog on restart | Medium | High — user-facing recovery |
| P2 | PDFium worker thread watchdog | Medium | Medium — isolates PDFium crashes |
| P2 | WER registration | Low | Medium — automatic crash collection |
| P3 | Custom crash upload | High | Low — optional enhancement |
| P3 | Process-per-document isolation | Very High | Low — only if needed |

---

## 11. Comparison: Current vs. Proposed

| Aspect | Current | Proposed |
|--------|---------|----------|
| Crash detection | Tauri panic handler | SEH + minidump + WER |
| Auto-save | None | 30s timer + event-triggered |
| Atomic save | None | Temp file + MoveFileEx |
| Recovery | None | Startup dialog with recovery options |
| Crash dumps | None | MiniDumpWriteDump → .dmp files |
| Diagnostic data | None | Structured dump on crash |
| PDFium isolation | None (same process) | Worker thread + SEH (v1), process isolation (v2) |

---

*Document 31 of 31 — Crash Recovery*