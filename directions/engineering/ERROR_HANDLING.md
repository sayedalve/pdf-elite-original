# Error Handling — PDF Elite C++/Win32/PDFium Rebuild

> **Document ID:** FILE-18 | **Version:** 1.0 | **Status:** Draft
> **Source Analysis:** PDF Elite v2.14.2 (Tauri v2 + React 19 + Java 25/Spring Boot)

---

## 1. Current Error Handling Analysis

### 1.1 Backend Error Handling

```
FACT (AGENTS.md): "Error handling: inconsistent — some use custom exceptions,
  some throw generic Exception"
```

| Component | Error Strategy | Issues |
|-----------|----------------|--------|
| **25+ API Controllers** | `@AutoJobPostMapping` with job tracking | Errors tracked per-job but inconsistent categorization |
| **TempFileManager** | Cleanup on error paths | May leave orphaned files if JVM crashes |
| **PDFBox operations** | Mix of custom + generic `Exception` | Callers cannot distinguish error types |
| **Spring Boot** | `@ControllerAdvice` global handler | Good pattern but limited by inconsistent exception hierarchy |

### 1.2 Frontend Error Handling

| Component | Error Strategy | Issues |
|-----------|----------------|--------|
| **toolErrorHandler** | Centralized utility function | Catches and formats tool errors, but coverage varies |
| **React hooks** | try/catch in async operations | Inconsistent — some hooks catch, others don't |
| **React Error Boundaries** | Component-level error catching | Catches render errors, not async errors |
| **Web Workers** | `onerror` + `onmessageerror` | Worker errors may not propagate to UI |
| **PDF.js** | Promise rejection handling | `.destroy()` errors silently swallowed in some paths |
| **EmbedPDF** | Internal error handling | Propagates errors as promise rejections |

### 1.3 Current Error Flow (Example)

```
User clicks "Merge PDFs"
  → React hook: try/catch around API call
    → Tauri IPC: invoke('merge_pdfs', { files })
      → Rust command: calls Spring Boot API
        → Java controller: @AutoJobPostMapping
          → PDFBox merge operation
            → throws generic Exception (inconsistent)
      ← Rust: propagates error
    ← Tauri IPC: rejects promise
  ← React: toolErrorHandler catches, shows toast
  → User sees: "An error occurred" (unhelpful)
```

```
PROBLEM: Error information is lost at each boundary.
  PDFBox knows it was a "password-protected PDF" but the user sees
  a generic "An error occurred" toast.
```

---

## 2. Proposed C++ Error Handling Architecture

### 2.1 Core Principle

```
RECOMMENDATION: Use std::expected<T, E> (C++23) as the primary error
  propagation mechanism. NEVER throw exceptions across module boundaries.
  Reserve exceptions ONLY for truly exceptional/unrecoverable conditions
  (OOM, corrupted stack, etc.).
```

### 2.2 Error Type Hierarchy

```cpp
// RECOMMENDATION: Structured error types using std::expected

#include <expected>
#include <string>
#include <system_error>

namespace pdf_elite {

// Base error — all errors carry a message and source location
struct Error {
    std::string message;          // User-facing (localized)
    std::string diagnostic;        // Developer-facing (English, detailed)
    std::string source_file;
    int source_line;
    std::string function_name;

    // For structured logging
    std::string category;          // "io", "pdf", "render", "oom", etc.
    int code;                      // Numeric error code within category
};

// Convenience type alias
template<typename T>
using Result = std::expected<T, Error>;

// Quick error construction
#define PDF_ERR(category, code, msg, diag) \
    Error{ msg, diag, __FILE__, __LINE__, __func__, category, code }

// Usage:
Result<int> GetPageCount() {
    if (!doc_) {
        return std::unexpected(
            PDF_ERR("pdf", 101, "Document not open",
                     "GetPageCount called on null document handle")
        );
    }
    int count = FPDF_GetPageCount(doc_);
    return count;
}
```

### 2.3 Error Categories and Codes

