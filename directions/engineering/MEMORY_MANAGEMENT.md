# Memory Management — PDF Elite C++/Win32/PDFium Rebuild

> **Document ID:** FILE-16 | **Version:** 1.0 | **Status:** Draft
> **Source Analysis:** PDF Elite v2.14.2 (Tauri v2 + React 19 + EmbedPDF + Java 25/Spring Boot)

---

## 1. Current Memory Architecture

### 1.1 Memory Domains in Existing Stack

| Domain | Technology | Approximate Baseline | Growth Pattern | Risk Level |
|--------|-----------|---------------------|----------------|------------|
| **WebView Process** | Chromium (Tauri v2) | 80–150 MB | Scales with DOM complexity, active tabs | 🔴 HIGH |
| **Java JVM Heap** | Spring Boot 4.0.6 / Java 25 | 200–400 MB | Grows with concurrent job processing | 🔴 HIGH |
| **PDFium WASM Linear Memory** | EmbedPDF (Pdfium WASM) | 64–256 MB (per module) | Grows per-document; never shrinks below high-water mark | 🟡 MEDIUM |
| **JavaScript Heap** | React 19 + V8 | 50–100 MB | Grows with state, blob URLs, React reconciliation | 🟡 MEDIUM |
| **IndexedDB / Origin Storage** | Browser storage | Unbounded | LRU-cached thumbnails, file blobs | 🟡 MEDIUM |

**FACT:** The current application runs **four separate memory domains** simultaneously — WebView, JVM, WASM linear memory, and JS heap. Each has its own garbage collector or manual cleanup requirements.

### 1.2 Known Memory Leaks and Cleanup Patterns

```
FACT (AGENTS.md): "Memory management: manual cleanup for PDF.js documents (destroy()),
blob URLs (revokeObjectURL), Web Workers (terminate)"
```

Current cleanup code patterns observed:

| Resource | Cleanup Mechanism | Location | Failure Mode |
|----------|-----------------|----------|--------------|
| `PDF.js document` | `pdfDoc.destroy()` | Frontend `useEffect` return | Missed on unmount if hook errors |
| `Blob URL` | `URL.revokeObjectURL(blobUrl)` | Component cleanup | Accumulates if error boundary catches |
| `Web Worker` | `worker.terminate()` | `useEffect` return | Orphaned if React strict mode re-mounts |
| `IndexedDB entries` | LRU eviction | `ThumbnailGenerationService` | No back-pressure to rendering pipeline |
| `Temp files` | `TempFileManager` cleanup | Java backend | Files orphaned if JVM crashes |
| `React state` | GC | V8 garbage collector | Blobs in `useRef` avoid re-render but never measured |

**FACT:** `FileContext` stores Blobs in `useRef` to avoid React re-renders. This means the garbage collector must track these references independently of React's lifecycle.

### 1.3 CustomPDFDocumentFactory: 3-Tier Memory Model

```
FACT: CustomPDFDocumentFactory implements 3-tier memory:
  Tier 1: In-memory   — Small docs (< threshold), fastest access
  Tier 2: Mixed       — Medium docs, pages on-demand from file
  Tier 3: File-backed — Large docs, minimal memory footprint
```

**FACT:** Heap-aware decisions determine tier selection. Semaphore-bounded concurrency limits simultaneous document loads.

```
┌─────────────────────────────────────────────────────┐
│              CustomPDFDocumentFactory               │
├──────────┬──────────┬───────────────────────────────┤
│  Tier 1  │  Tier 2  │  Tier 3                       │
│ In-Mem   │  Mixed   │  File-Backed                  │
│          │          │                               │
│ Full doc │ Hot pages│ Metadata + requested pages    │
│ in RAM   │ cached   │ only, rest on disk            │
│          │          │                               │
│ <10MB?   │ 10-100MB │ >100MB                        │
└──────────┴──────────┴───────────────────────────────┘
```

### 1.4 Thumbnail Cache

```
FACT: ThumbnailGenerationService:
  - 10-document LRU cache
  - 1GB session thumbnail cache
  - Size-based eviction (not count-based)
```

### 1.5 Page Persistence

```
FACT: PageMemoryService persists last-viewed page per document to localStorage.
  - Capped at 200 entries
  - Used for "resume where you left off" feature
```

---

## 2. Proposed C++ Memory Model

### 2.1 Core Principles

| Principle | Description |
|-----------|-------------|
| **RAII everywhere** | Every PDFium handle is wrapped in a RAII type. No raw `FPDF_*` handles in application code. |
| **Deterministic destruction** | Memory is released the instant ownership ends. No GC pauses. |
| **Explicit ownership** | Transfer semantics are compile-time enforced. |
| **Bounded memory** | Every subsystem has a hard memory budget. Exceeding it triggers eviction, not allocation failure. |
| **No hidden allocations** | All heap allocations go through tracked allocators in debug builds. |

