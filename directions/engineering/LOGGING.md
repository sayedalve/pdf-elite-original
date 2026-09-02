# Logging

> Engineering doc for PDF Elite native C++/Win32/PDFium rebuild
> Target: File 30 of 31 in engineering documentation set

---

## 1. Current State

### 1.1 Frontend Logging

**FACT:** The web frontend has **no structured logging**. Errors are caught by React error boundaries and displayed in the UI. Console logging is used for development only.

### 1.2 Rust/Tauri Logging

**FACT:** The Tauri backend has a simple logging implementation in `logging.rs` (approximately 90 lines):

- In-memory log buffer (circular, bounded)
- File logging to a log file in the app data directory
- Exposed via `get_tauri_logs` Tauri command for frontend retrieval

| Feature | Status |
|---------|--------|
| Log levels | Basic (likely debug/info/error) |
| Structured fields | None |
| Log rotation | Unknown (likely none) |
| Performance logging | None |
| Crash context logging | None |

### 1.3 Backend Logging (Stirling PDF)

**FACT:** The Java backend uses standard Spring Boot logging (Logback) with no custom structured fields relevant to the desktop app.

---

## 2. Proposed Logging Strategy

### 2.1 Library Selection

**RECOMMENDATION: Use [spdlog](https://github.com/gabime/spdlog)**

| Criteria | spdlog | Other Options |
|-----------|--------|---------------|
| Header-only option | ✅ Yes | Some require linking |
| Performance | Very fast (millions/sec) | Varies |
| Structured logging | Via custom fmt | Some built-in |
| File rotation | Built-in | Varies |
| Platform support | Windows, Linux, macOS | Varies |
| Maturity | Widely used, well-maintained | Varies |
| License | MIT | Varies |

### 2.2 Log Levels

| Level | Use Case | Example |
|-------|----------|---------|
| `trace` | Very fine-grained debugging | "Entering RenderTile(page=3, x=2, y=1)" |
| `debug` | Development debugging | "Cache hit rate: 87%" |
| `info` | Normal operations | "Document opened: invoice.pdf (12 pages)" |
| `warn` | Recoverable issues | "PDFium returned error for annotation on page 5" |
| `error` | Operation failures | "Failed to save document: access denied" |
| `critical` | Severe failures | "PDFium crashed rendering page 3" |

**RECOMMENDATION:** Default to `info` level in release builds, `debug` in debug builds.

---

## 3. Log Output Sinks

### 3.1 Sink Configuration

```cpp
void InitializeLogging(const std::filesystem::path& log_dir) {
    // 1. Rotating file sink (primary)
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_dir / "pdf-elite.log",
        10 * 1024 * 1024,  // 10 MB max per file
        5                    // 5 rotating files
    );

    // 2. Debug output sink (Visual Studio Output window)
    auto debug_sink = std::make_shared<spdlog::sinks::windebug_sink_mt>();

    // 3. Console sink (debug builds only)
    #ifdef _DEBUG
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    #endif

    // Compose logger
    std::vector<spdlog::sink_ptr> sinks = { file_sink, debug_sink };
    #ifdef _DEBUG
    sinks.push_back(console_sink);
    #endif

    auto logger = std::make_shared<spdlog::logger>("main", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::debug);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
    spdlog::set_default_logger(logger);
}
```

### 3.2 Sink Details

| Sink | Format | Purpose |
|------|--------|---------|
| Rotating file | `[2025-01-15 10:30:45.123] [info] [main] Document opened: invoice.pdf` | Persistent log for diagnostics |
| Win32 debug output | `[info] [main] Document opened: invoice.pdf` | Visual Studio debugger, DebugView |
| Console (debug) | Colored output | Development command-line debugging |

### 3.3 Log File Location

```
%LOCALAPPDATA%\PDF Elite\logs\
├── pdf-elite.log         (current log file)
├── pdf-elite.1.log       (rotated)
├── pdf-elite.2.log       (rotated)
├── pdf-elite.3.log       (rotated)
└── pdf-elite.4.log       (rotated)
```

---

## 4. Structured Logging

### 4.1 Structured Fields

Every log entry should include relevant contextual fields:

| Field | Source | Example |
|-------|--------|---------|
| `module` | Compile-time macro | `pdf_engine`, `rendering`, `ui`, `editing` |
| `operation` | Function/event name | `open_document`, `render_tile`, `create_annotation` |
| `document_id` | Document manager | `doc-a3f2b1c4` |
| `page` | Operation context | `5` |
| `duration_ms` | Timer | `23.5` |
| `file` | Source file | `render.cpp:142` |
| `error_code` | Result<T> | `CORRUPT_PDF` |
| `memory_mb` | Memory tracker | `142.3` |

### 4.2 Structured Log Format

```cpp
// Module-specific logger with structured fields
#define LOG_MODULE(module_name) \
    spdlog::get(module_name) ? \
        spdlog::get(module_name) : \
        spdlog::create(module_name)

// Usage
#define LOG_INFO(...)   spdlog::info(__VA_ARGS__)
#define LOG_WARN(...)   spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...)  spdlog::error(__VA_ARGS__)

// Structured log pattern (JSON-like for file sink, readable for console)
// File: [{"ts":"...","level":"info","module":"rendering","op":"render_tile",
//         "doc":"doc-a3f2b1","page":3,"dur_ms":23.5}] Message text
```

### 4.3 Module Loggers

```cpp
// Each module creates its own logger
namespace logging {
    void InitModule(const char* module_name) {
        auto logger = spdlog::get(module_name);
        if (!logger) {
            logger = spdlog::create(module_name);
        }
        logger->set_pattern(
            "[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v"
        );
    }
}

// Module init
logging::InitModule("pdf_engine");
logging::InitModule("rendering");
logging::InitModule("editing");
logging::InitModule("ui");
```

---

## 5. PDFium Logging Capture

### 5.1 Redirecting PDFium's Internal Logging

PDFium uses `FXLOG` internally. Redirect it to our logging system:

```cpp
void InstallPdfiumLogHandler() {
    // PDFium's logging goes through its own callback system
    // Capture and forward to spdlog
    FSDK_SetUnspObjProcessHandler([](FX_UNSPTYPE type, void* info) {
        // Forward to spdlog at appropriate level
    });
}
```

**ASSUMPTION:** PDFium's logging callback API may need investigation at implementation time. Fallback: wrap all PDFium calls with try/catch and log errors.

---

## 6. Performance Event Logging

### 6.1 Timing Utility

```cpp
class ScopedTimer {
public:
    ScopedTimer(const char* operation, const char* module = "default")
        : operation_(operation), module_(module), start_(std::chrono::high_resolution_clock::now()) {}

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count() / 1000.0;
        spdlog::info("[{}] {} completed in {:.1f}ms", module_, operation_, ms);
    }

private:
    const char* operation_;
    const char* module_;
    std::chrono::high_resolution_clock::time_point start_;
};

// Usage
void RenderPage(...) {
    ScopedTimer timer("render_page", "rendering");
    // ... rendering code ...
}
```

### 6.2 Performance Thresholds

| Operation | Warning Threshold | Error Threshold |
|-----------|-------------------|-----------------|
| Open document | > 2s | > 10s |
| Render single page (72 DPI) | > 200ms | > 1s |
| Render single page (300 DPI) | > 1s | > 5s |
| Search full document | > 3s | > 10s |
| Save document | > 1s | > 5s |
| Generate thumbnail | > 100ms | > 500ms |

---

## 7. Diagnostic Dump

On crash or user request, write a comprehensive diagnostic dump:

```cpp
void WriteDiagnosticDump(const std::filesystem::path& path) {
    std::ofstream out(path);
    out << "=== PDF Elite Diagnostic Dump ===\n";
    out << "Timestamp: " << GetCurrentTimeString() << "\n";
    out << "Version: " << APP_VERSION << "\n";
    out << "OS: " << GetOSVersionString() << "\n";
    out << "Memory: " << GetMemoryUsageMB() << " MB\n";
    out << "Open documents: " << doc_manager->GetOpenDocumentCount() << "\n";
    out << "Render cache: " << render_cache->GetMemoryUsage() << " bytes\n";
    out << "Thread count: " << GetActiveThreadCount() << "\n";
    out << "\n--- Recent Log Entries ---\n";
    // Dump last 200 log lines
}
```

---

## 8. User-Facing Log Viewer (Optional)

**RECOMMENDATION:** Add a simple log viewer in the Help menu for advanced users and debugging:

- **Menu path:** Help → View Logs → Opens log folder in Explorer
- **Dialog:** Simple text viewer showing last N lines of the log file
- **Purpose:** Support requests, self-diagnosis
- **Not in v1 scope** but cheap to add later

---

## 9. Migration from Current System

| Current | Proposed | Migration |
|---------|----------|-----------|
| Rust `logging.rs` (90 lines) | spdlog (header-only) | Complete rewrite |
| `get_tauri_logs` command | Diagnostic dump file | New implementation |
| Frontend console.log | spdlog debug sink | Replaced entirely |
| No structured fields | Structured with module/operation | New |
| No rotation | 10MB × 5 rotating files | New |

---

*Document 30 of 31 — Logging*