```
RECOMMENDATION: Hierarchical error categorization:

┌────────────┬──────┬──────────────────────────────────────┐
│ Category   │ Code │ Description                          │
├────────────┼──────┼──────────────────────────────────────┤
│ pdf        │ 1xx  │ PDFium operation failures             │
│            │ 101  │ Document not loaded                   │
│            │ 102  │ Invalid page index                   │
│            │ 103  │ Password required / incorrect         │
│            │ 104  │ Corrupt PDF structure                │
│            │ 105  │ Unsupported feature                  │
│            │ 106  │ Encryption error                     │
│            │ 107  │ Annotation error                     │
│            │ 108  │ Form field error                     │
│            │ 109  │ Bookmark/outline error               │
│            │ 110  │ Metadata read/write error             │
├────────────┼──────┼──────────────────────────────────────┤
│ io         │ 2xx  │ File system operations                │
│            │ 201  │ File not found                       │
│            │ 202  │ File already exists (overwrite fail)  │
│            │ 203  │ Permission denied                    │
│            │ 204  │ Disk full                            │
│            │ 205  │ File locked by another process       │
│            │ 206  │ Path too long                        │
│            │ 207  │ Invalid file name                    │
│            │ 208  │ Read/write error (I/O failure)       │
│            │ 209  │ Temp file creation failed            │
├────────────┼──────┼──────────────────────────────────────┤
│ render     │ 3xx  │ Rendering pipeline failures           │
│            │ 301  │ Page render timeout                  │
│            │ 302  │ Tile cache overflow                  │
│            │ 303  │ Unsupported color space              │
│            │ 304  │ Font rendering failure              │
│            │ 305  │ Image decoding failure               │
│            │ 306  │ ICC profile error                    │
├────────────┼──────┼──────────────────────────────────────┤
│ oom        │ 4xx  │ Memory allocation failures            │
│            │ 401  │ Out of memory (process budget)       │
│            │ 402  │ Tile cache exhausted                 │
│            │ 403  │ Too many documents open               │
│            │ 404  │ Buffer too small                     │
│            │ 405  │ Large document exceeds limits        │
├────────────┼──────┼──────────────────────────────────────┤
│ security   │ 5xx  │ Security-related failures              │
│            │ 501  │ Blocked JavaScript execution          │
│            │ 502  │ Blocked external URL                  │
│            │ 503  │ Blocked embedded file extraction      │
│            │ 504  │ File integrity check failed         │
│            │ 505  │ Path traversal attempt blocked       │
│            │ 506  │ DLL verification failed              │
├────────────┼──────┼──────────────────────────────────────┤
│ system     │ 6xx  │ Windows/OS failures                   │
│            │ 601  │ Win32 API failure                    │
│            │ 602  │ GDI resource exhausted              │
│            │ 603  │ Worker process crashed               │
│            │ 604  │ IPC communication failure            │
│            │ 605  │ Print spooler error                  │
│            │ 606  │ Clipboard access denied              │
└────────────┴──────┴──────────────────────────────────────┘
```

### 2.4 Error Propagation Rules

```
RECOMMENDATION: Strict rules for error handling at module boundaries:

Rule 1: Never throw exceptions across DLL/module boundaries
  - Public API functions return Result<T>
  - Exceptions are caught at the module boundary and converted to Result

Rule 2: Internal code MAY use exceptions for truly exceptional cases
  - std::bad_alloc (OOM)
  - std::runtime_error for logic errors that should never occur

Rule 3: Every fallible public function returns Result<T>
  - Constructors: use static factory methods (Result<PdfDocument> Open(...))
  - void functions: return Result<void>

Rule 4: Always propagate errors; never silently swallow
  - If an error is intentionally handled, log it at DEBUG level
  - If an error is unexpected, log it at ERROR level

Rule 5: Error messages are split into user-facing and diagnostic
  - message: localized, user-friendly, actionable
  - diagnostic: English, technical, with file:line:func
```

### 2.5 Pattern Examples

```cpp
// RECOMMENDATION: Factory pattern for fallible construction

class PdfDocument {
public:
    // Cannot throw from constructor — use factory
    static Result<PdfDocument> Open(const std::filesystem::path& path) {
        // Validate input
        if (!std::filesystem::exists(path)) {
            return std::unexpected(
                PDF_ERR("io", 201,
                        L"File not found: " + path.filename().wstring(),
                        "PdfDocument::Open — path does not exist: " + path.string())
            );
        }

        // Try PDFium load
        FPDF_DOCUMENT doc = FPDF_LoadDocument(path.string().c_str(), nullptr);
        if (!doc) {
            DWORD err = FPDF_GetLastError();
            return std::unexpected(PdfErrorFromFpdfium(err));
        }

        return PdfDocument(doc);
    }

    // Fallible page access
    Result<PdfPage> GetPage(int index) {
        if (index < 0 || index >= PageCount()) {
            return std::unexpected(
                PDF_ERR("pdf", 102,
                        "Invalid page number",
                        "GetPage index out of range: " + std::to_string(index) +
                        " (count: " + std::to_string(PageCount()) + ")")
            );
        }
        FPDF_PAGE page = FPDF_LoadPage(doc_, index);
        if (!page) {
            return std::unexpected(
                PDF_ERR("pdf", 104, "Failed to load page",
                         "FPDF_LoadPage returned null for index " + std::to_string(index))
            );
        }
        return PdfPage(page, index);
    }

    // Fallible save
    Result<void> Save(const std::filesystem::path& path) {
        // ... implementation
        return {};  // Success (Result<void>)
    }
};
```