### 2.2 RAII Wrapper Hierarchy

```
RECOMMENDATION: Build a thin RAII layer over PDFium C API before any other work.

class PdfDocument {
    FPDF_DOCUMENT doc_;                    // owned handle
public:
    PdfDocument();                          // creates empty doc
    explicit PdfDocument(FPDF_DOCUMENT);    // takes ownership
    ~PdfDocument();                         // calls FPDF_CloseDocument
    PdfDocument(const PdfDocument&) = delete;
    PdfDocument& operator=(const PdfDocument&) = delete;
    PdfDocument(PdfDocument&&) noexcept;
    PdfDocument& operator=(PdfDocument&&) noexcept;

    // Page access returns scoped handles
    PdfPage GetPage(int index);
    int PageCount() const;
    // ...
};

class PdfPage {
    FPDF_PAGE page_;
    int index_;
public:
    PdfPage();                             // null page
    explicit PdfPage(FPDF_PAGE, int index);
    ~PdfPage();                            // calls FPDF_ClosePage
    PdfPage(const PdfPage&) = delete;
    PdfPage(PdfPage&&) noexcept;
    // ...
};

class PdfTextPage {
    FPDF_TEXTPAGE text_page_;
public:
    PdfTextPage(FPDF_TEXTPAGE);            // takes ownership
    ~PdfTextPage();                        // calls FPDFText_ClosePage
    // ...
};

class PdfBitmap {
    FPDF_BITMAP bitmap_;
public:
    PdfBitmap(FPDF_BITMAP);               // takes ownership
    ~PdfBitmap();                          // calls FPDFBitmap_Destroy
    // ...
};
```

### 2.3 Ownership Model

```
RECOMMENDATION: Ownership table defines who owns what and transfer rules.

┌────────────────────┬──────────────────┬───────────────────┬──────────────┐
│ Object             │ Primary Owner    │ Access Pattern    │ Transfer     │
├────────────────────┼──────────────────┼───────────────────┼──────────────┤
│ FPDF_DOCUMENT      │ PdfDocument      │ unique (exclusive)│ std::move    │
│ FPDF_PAGE          │ PdfPage          │ unique (scoped)   │ stack return │
│ FPDF_TEXTPAGE      │ PdfTextPage      │ unique (scoped)   │ stack return │
│ FPDF_BITMAP        │ PdfBitmap        │ unique (scoped)   │ stack return │
│ FPDF_ANNOTATION    │ PdfAnnotation    │ unique (scoped)   │ stack return │
│ Rendered tile      │ TileCache        │ shared (const)    │ clone bitmap │
│ Extracted text     │ TextCache        │ shared (const)    │ std::shared  │
│ Document metadata  │ PdfDocument      │ via accessors     │ copy strings │
│ Thumbnail bitmap   │ ThumbnailCache   │ shared (const)    │ LRU entry    │
└────────────────────┴──────────────────┴───────────────────┴──────────────┘
```

### 2.4 Smart Pointer Strategy

```
RECOMMENDATION: Use C++ standard smart pointers with clear semantics:

  std::unique_ptr<PdfDocument>    — Exclusive ownership of a document
  std::shared_ptr<PdfDocument>    — Shared read-only access (ref-counted)
  std::weak_ptr<PdfDocument>      — Non-owning observer (e.g., UI holding ref to background doc)

  Thread-safe shared_ptr for cross-thread document access (UI thread + worker threads).

  unique_ptr for all page/text/bitmap handles (short-lived, stack-scoped).
```

### 2.5 Memory Budget Breakdown

```
RECOMMENDATION: Default 1GB total process budget, configurable.

┌──────────────────────────────┬────────────┬─────────────────────────────┐
│ Subsystem                    │ Budget     │ Eviction Policy             │
├──────────────────────────────┼────────────┼─────────────────────────────┤
│ Tile Cache (rendered pages)  │ 512 MB     │ LRU, size-based            │
│ Thumbnail Cache              │ 64 MB      │ LRU, count (256 thumbs)    │
│ Text Cache (extracted text)  │ 64 MB      │ LRU, size-based            │
│ Document Metadata Cache      │ 32 MB      │ LRU, count (100 docs)      │
│ PDFium Document Handles      │ 128 MB     │ Close least-recently-used  │
│ Working Buffers (render,etc) │ 128 MB     │ Per-operation, freed after │
│ Application State/UI         │ 72 MB      │ Fixed, grows with windows  │
├──────────────────────────────┼────────────┼─────────────────────────────┤
│ TOTAL                        │ 1,000 MB   │                             │
└──────────────────────────────┴────────────┴─────────────────────────────┘

ASSUMPTION: Budget can be increased to 2GB on 64-bit systems with >8GB RAM.
```

