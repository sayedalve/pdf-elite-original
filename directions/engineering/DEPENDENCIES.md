# Dependencies — PDF Elite Native (C++/Win32/PDFium)

> **Status:** Proposed | **Applies to:** Native C++ rebuild | **Last updated:** 2025-01

---

## Table of Contents

1. [Current Dependency Inventory](#current-dependency-inventory)
2. [Proposed Native Dependencies](#proposed-native-dependencies)
3. [Size Comparison](#size-comparison)
4. [License Analysis](#license-analysis)
5. [Dependency Mapping: Java/Rust/JS → C++](#dependency-mapping)
6. [Dependencies Removed Entirely](#dependencies-removed-entirely)
7. [External Tool Dependencies](#external-tool-dependencies)

---

## Current Dependency Inventory

### Runtime Stack

| Component | Version | Size (bundled) | Purpose |
|-----------|---------|----------------|---------|
| **JRE** | JDK 25 | ~200MB | Java runtime for Spring Boot app |
| **WebView2** | System/Evergreen | ~100MB (system) | Tauri's rendering engine on Windows |
| **Node.js** | (dev only) | ~60MB (dev) | Vite build toolchain |
| **Rust runtime** | (static) | ~10MB | Tauri binary and plugins |
| **Python** | (dev only) | ~50MB (dev) | FastAPI engine, tooling |

> **FACT:** The current Tauri build bundles `libs/*.jar + runtime/jre/**/*` — a full JRE is embedded in the installer. This alone accounts for ~200MB.

### Java Libraries (Gradle Modules)

| Library | Version | Module | Size | Purpose |
|---------|---------|--------|------|---------|
| Spring Boot Starter | 4.0.6 | core | ~15MB | Web framework, DI, configuration |
| Spring Boot Actuator | 4.0.6 | core | ~2MB | Health checks, metrics |
| Spring Boot Web | 4.0.6 | core | ~5MB | REST API, MVC |
| PDFBox | 3.0.7 | core | ~8MB | PDF manipulation (Java) |
| JPDFium | 1.0.2 (private) | proprietary | ~25MB | PDFium JNI wrapper |
| Jackson 3 | 3.x | core | ~3MB | JSON serialization |
| Lombok | Latest | common | ~2MB | Boilerplate reduction |
| H2 Database | Latest | core | ~2MB | Embedded database |
| PostgreSQL Driver | Latest | core | ~1MB | Production database |
| Bucket4j | Latest | core | ~1MB | Rate limiting |
| Jetty HTTP/2 | (via Boot) | core | ~3MB | Embedded web server |
| Spring Security | 4.0.6 | saas | ~5MB | Authentication/authorization |
| Spring Data JPA | 4.0.6 | core | ~4MB | ORM, repository abstraction |
| Hibernate | (via Boot) | core | ~8MB | JPA implementation |

### Frontend NPM Packages (Key)

| Package | Version | Size (bundled) | Purpose |
|---------|---------|----------------|---------|
| React | 19.x | ~40KB gz | UI framework |
| TypeScript | 5.x | (dev) | Type safety |
| Mantine | 7.x | ~150KB gz | Component library |
| TailwindCSS | 4.x | ~30KB gz | Utility CSS |
| @embedpdf/core | ^2.14.4 | ~80KB gz | Custom PDF embed component |
| @embedpdf/react | ^2.14.4 | ~20KB gz | React wrapper for embedpdf |
| @embedpdf/toolbar | ^2.14.4 | ~15KB gz | PDF toolbar component |
| pdfjs-dist | ^5.4.149 | ~800KB gz | PDF.js rendering engine |
| pdf-lib | Latest | ~200KB gz | Client-side PDF editing |
| react-router-dom | 7.x | ~30KB gz | Client-side routing |
| zustand | Latest | ~5KB gz | State management (Mantine) |
| @tauri-apps/api | v2 | ~10KB gz | Tauri IPC bridge |
| Vite | 7.x | (dev) | Build tool |

### Rust Crates (Tauri)

| Crate | Version | Purpose |
|-------|---------|---------|
| tauri | 2.x | Application framework, window management |
| tauri-plugin-shell | 2.x | Shell command execution |
| tauri-plugin-fs | 2.x | File system access |
| tauri-plugin-deep-link | 2.x | URL scheme handling |
| tauri-plugin-updater | 2.x | Auto-update mechanism |
| reqwest | Latest | HTTP client (Rust-side) |
| tokio | Latest | Async runtime |
| keyring | Latest | Secure credential storage |
| once_cell | Latest | Lazy initialization |
| url | Latest | URL parsing |

### Python Dependencies (Engine)

| Package | Purpose |
|---------|---------|
| FastAPI | REST API for PDF processing engine |
| pydantic-ai | AI-assisted PDF operations |
| uvicorn | ASGI server |

### External Tools (Docker-deployed)

| Tool | Purpose | Docker Stage |
|------|---------|--------------|
| Ghostscript | PostScript/PDF conversion | Stage 3 (pre-built image) |
| OCRmyPDF | OCR layer for scanned PDFs | Stage 3 |
| Tesseract | OCR engine | Stage 3 |
| LibreOffice | Office document conversion | Stage 3 |
| WeasyPrint | HTML-to-PDF rendering | Stage 3 |
| qpdf | PDF linearization/optimization | Stage 3 |
| ImageMagick | Image processing | Stage 3 |
| Calibre | E-book to PDF conversion | Stage 3 |

> **FACT:** The Docker 3-stage build uses a pre-built base image containing LibreOffice, Calibre, and Tesseract to avoid installing them at build time.

---

## Proposed Native Dependencies

### Core Dependencies

| Dependency | Source | Size | Linkage | License | Purpose |
|------------|--------|------|---------|---------|---------|
| **PDFium** | pdfium-binaries / build from source | ~30MB (static lib) | Static | BSD-3-Clause | PDF rendering, text extraction, form handling |
| **Windows SDK** | System-installed | 0MB (external) | N/A | System | Win32, GDI+, Direct2D, Shell, Common Controls |
| **MSVC CRT** | System-installed | 0MB (external) | Dynamic (/MD) | System | C runtime, STL |

### Build-Only Dependencies

| Dependency | Source | Purpose |
|------------|--------|---------|
| CMake 3.28+ | cmake.org | Build system |
| Visual Studio 2022 | visualstudio.microsoft.com | Compiler, debugger, SDK |
| Ninja | ninja-build.github.io | Fast build runner (optional) |
| Google Test | FetchContent | Unit testing |
| clang-format | LLVM | Code formatting |
| clang-tidy | LLVM | Static analysis |

### Optional Dependencies (vcpkg)

| Dependency | Size | Purpose | When Needed |
|------------|------|---------|-------------|
| wil | ~1MB (headers) | Win32 RAII wrappers | Always recommended |
| libxml2 | ~2MB | XFA form XML parsing | If XFA support required |
| harfbuzz | ~3MB | Advanced text shaping | If complex script support needed |
| libcurl | ~1.5MB | HTTP client for updates | If custom update server |
| zlib | ~300KB | Compression utilities | Usually bundled with PDFium |
| minizip | ~200KB | ZIP handling | For PDF package/Portfolio support |

---

## Size Comparison

### Bundled Size (Installed Application)

| Component | Current (Tauri) | Proposed (Native) | Savings |
|-----------|----------------|-------------------|---------|
| Runtime | ~200MB (JRE) | 0MB (system CRT) | 200MB |
| Application code | ~100MB (JARs + Tauri) | ~30MB (EXE + PDFium) | 70MB |
| Frontend assets | ~50MB (JS/CSS/WASM) | 0MB (native UI) | 50MB |
| External tools | ~500MB+ (Docker) | 0MB (native calls) | 500MB+ |
| Resources | ~5MB | ~5MB | 0MB |
| **Total installed** | **~400-600MB** | **~35MB** | **~365-565MB** |

### Installer Size

| Target | Current | Proposed |
|--------|---------|----------|
| NSIS installer | 300-500MB | <80MB |
| Portable ZIP | 400-600MB | <40MB |

### Dependency Count

| Category | Current | Proposed | Reduction |
|----------|---------|----------|------------|
| Language runtimes | 3 (Java, Node, Rust) | 0 | -3 |
| Build tools | 4+ (Gradle, npm, cargo, NSIS) | 2 (CMake, MSVC) | -2 |
| Runtime libraries | 50+ Java JARs | 1 (PDFium) | -49 |
| NPM packages | 150+ | 0 | -150 |
| Rust crates | 30+ | 0 | -30 |
| External tools | 8 | 0 (optional native) | -8 |
| **Total managed deps** | **~240+** | **~5** | **~98%** |

---

## License Analysis

### Current Application License Stack

| Layer | Libraries | Primary Licenses |
|-------|----------|-----------------|
| Java | Spring Boot, PDFBox, Jackson, H2, Hibernate | Apache-2.0 (Spring, PDFBox, Jackson), EPL-2.0 (H2), LGPL-2.1 (Hibernate) |
| Frontend | React, Mantine, PDF.js, pdf-lib | MIT (React, Mantine, pdf-lib), Apache-2.0 (PDF.js) |
| Rust | Tauri, reqwest, tokio | Apache-2.0 / MIT (all) |
| Python | FastAPI, pydantic-ai | MIT (all) |
| External | Ghostscript, Tesseract, LibreOffice, Calibre | GPL (Ghostscript), Apache-2.0 (Tesseract), MPL-2.0 (LibreOffice), GPL-3.0 (Calibre) |
| PDFium (via JPDFium) | Private Maven artifact | BSD-3-Clause |

### Proposed Native Application License Stack

| Dependency | License | Implication |
|------------|---------|-------------|
| PDFium | BSD-3-Clause | Permissive, no copyleft. Static linking OK. |
| Windows SDK | System | Proprietary, but free for Windows development. |
| MSVC CRT | System | Proprietary, distributable via VCRedist. |
| Google Test | BSD-3-Clause | Test-only, not shipped. |
| wil (optional) | MIT | Header-only, permissive. |
| Application code | Apache-2.0 | Maintains current license. |

> **FACT:** The current app is Apache 2.0 licensed. The native rebuild maintains this license while dramatically simplifying the dependency license matrix.

> **RECOMMENDATION:** PDFium's BSD-3-Clause license is compatible with Apache 2.0. Static linking is explicitly permitted. No license notice bundling is required, but including a NOTICES.txt file listing PDFium is best practice.

---

## Dependency Mapping

### Java/Rust/JS → C++ Equivalents

| Current (Java/JS/Rust) | Native (C++) | Notes |
|------------------------|---------------|-------|
| PDFBox 3.0.7 | PDFium (direct) | PDFium IS the engine behind PDF.js too. Eliminates Java wrapper. |
| JPDFium 1.0.2 | PDFium (direct) | Removes JNI layer overhead. |
| pdfjs-dist ^5.4.149 | PDFium rendering | Same engine, native speed, no WASM compilation. |
| pdf-lib | PDFium editing | Native PDF page manipulation. |
| @embedpdf/* | Custom C++ ViewerControl | Native Win32 control, no JS bridge. |
| React 19 + Mantine 7 | Win32 UI + Direct2D | Native controls, no DOM overhead. |
| Spring Boot 4.0.6 | Not needed | No server required for desktop app. |
| Jackson 3 | nlohmann/json or WinRT JSON | Only needed for settings/config. |
| H2 / PostgreSQL | SQLite or Windows Registry | Local settings storage. |
| Bucket4j | Not needed | No rate limiting for desktop app. |
| Jetty HTTP/2 | WinHTTP or WinINet | Only for auto-update HTTP calls. |
| tauri-plugin-fs | Win32 CreateFile API | Direct system calls. |
| tauri-plugin-shell | Win32 CreateProcess | Direct system calls. |
| tauri-plugin-deep-link | Win32 RegisterProtocol | Direct registry calls. |
| tauri-plugin-updater | Custom WinHTTP updater | GitHub Releases API. |
| reqwest (Rust) | WinHTTP | HTTP client for updates. |
| tokio (Rust) | Windows Thread Pool | Native work items. |
| keyring (Rust) | Windows Credential Store | `CredWrite`/`CredRead` API. |
| zustand | Application state object | Simple C++ observer pattern. |
| react-router-dom | Tab/page state machine | Native window/tab management. |

### Functional Coverage

| Feature | Current Stack | Native Stack | Status |
|---------|--------------|--------------|--------|
| PDF rendering | PDF.js (WASM) + JPDFium | PDFium Direct2D | ✅ Improved |
| PDF text extraction | PDFBox + JPDFium | PDFium FPDFText | ✅ Equivalent |
| PDF form filling | PDFBox + JPDFium | PDFium FPDFAnnot | ✅ Equivalent |
| PDF annotation | @embedpdf + pdf-lib | PDFium FPDFAnnot | ✅ Equivalent |
| Page thumbnails | pdfjs-dist | PDFium + TileCache | ✅ Equivalent |
| Text search | PDFBox + pdfjs-dist | PDFium FPDFText | ✅ Equivalent |
| File open/save | Tauri FS plugin | Win32 Open/Save dialogs | ✅ Equivalent |
| Print | Browser print dialog | Win32 PrintDlgEx | ✅ Equivalent |
| OCR | Tesseract (Docker) | Tesseract C API (optional) | ⚠️ Optional |
| Office conversion | LibreOffice (Docker) | COM automation (optional) | ⚠️ Optional |
| E-book conversion | Calibre (Docker) | Not supported natively | ❌ Removed |
| Ghostscript | Docker | Not needed (PDFium handles PS) | ❌ Removed |
| Web UI | React + Mantine | Native Win32 UI | ✅ Replaced |
| REST API | Spring Boot | Not needed (desktop app) | ❌ Removed |
| Authentication | Spring Security | Not needed (desktop app) | ❌ Removed |
| Database | H2/PostgreSQL | SQLite / Registry | ✅ Replaced |
| Auto-update | tauri-plugin-updater | WinHTTP + GitHub API | ✅ Equivalent |
| Deep linking | tauri-plugin-deep-link | Win32 protocol handler | ✅ Equivalent |
| Credential storage | keyring (Rust) | Windows Credential Store | ✅ Equivalent |

---

## Dependencies Removed Entirely

### Eliminated Runtimes

| Runtime | Reason | Impact |
|---------|--------|--------|
| **JRE (JDK 25)** | No Java code in native app | -200MB, -1 JVM startup, -1 GC overhead |
| **Node.js** | No JavaScript build step | -60MB dev, -Vite complexity |
| **Rust toolchain** | No Tauri framework | -10MB binary, -cargo build time |
| **Python** | No FastAPI engine | -50MB dev, -Docker dependency |
| **WebView2** | Native Win32 UI replaces web view | -100MB system dependency |

### Eliminated Frameworks

| Framework | Reason |
|-----------|--------|
| **Spring Boot 4.0.6** | Desktop app has no web server |
| **Hibernate/JPA** | No database ORM needed for local storage |
| **Spring Security** | No authentication for desktop app |
| **Bucket4j** | No rate limiting for single-user app |
| **Jetty** | No embedded web server |
| **React 19** | Native UI replaces web UI |
| **Mantine 7** | Native Win32 controls replace components |
| **TailwindCSS 4** | No CSS needed for native UI |
| **Vite 7** | No JavaScript bundler needed |
| **Tauri 2.x** | Direct Win32 API replaces framework |

### Eliminated External Tools

| Tool | Reason | Alternative |
|------|--------|-------------|
| **Ghostscript** | PDFium handles PS-in-PDF natively | Not needed |
| **WeasyPrint** | No HTML-to-PDF server component | Not needed |
| **qpdf** | PDFium handles linearization | Not needed |
| **ImageMagick** | Direct2D/GDI+ for image ops | Not needed |
| **Calibre** | E-book conversion out of scope | Feature removed |
| **OCRmyPDF** | Optional: call Tesseract C API directly | Optional integration |
| **LibreOffice** | Optional: COM automation for .docx conversion | Optional integration |

> **RECOMMENDATION:** For OCR and Office conversion, consider optional plugin architecture where these tools are detected at runtime but not bundled. The core app works without them.

---

## External Tool Dependencies

### Optional: Tesseract OCR

| Aspect | Details |
|--------|---------|
| **Integration** | Tesseract C API (`tesseract.h`) |
| **Linkage** | Dynamic (load `tesseract.dll` at runtime) |
| **Detection** | Check for `TESSDATA_PREFIX` env var or known install paths |
| **Distribution** | Not bundled; user installs separately |
| **Fallback** | Graceful degradation — OCR features hidden if not found |

### Optional: LibreOffice COM

| Aspect | Details |
|--------|---------|
| **Integration** | COM automation (`com.sun.star.ServiceManager`) |
| **Detection** | Check for LibreOffice COM registration |
| **Distribution** | Not bundled; user installs separately |
| **Fallback** | Show "Install LibreOffice" prompt for .docx-to-PDF |

> **ASSUMPTION:** Most users do not need OCR or Office conversion daily. Making these optional keeps the core installer under 80MB while supporting power users who install the external tools.

---

## Summary

The native C++/PDFium rebuild reduces the dependency footprint from **~240+ managed dependencies** to **~5**, eliminates **4 language runtimes**, and removes the need for Docker-based external tools entirely. The total installed size drops from **~400-600MB to ~35MB** — a **90%+ reduction**. The license matrix simplifies from a complex mix of Apache, MIT, GPL, LGPL, EPL, MPL to essentially **Apache 2.0 (app) + BSD-3-Clause (PDFium)**.