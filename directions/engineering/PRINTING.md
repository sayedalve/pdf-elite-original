# Printing System

> Engineering doc for rebuilding the PDF Elite printing subsystem as a native C++/Win32 application.

## Current Architecture (Tauri/Web)

### Printing Implementation

| Component | Technology | Platform | Status |
|-----------|-----------|----------|--------|
| PrintPlugin | EmbedPDF (JavaScript) | WebView print | Active |
| print.rs | Rust | macOS only (69 lines) | macOS only |
| Native Win32 printing | — | Windows | **NOT IMPLEMENTED** |

> **FACT**: EmbedPDF's PrintPlugin handles printing via the WebView's built-in print dialog.

> **FACT**: Rust `print.rs` exists but only implements macOS printing (69 lines). No native Windows printing implementation exists.

> **FACT**: On Windows, printing currently goes through the WebView's print dialog — a browser-level print, not a native PDF-aware print.

### Current Limitations

```
Current Windows Printing:
  ├── Uses WebView print dialog (browser-based)
  ├── No control over print quality / DPI
  ├── No page range selection in PDF terms
  ├── No fit-to-page / custom scaling options
  ├── No multi-page-per-sheet support
  ├── No dual-sided printing options
  └── Prints as rendered bitmap in WebView (quality loss possible)
```

---

## Proposed Native Architecture (C++/Win32)

### Printing Strategy

Two approaches for native PDF printing:

| Approach | Method | Quality | Speed | Complexity |
|----------|--------|---------|-------|------------|
| GDI Print | PDFium → Bitmap → Printer DC | Good | Fast for small docs | Medium |
| XPS Print | PDFium → XPS → Windows XPS Print API | Excellent | Consistent | High |

> **RECOMMENDATION**: Implement **XPS Print** as the primary method for best quality. Fall back to **GDI Print** for older printers or when XPS is unavailable.

### Printing Architecture Overview

```
User clicks Print (Ctrl+P)
  └── CPrintManager::ShowPrintDialog()
        │
        ├── User selects printer, settings
        │     └── PRINTDLGEX returns DEVMODE + printer DC
        │
        ├── CPrintJob created
        │     ├── Page range parsing
        │     ├── Scaling calculations
        │     └── Print job started
        │
        └── Per-page rendering loop:
              ├── PDFium renders page to bitmap (specified DPI)
              ├── Apply scaling / fit-to-page
              ├── Send bitmap to printer DC (GDI)
              │     OR
              ├── Convert to XPS page
              │     └── Send via XPS Print API
              └── Advance to next page
```

### Print Manager

```cpp
class CPrintManager {
public:
    struct PrintSettings {
        // Printer
        std::wstring printerName;
        DEVMODEW devMode = {};

        // Page range
        enum Range { AllPages, PageRange, CurrentPage };
        Range pageRange = AllPages;
        int rangeStart = 1;
        int rangeEnd = -1;  // -1 = last page

        // Scaling
        enum ScaleMode { ActualSize, FitToPage, FitWidth, CustomScale };
        ScaleMode scaleMode = FitToPage;
        double customScale = 1.0;

        // Layout
        int pagesPerSheet = 1;       // 1, 2, 4, 6, 9
        enum Duplex { Simplex, DuplexLong, DuplexShort };
        Duplex duplex = Simplex;

        // Quality
        int printDPI = 300;
        bool printInColor = true;
        bool highQuality = true;

        // Orientation
        enum Orientation { Auto, Portrait, Landscape };
        Orientation orientation = Auto;  // Use page's native orientation
    };

    // Show print dialog and get settings
    HRESULT ShowPrintDialog(HWND hWnd, PrintSettings& settings);

    // Print with settings (no dialog, for batch printing)
    HRESULT PrintDocument(CDocument* doc, const PrintSettings& settings);

    // Print to file (PDF)
    HRESULT PrintToPDF(CDocument* doc, const std::wstring& outputPath,
                       const PrintSettings& settings);

private:
    HRESULT PrintViaGDI(CDocument* doc, const PrintSettings& settings);
    HRESULT PrintViaXPS(CDocument* doc, const PrintSettings& settings);
};
```