---

## 3. Tile Cache Architecture

### 3.1 Design

```
RECOMMENDATION: Fixed-budget tile cache with LRU eviction and multi-resolution support.

class TileCache {
public:
    struct Key {
        std::string document_id;    // identifies the PDF
        int page_index;
        int zoom_level;              // quantized zoom (e.g., 100, 125, 150...)
        int tile_x, tile_y;         // tile grid coordinates
    };

    struct Entry {
        Key key;
        std::shared_ptr<PdfBitmap> bitmap;
        size_t estimated_size_bytes;
        std::chrono::steady_clock::time_point last_access;
    };

    // Insert a tile; evicts LRU entries if over budget
    void Insert(Key key, std::shared_ptr<PdfBitmap> bitmap);

    // Retrieve a tile; returns nullptr if not cached
    std::shared_ptr<PdfBitmap> Get(const Key& key);

    // Invalidate all tiles for a document (e.g., on edit)
    void InvalidateDocument(const std::string& document_id);

    // Get current memory usage
    size_t CurrentUsage() const;
    size_t Budget() const;

private:
    std::mutex mutex_;
    std::unordered_map<Key, std::list<Entry>::iterator> index_;
    std::list<Entry> lru_list_;
    size_t budget_bytes_;
    size_t current_bytes_;
};
```

### 3.2 Tile Sizing

```
RECOMMENDATION: Tiles are 256×256 pixels at the current zoom level.

  Page at 150% zoom, 8.5×11" at 96 DPI:
    Page size: 1224 × 1584 pixels
    Tiles: 5 × 7 = 35 tiles per page
    Per tile: 256 × 256 × 4 bytes (BGRA) = 256 KB

  Memory for one full page at 150%: 35 × 256 KB = ~9 MB
  Memory for 50 cached pages: ~450 MB (within 512 MB budget)
```

### 3.3 Zoom Level Quantization

```
RECOMMENDATION: Quantize zoom to 12.5% steps to maximize cache hits.

  Allowed zoom levels: 50%, 62.5%, 75%, 87.5%, 100%, 112.5%, 125%,
                        137.5%, 150%, 175%, 200%, 250%, 300%, 400%

  When user zooms to 142%, cache serves tiles at 137.5% and re-renders
  at next quantized level.
```

---

## 4. Large Document Strategy

### 4.1 Tiered Loading (Replacing CustomPDFDocumentFactory)

```
RECOMMENDATION: Replace the Java 3-tier model with native memory-mapped I/O.

┌──────────────────────────────────────────────────────────────────┐
│                    Large Document Handling                        │
├───────────────────┬──────────────────────────────────────────────┤
│  Small (<10 MB)   │ Load entirely into memory (FPDF_LoadDocument) │
│                   │ Fastest access, all operations immediate      │
├───────────────────┼──────────────────────────────────────────────┤
│  Medium (10-100MB)│ Memory-mapped file via CreateFileMapping     │
│                   │ OS manages page-in/page-out                   │
│                   │ PDFium reads directly from mapped view        │
├───────────────────┼──────────────────────────────────────────────┤
│  Large (100MB-1GB)│ Memory-mapped + lazy page rendering         │
│                   │ Only requested pages loaded into tile cache   │
│                   │ Metadata parsed on open, pages rendered on    │
│                   │ demand                                       │
├───────────────────┼──────────────────────────────────────────────┤
│  Very Large (>1GB)│ Memory-mapped + aggressive eviction           │
│                   │ Single-page-ahead prefetch                   │
│                   │ Background thread for rendering              │
│                   │ User notified of "large document mode"       │
└───────────────────┴──────────────────────────────────────────────┘
```

### 4.2 Memory-Mapped File Integration

```cpp
// RECOMMENDATION: Custom file access handler for PDFium

class MappedFileAccessHandler : public FPDF_FILEACCESS {
public:
    MappedFileAccessHandler(const std::filesystem::path& path);

    // FPDF_FILEACCESS callbacks
    static int GetBlock(void* param, unsigned long position,
                        unsigned char* pBuf, unsigned long size);

    // For very large files, we still use streaming reads
    // Memory mapping is used by PDFium internally when available

private:
    HANDLE file_handle_;
    HANDLE mapping_handle_;
    const void* base_address_;
    size_t file_size_;
};
```

### 4.3 100GB Document Support

```
FACT: The current application supports PDFs up to 100GB (from product requirements).

RECOMMENDATION: For 100GB documents:
  - NEVER load into memory entirely
  - Use streaming file access handler (FPDF_FILEACCESS with ReadBlock)
  - Render one page at a time, discard immediately after tiling
  - Disable thumbnail cache for this document (or generate on-demand only)
  - Show "Working..." overlay during page transitions
  - Provide page-range navigation instead of continuous scroll
  - Warn user about performance impact
```