```cpp
// RECOMMENDATION: Call-site pattern — propagate with context

Result<Bitmap> RenderPageThumbnail(PdfDocument& doc, int page_index, int dpi) {
    // Get page (propagates error if invalid)
    ASSIGN_OR_RETURN(page, doc.GetPage(page_index));

    // Render (propagates error if timeout/OOM)
    ASSIGN_OR_RETURN(bitmap, RenderPage(page, dpi));

    return bitmap;
}

// Helper macro for error propagation (like Rust's ? operator)
#define ASSIGN_OR_RETURN(lhs, expr) \
    auto _result_##__LINE__ = (expr); \
    if (!_result_##__LINE__.has_value()) { \
        return std::unexpected(std::move(_result_##__LINE__.error())); \
    } \
    lhs = std::move(_result_##__LINE__.value())
```

---

## 3. Recovery Strategies

### 3.1 Recovery Matrix

```
RECOMMENDATION: Each error category has a defined recovery strategy:

┌────────────┬────────────────────────────────────────────────────────┐
│ Category   │ Recovery Strategy                                       │
├────────────┼────────────────────────────────────────────────────────┤
│ pdf:101    │ Prompt user to select a document                        │
│ pdf:102    │ Clamp to valid range, render nearest valid page        │
│ pdf:103    │ Show password dialog, retry                             │
│ pdf:104    │ Show "corrupt file" error, offer repair attempt          │
│ pdf:105    │ Show "unsupported" message, suggest alternative        │
│ pdf:106    │ Show "encrypted" message, prompt for password/key       │
├────────────┼────────────────────────────────────────────────────────┤
│ io:201     │ Show "file not found", prompt to relocate               │
│ io:203     │ Show "access denied", suggest running as owner          │
│ io:204     │ Show "disk full", suggest freeing space                 │
│ io:205     │ Show "file in use", suggest closing other app          │
├────────────┼────────────────────────────────────────────────────────┤
│ render:301 │ Show placeholder, retry in background                  │
│ render:302 │ Evict old tiles, retry render                           │
│ render:305 │ Show image placeholder, skip broken image              │
├────────────┼────────────────────────────────────────────────────────┤
│ oom:401    │ Close least-recently-used documents, retry              │
│ oom:402    │ Evict tile cache, re-render on demand                  │
│ oom:403    │ Close background documents, prompt user to close docs │
│ oom:405    │ Switch to memory-mapped mode, warn about performance   │
├────────────┼────────────────────────────────────────────────────────┤
│ security:* │ Log full details, show generic security warning         │
│ system:603 │ Restart worker process, recover if possible             │
│ system:604 │ Retry IPC with backoff (100ms, 500ms, 1s)              │
└────────────┴────────────────────────────────────────────────────────┘
```

### 3.2 Graceful Degradation

```
RECOMMENDATION: Fallback rendering for when PDFium fails:

Level 0 (Full):     PDFium renders page with all features
Level 1 (Basic):    PDFium renders without annotations
Level 2 (Minimal):  PDFium renders without images, annotations, or fonts
Level 3 (Fallback): Show page dimensions with gray placeholder
Level 4 (Critical): Show page number text only

Degradation is per-page, not per-document. Different pages may render
at different levels depending on what triggers errors.

User is informed: "Page 5 rendered in basic mode — some features may not display"
```

### 3.3 Crash Protection

```
RECOMMENDATION: Windows SEH only at the top-level PDFium worker boundary.

  // In the PDFium worker process main loop:
  __try {
      // Execute PDFium operation
      result = ExecutePdfiumOperation(request);
  } __except (FilterException(GetExceptionCode())) {
      // Log the crash
      LogCrash(GetExceptionCode(), GetExceptionInformation());
      // Notify main process
      SendWorkerCrashNotification();
      // Exit worker (main process will restart it)
      ExitProcess(1);
  }

  NOTE: SEH is ONLY at the process boundary, never in application code.
  Application code uses Result<T> and RAII exclusively.
```

---

## 4. Error Logging

### 4.1 Log Structure