### Print Dialog (GDI)

```cpp
HRESULT CPrintManager::ShowPrintDialog(HWND hWnd, PrintSettings& settings) {
    // Initialize PRINTDLGEX structure
    PRINTPAGERANGE pageRange = { settings.rangeStart, settings.rangeEnd };

    PRINTDLGEXW pd = { 0 };
    pd.lStructSize = sizeof(PRINTDLGEXW);
    pd.hwndOwner = hWnd;
    pd.Flags = PD_RETURNDC | PD_USEDEVMODECOPIESANDCOLLATE |
               PD_NOPAGENUMS | PD_NOSELECTION;  // Use our own page range UI

    // Page ranges
    pd.nPageRanges = 1;
    pd.lpPageRanges = &pageRange;

    // Callback for custom page range handling
    pd.lpCallback = PrintDialogCallback;
    pd.lParam = reinterpret_cast<LPARAM>(&settings);

    HRESULT hr = PrintDlgExW(&pd);
    if (hr == S_OK) {
        // User clicked Print
        settings.printerName = GetPrinterName(pd.hDC);
        settings.devMode = *reinterpret_cast<DEVMODEW*>(
            GlobalLock(pd.hDevMode));
        settings.printInColor = (settings.devMode.dmColor == DMCOLOR_COLOR);
        return S_OK;
    }

    return (hr == S_FALSE) ? S_FALSE : hr;  // S_FALSE = Cancel
}
```

### Custom Print Dialog

> **RECOMMENDATION**: Build a custom print dialog for better PDF-specific controls (page range, fit mode, quality).

```
┌──────────────────────────────────────────────────────────┐
│ Print                                                        │
├──────────────────────────────────────────────────────────┤
│                                                              │
│  Printer: [Microsoft Print to PDF          ▼]              │
│  Status:  Ready                                              │
│                                                              │
│  ── Page Range ──────────────────────────────               │
│  ○ All pages (47)                                            │
│  ○ Current page (page 5)                                     │
│  ○ Pages: [5] to [12]                                        │
│                                                              │
│  ── Scaling ─────────────────────────────────                │
│  ○ Fit to printable area                                     │
│  ○ Fit to page width                                         │
│  ○ Actual size                                               │
│  ○ Custom scale: [150] %                                     │
│                                                              │
│  ── Layout ──────────────────────────────────                │
│  Orientation: ○ Auto  ○ Portrait  ○ Landscape              │
│  Pages per sheet: [1 ▼]                                     │
│  ☐ Print on both sides (duplex)                              │
│                                                              │
│  ── Quality ─────────────────────────────────               │
│  Quality: ○ Draft  ● Normal  ○ High                         │
│  DPI: [300 ▼]                                                │
│  ☐ Color (uncheck for grayscale)                             │
│                                                              │
│  ── Copies ────────────────────────────────                 │
│  Number of copies: [1]                                      │
│  ☐ Collate                                                    │
│                                                              │
│                        [Printer...]  [Print]  [Cancel]      │
└──────────────────────────────────────────────────────────┘
```

### GDI Printing Implementation

