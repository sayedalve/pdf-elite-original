# PDF Elite Size Audit

**Date:** 2026-08-17
**Platform:** Windows 10 x64
**Architecture:** Native C++ (Win32 API)
**Compiler:** MSVC (Visual Studio 2022)
**Configuration:** Release

## Packaging Sizes

| Component | Exact Size (Bytes) | Size (MB) | Notes |
|-----------|--------------------|-----------|-------|
| `PDFElite.exe` | 130,560 | 0.13 MB | Core Native Executable. Statically links all UI, core, and engine modules. |
| `pdfium.dll` | 7,260,672 | 6.92 MB | Chromium PDFium Renderer (pinned at build 7999). |
| `resources/` | 27 | 0.00 MB | Runtime configuration files (`theme.json`). |
| **Total Uncompressed** | **7,391,259** | **7.05 MB** | Total disk space required to run. |

## Installer / Portable Sizes

| Package | Size (Bytes) | Size (MB) | Notes |
|---------|--------------|-----------|-------|
| Portable ZIP (`PDFEliteSetup.zip`) | 3,677,582 | 3.51 MB | Deflate compression. Includes exe, dll, and resources. |
| NSIS Installer (`PDFEliteSetup.exe`) | ~3.5 MB | ~3.5 MB | LZMA compression adds similar ratio (NSIS not available in local dev env, but generated correctly in CI). |

## Analysis against Constraint

The user imposed absolute constraint for the Release package was:
**Absolute Maximum:** 120 MB
**Preferred Size:** 80 to 100 MB

**Actual Result:** ~3.5 MB (Compressed), ~7.0 MB (Uncompressed).
**Status:** **PASS** (97% under the maximum size constraint).

This dramatic reduction is the direct result of stripping out Node, Java, JRE, Tauri, WebViews, and Docker dependencies, utilizing raw Win32 APIs and a raw Chromium PDFium DLL.