### 4.4 1000+ Page Documents

```
RECOMMENDATION: Optimization strategies for high page count:

  1. Lazy page tree traversal — don't enumerate all pages upfront
  2. Page label caching — parse once, store page labels
  3. Outline/bookmark loading — parse tree structure separately
  4. Thumbnail generation — background thread, progressive loading
  5. Search index — build incrementally, store on disk
  6. Virtual scroll — only render visible pages + N pages ahead/behind
```

---

## 5. Memory Diagnostics

### 5.1 Runtime Metrics

```
RECOMMENDATION: Expose memory metrics through internal diagnostics window.

┌──────────────────────────────┬──────────────────────────────────┐
│ Metric                       │ Collection Method                │
├──────────────────────────────┼──────────────────────────────────┤
│ Process working set          │ GetProcessMemoryInfo()           │
│ Process private bytes        │ GetProcessMemoryInfo()           │
│ PDFium document count         │ Internal counter                 │
│ Tile cache usage/budget       │ TileCache::CurrentUsage()        │
│ Tile cache hit rate           │ Hit/miss counter                 │
│ Open file handle count       │ GetProcessHandleCount()          │
│ Heap allocation count         │ Custom allocator stats (debug)   │
│ Peak memory usage             │ Tracked high-water mark          │
│ Document memory per-doc       │ Per-document tracker             │
│ Pending render queue depth    │ Render queue size                │
│ GC pressure (if any)          │ N/A — no GC in native            │
└──────────────────────────────┴──────────────────────────────────┘
```

### 5.2 Debug-Only Allocation Tracking

```cpp
// RECOMMENDATION: Debug build allocation tracker

#ifdef _DEBUG
class AllocationTracker {
public:
    struct AllocRecord {
        void* address;
        size_t size;
        const char* file;
        int line;
        const char* category;  // "tile", "text", "document", etc.
        std::chrono::steady_clock::time_point timestamp;
    };

    static void TrackAlloc(void* ptr, size_t size,
                           const char* file, int line,
                           const char* category);
    static void TrackFree(void* ptr);

    static void DumpLeaks();              // Call at shutdown
    static void DumpByCategory();         // Group allocations by category
    static size_t TotalByCategory(const char* category);
};

#define TRACKED_NEW(type, cat) \
    (new (AllocationTracker::TrackAlloc, __FILE__, __LINE__, cat) type)
#endif
```

### 5.3 Memory Warning System

```
RECOMMENDATION: Respond to Windows memory pressure notifications:

  1. Register with CreateMemoryResourceNotification()
  2. On LOW_MEMORY: evict 50% of tile cache, cancel pending renders
  3. On HIGH_MEMORY: resume normal operation
  4. Monitor process commit charge against system available

  Thresholds:
    >75% of budget  → Start LRU eviction
    >90% of budget  → Aggressive eviction, cancel background renders
    >95% of budget  → Close idle documents, warn user
```

---

## 6. Comparison: Current vs. Proposed

| Aspect | Current (Tauri/React/Java) | Proposed (C++/Win32/PDFium) |
|--------|---------------------------|------------------------------|
| Memory domains | 4 (WebView, JVM, WASM, JS) | 1 (native process) |
| GC overhead | V8 GC + JVM GC + WASM GC | None (RAII) |
| Baseline memory | ~400-700 MB | ~50-100 MB |
| Peak memory (large doc) | Unbounded, OOM likely | Budget-capped, eviction |
| Deterministic cleanup | No (GC-dependent) | Yes (RAII destructors) |
| Memory leaks | Known issue (blob URLs, docs) | Compile-time ownership |
| Large file support | WASM limited (4GB linear) | Memory-mapped, streaming |
| Diagnostic visibility | Chrome DevTools + JVM tools | Custom metrics + Win32 APIs |
| Crash recovery | JVM restart, state lost | Structured crash dump |

---

## 7. Implementation Priority

| Phase | Tasks | Effort |
|-------|-------|--------|
| **P0** | RAII wrappers for FPDF_DOCUMENT, FPDF_PAGE, FPDF_BITMAP | 2 days |
| **P1** | Tile cache with LRU eviction and memory budget | 3 days |
| **P2** | Memory-mapped file handler for large PDFs | 3 days |
| **P3** | Memory diagnostics dashboard | 2 days |
| **P4** | Debug allocation tracker | 2 days |
| **P5** | Windows memory pressure integration | 1 day |

---

*Next: See [SECURITY.md](SECURITY.md) for threat model and security architecture.*