```cpp
HRESULT CPrintManager::PrintViaGDI(CDocument* doc,
                                     const PrintSettings& settings) {
    HDC hDC = CreateDCW(settings.printerName.c_str(), nullptr, nullptr,
                        &settings.devMode);
    if (!hDC) return HRESULT_FROM_WIN32(GetLastError());

    DOCINFOW docInfo = { 0 };
    docInfo.cbSize = sizeof(DOCINFOW);
    docInfo.lpszDocName = L"PDF Elite - Document";

    if (StartDocW(hDC, &docInfo) <= 0) {
        DeleteDC(hDC);
        return E_FAIL;
    }

    int pageCount = doc->GetPageCount();
    int startPage = (settings.pageRange == PrintSettings::CurrentPage)
                    ? doc->GetActivePage() : settings.rangeStart - 1;
    int endPage = (settings.pageRange == PrintSettings::AllPages)
                  ? pageCount - 1 : settings.rangeEnd - 1;

    for (int page = startPage; page <= endPage; page++) {
        // 1. Get page dimensions
        double pageWidth, pageHeight;
        doc->GetPageSize(page, pageWidth, pageHeight);

        // 2. Calculate print dimensions with scaling
        int physicalWidth = GetDeviceCaps(hDC, PHYSICALWIDTH);
        int physicalHeight = GetDeviceCaps(hDC, PHYSICALHEIGHT);
        double scaleX, scaleY, scale;
        CalculateScale(settings, pageWidth, pageHeight,
                       physicalWidth, physicalHeight, scale);

        // 3. Render page to bitmap via PDFium
        int renderDPI = settings.printDPI;
        int bmpWidth = static_cast<int>(pageWidth * renderDPI / 72.0 * scale);
        int bmpHeight = static_cast<int>(pageHeight * renderDPI / 72.0 * scale);

        HBITMAP hBmp = doc->RenderPageToBitmap(page, bmpWidth, bmpHeight);

        // 4. Start page and send bitmap
        StartPage(hDC);

        // Center on paper
        int offsetX = (physicalWidth - bmpWidth) / 2;
        int offsetY = (physicalHeight - bmpHeight) / 2;

        // Draw bitmap to printer DC
        HDC hMemDC = CreateCompatibleDC(hDC);
        SelectObject(hMemDC, hBmp);
        SetStretchBltMode(hDC, HALFTONE);
        StretchBlt(hDC, offsetX, offsetY, bmpWidth, bmpHeight,
                   hMemDC, 0, 0, bmpWidth, bmpHeight, SRCCOPY);
        DeleteDC(hMemDC);
        DeleteObject(hBmp);

        EndPage(hDC);
    }

    EndDocW(hDC);
    DeleteDC(hDC);
    return S_OK;
}
```

### XPS Print Implementation (Preferred)

> **RECOMMENDATION**: Use XPS Print API for vector-quality output. PDF pages are converted to XPS, preserving text and vector graphics at printer resolution.

```
PDFium → Render to bitmap (high DPI) → Convert to XPS page → XPS Print API
  OR (ideal but complex):
PDFium → Direct XPS conversion (requires custom implementation)
```

```cpp
HRESULT CPrintManager::PrintViaXPS(CDocument* doc,
                                    const PrintSettings& settings) {
    // 1. Get XPS printer
    IXpsPrintJob* pJob = nullptr;
    IXpsPrintJobStream* pStream = nullptr;
    HRESULT hr = StartXpsPrintJob(
        settings.printerName.c_str(),
        L"PDF Elite - Document",
        nullptr,          // job name
        nullptr,          // progress event
        0,                // completion event
        nullptr,          // priority
        0,                // max pages
        &pJob,
        &pStream,
        nullptr
    );
    if (FAILED(hr)) return hr;

    // 2. Create XPS OM writer
    IXpsOMObjectFactory* pFactory = nullptr;
    CoCreateInstance(CLSID_XpsOMObjectFactory, nullptr,
                    CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));

    IXpsOMPackageWriter* pWriter = nullptr;
    // ... create package writer targeting the print job stream

    // 3. For each page in range:
    //    a. Render page to high-DPI bitmap via PDFium
    //    b. Create XPS page with image resource
    //    c. Add to package writer

    // 4. Close writer → job sends to printer
    pWriter->Close();
    pJob->Close();
    pStream->Close();

    // Cleanup
    pJob->Release();
    pStream->Release();
    pFactory->Release();
    pWriter->Release();

    return S_OK;
}
```

### Scaling Calculations