```
RECOMMENDATION: Structured JSON logging to file:

{
  "timestamp": "2024-01-15T14:23:01.123Z",
  "level": "error",
  "category": "pdf",
  "code": 104,
  "message": "Failed to load page",
  "diagnostic": "FPDF_LoadPage returned null for index 42 in document 'report.pdf'",
  "source": {
    "file": "pdf_document.cpp",
    "line": 187,
    "function": "GetPage"
  },
  "context": {
    "document_id": "abc-123",
    "document_path": "C:\\Users\\...\\report.pdf",
    "page_index": 42,
    "page_count": 100
  },
  "thread_id": 7,
  "process_id": 12345
}
```

### 4.2 Log Levels and Rotation

| Level | When to Use | Retention |
|-------|-------------|-----------|
| **TRACE** | Every PDFium API call entry/exit (debug builds only) | 1 day |
| **DEBUG** | Error recovery attempts, cache evictions, resource allocations | 7 days |
| **INFO** | User actions (open, save, close), configuration changes | 30 days |
| **WARN** | Graceful degradation, retry attempts, approaching limits | 90 days |
| **ERROR** | Operation failures, crash recovery, security events | 1 year |
| **FATAL** | Unrecoverable crash, data corruption | 1 year |

```
Log location: %LOCALAPPDATA%\PDF Elite\Logs\
Rotation: Max 50MB per file, max 10 files per level
         Total max: 500MB across all levels
```

### 4.3 User-Facing Error Messages

```
RECOMMENDATION: Error messages localized, actionable, non-technical.

  Bad:  "Error code: pdf:104 — FPDF_LoadPage returned null"
  Good:  "Could not display page 43 of 'Report.pdf'. The page may be
          corrupted. Try reopening the file or contact the sender."

  Bad:  "std::bad_alloc"
  Good:  "Not enough memory to open this file. Try closing other documents
          or applications, then try again."

  Bad:  "io:203 Permission denied"
  Good:  "Cannot save to this location. Choose a different folder or
          check that you have permission to write to this file."
```

---

## 5. Integration Error Handling

### 5.1 IPC Error Protocol (Main ↔ Worker)

```
RECOMMENDATION: Structured error serialization over IPC:

  Request:
    { "type": "render_page", "doc_id": "...", "page": 42, "dpi": 150 }

  Response (success):
    { "status": "ok", "bitmap_handle": 7, "width": 1200, "height": 1584 }

  Response (error):
    {
      "status": "error",
      "category": "pdf",
      "code": 104,
      "message": "Failed to load page",
      "diagnostic": "FPDF_LoadPage returned null for index 42",
      "retryable": false
    }
```

### 5.2 Win32 Error Integration

```cpp
// RECOMMENDATION: Convert Win32 errors to our Error type

inline Error Win32Error(DWORD code, const std::string& context) {
    LPSTR msg = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM,
                   nullptr, code, 0, (LPSTR)&msg, 0, nullptr);
    std::string system_msg = msg ? msg : "Unknown error";
    LocalFree(msg);

    return Error{
        /*message*/    context,
        /*diagnostic*/ system_msg + " (HRESULT: 0x" +
                       fmt::format("{:08X}", code) + ")",
        /*source_file*/ __FILE__,
        /*source_line*/ __LINE__,
        /*function*/   __func__,
        /*category*/   "system",
        /*code*/       static_cast<int>(code)
    };
}

// Usage:
HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                           nullptr, OPEN_EXISTING, 0, nullptr);
if (file == INVALID_HANDLE_VALUE) {
    return std::unexpected(Win32Error(GetLastError(), "Cannot open file"));
}
```

---

## 6. Comparison: Current vs. Proposed

| Aspect | Current (Tauri/React/Java) | Proposed (C++/Win32/PDFium) |
|--------|---------------------------|------------------------------|
| Primary mechanism | Exceptions (Java) + try/catch (JS) | `Result<T>` (no exceptions) |
| Error categorization | Inconsistent (mix of custom/generic) | Structured categories + codes |
| Cross-boundary errors | Lost at each layer (IPC, API) | Preserved across all boundaries |
| User messages | Generic "An error occurred" | Localized, actionable messages |
| Diagnostic info | Stack traces (Java), console (JS) | Structured JSON with context |
| Recovery | Limited (retry button on some tools) | Defined strategy per error category |
| Crash handling | JVM crash → user restarts app | Worker crash → auto-restart, recover state |
| Logging | Application logs + browser console | Structured JSON, rotation, levels |

---

*Next: See [TESTING.md](TESTING.md) for testing architecture.*