| Scale Mode | Calculation | Example (A4 page on Letter printer) |
|-----------|-------------|--------------------------------------|
| Actual Size | 100% — may crop | 210mm → 215.9mm (minor crop) |
| Fit to Page | min(scaleX, scaleY) | Scale to fit within letter bounds |
| Fit to Width | scaleX only | Fill width, may extend vertically |
| Custom | user-specified % | 150% — may span multiple sheets |

```cpp
void CalculateScale(const PrintSettings& settings,
                    double pageWidthPt, double pageHeightPt,
                    int printerWidthPx, int printerHeightPx,
                    double& outScale) {
    double printerDPI = GetDeviceCaps(hDC, LOGPIXELSX);
    double printerWidthPt = printerWidthPx * 72.0 / printerDPI;
    double printerHeightPt = printerHeightPx * 72.0 / printerDPI;

    switch (settings.scaleMode) {
        case PrintSettings::ActualSize:
            outScale = 1.0;
            break;
        case PrintSettings::FitToPage:
            outScale = std::min(printerWidthPt / pageWidthPt,
                                printerHeightPt / pageHeightPt);
            break;
        case PrintSettings::FitWidth:
            outScale = printerWidthPt / pageWidthPt;
            break;
        case PrintSettings::CustomScale:
            outScale = settings.customScale / 100.0;
            break;
    }
}
```

### Print Preview (Stretch Goal)

> **RECOMMENDATION**: Implement print preview in a future phase. Use an off-screen rendering approach: render all pages in the print configuration and display in a scrollable preview window.

```
Print Preview Window:
  ┌──────────────────────────────────────┐
  │ Print Preview              [Print]   │
  ├──────────────────────────────────────┤
  │  ┌──────────────┐                    │
  │  │              │  Page 1 of 5       │
  │  │  (rendered   │  Fit to Page      │
  │  │   preview)   │  300 DPI          │
  │  │              │                    │
  │  │              │                    │
  │  └──────────────┘                    │
  │  [◄] [1/5] [►]  Zoom: [Fit ▼]      │
  └──────────────────────────────────────┘
```

### Batch Printing via Command Line

```cpp
// Command-line printing (from FILE_HANDLING.md)
// PDF Elite.exe /print "C:\docs\report.pdf" /printer "HP LaserJet"
// PDF Elite.exe /print "C:\docs\*.pdf" /range "1-5" /quiet
```

### Print-to-PDF

> **RECOMMENDATION**: Support "Print to PDF" by redirecting output to "Microsoft Print to PDF" virtual printer, or by directly generating a new PDF via PDFium.

```cpp
HRESULT CPrintManager::PrintToPDF(CDocument* doc,
                                   const std::wstring& outputPath,
                                   const PrintSettings& settings) {
    // Option 1: Use Microsoft Print to PDF
    // Settings.printerName = "Microsoft Print to PDF";
    // Need to set output path via DEVMODE dmDeviceName trick

    // Option 2: Direct PDF generation (preferred)
    // Use PDFium to save the document (preserves vector quality)
    FPDF_DOCUMENT clonedDoc = FPDF_CreateNewDocument();
    // Copy pages with modifications...
    FPDF_SaveAsCopy(clonedDoc, outputPath, 0);

    return S_OK;
}
```

---

## Implementation Checklist

- [ ] Implement `CPrintManager` class with settings struct
- [ ] Implement custom print dialog with PDF-specific options
- [ ] Implement GDI Print path (bitmap → printer DC)
- [ ] Implement XPS Print path (bitmap → XPS → printer)
- [ ] Implement page range selection
- [ ] Implement scaling modes (fit to page, actual size, custom)
- [ ] Implement orientation control (auto, portrait, landscape)
- [ ] Implement multi-page-per-sheet layout (2, 4, 6, 9)
- [ ] Implement duplex printing support
- [ ] Implement command-line printing (`/print` flag)
- [ ] Implement print-to-PDF (direct PDFium save)
- [ ] Add print progress dialog
- [ ] Stretch: Implement print preview window
