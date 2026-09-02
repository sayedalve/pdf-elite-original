# Clipboard Operations

> Engineering doc for rebuilding the PDF Elite clipboard subsystem as a native C++/Win32 application.

## Current Architecture (Tauri/Web)

### Clipboard Implementation

| Operation | Technology | Implementation |
|-----------|-----------|----------------|
| Text copy | Browser API | `navigator.clipboard.writeText(text)` |
| Copy trigger | React component | `TextSelectionMenu` with copy option |
| Image copy | — | Not implemented |
| PDF object copy | — | Not implemented |
| Paste | — | Not implemented |

> **FACT**: TextSelectionMenu provides a copy option that uses `navigator.clipboard.writeText` (browser API).

> **FACT**: In Tauri, clipboard access works via browser APIs or `tauri-plugin-clipboard`.

> **FACT**: No image copy, PDF object copy, or paste operations exist in the current application.

### Current Copy Flow

```
User selects text on PDF page
  └── TextSelectionMenu appears (context menu)
        └── User clicks "Copy"
              └── navigator.clipboard.writeText(selectedText)
                    └── OS clipboard updated with plain text
```

---

## Proposed Native Architecture (C++/Win32)

### Clipboard Architecture Overview

```
Clipboard Operations
├── Text Copy        → CF_UNICODETEXT
├── Image Copy       → CF_DIB / CF_PNG
├── PDF Selection    → Custom format "PDF Elite Selection"
├── Paste Text       → Read CF_UNICODETEXT
├── Paste Image      → Read CF_DIB
└── Paste PDF Object → Read custom format (internal)
```

### Win32 Clipboard APIs

> **RECOMMENDATION**: Use Win32 clipboard API directly (`OpenClipboard`, `SetClipboardData`, `CloseClipboard`) rather than OLE data transfer for simplicity and reliability.

```cpp
class CClipboard {
public:
    // Text operations
    static bool CopyText(const std::wstring& text);
    static std::optional<std::wstring> PasteText();

    // Image operations
    static bool CopyBitmap(HBITMAP hBitmap);
    static bool CopyBitmapToDeviceIndependentBitmap(HBITMAP hBitmap);
    static std::optional<HBITMAP> PasteBitmap();

    // Custom format operations
    static bool CopyCustomFormat(UINT formatId, const std::vector<uint8_t>& data);
    static std::optional<std::vector<uint8_t>> PasteCustomFormat(UINT formatId);

    // High-level operations
    static bool CopySelection(const PDFSelection& selection);
    static bool CopyPageAsImage(int pageIndex, const RECT& region);
    static bool CopyAnnotations(const std::vector<AnnotationRef>& annotations);

    // Clipboard monitoring
    static void EnableClipboardMonitoring(HWND hWnd);

    // Cleanup
    static void Empty();

private:
    static UINT RegisterCustomFormat(const std::wstring& name);
};
```

### Text Copy Implementation

```cpp
bool CClipboard::CopyText(const std::wstring& text) {
    if (!OpenClipboard(nullptr)) return false;
    EmptyClipboard();

    size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, bytes);
    if (!hMem) {
        CloseClipboard();
        return false;
    }

    wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
    wcscpy_s(pMem, text.size() + 1, text.c_str());
    GlobalUnlock(hMem);

    HANDLE result = SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();

    if (!result) {
        GlobalFree(hMem);
        return false;
    }
    return true;
}
```

### Image Copy Implementation

```cpp
bool CClipboard::CopyPageAsImage(int pageIndex, const RECT& region) {
    // 1. Render the page region to a bitmap via PDFium
    auto* doc = CDocumentManager::Instance().GetActiveDocument();
    if (!doc) return false;

    HBITMAP hBitmap = doc->RenderPageRegionToBitmap(pageIndex, region);
    if (!hBitmap) return false;

    // 2. Convert to DIB for clipboard
    return CopyBitmapToDeviceIndependentBitmap(hBitmap);
}

bool CClipboard::CopyBitmapToDeviceIndependentBitmap(HBITMAP hBitmap) {
    if (!OpenClipboard(nullptr)) return false;
    EmptyClipboard();

    // Get bitmap info
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    // Create DIB
    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = -bmp.bmHeight;  // Top-down
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    size_t pixelDataSize = bmp.bmWidth * bmp.bmHeight * 4;
    HGLOBAL hDIB = GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + pixelDataSize);
    if (!hDIB) {
        CloseClipboard();
        return false;
    }

    void* pDIB = GlobalLock(hDIB);
    memcpy(pDIB, &bi, sizeof(BITMAPINFOHEADER));
    GetBitmapBits(hBitmap, static_cast<int>(pixelDataSize),
                  static_cast<BYTE*>(pDIB) + sizeof(BITMAPINFOHEADER));
    GlobalUnlock(hDIB);

    HANDLE result = SetClipboardData(CF_DIB, hDIB);
    CloseClipboard();

    DeleteObject(hBitmap);
    return result != nullptr;
}
```

### Custom Clipboard Format for PDF Objects

> **RECOMMENDATION**: Register a custom clipboard format for internal copy/paste of PDF selections (annotations, text blocks, images). This enables "Copy" within PDF Elite and "Paste" to paste the PDF object in another location within the same or different document.

```cpp
// Register once at application startup
UINT g_pdfSelectionFormat = 0;

void InitializeClipboardFormats() {
    g_pdfSelectionFormat = RegisterClipboardFormatW(L"PDF Elite Selection");
}
```

```cpp
struct PDFSelectionData {
    uint32_t magic;         // 0x50445345 ("PDSE")
    uint32_t version;       // 1
    uint32_t type;          // 0=text, 1=image, 2=annotation, 3=mixed
    uint32_t itemCount;
    uint32_t sourcePage;
    // Followed by per-item data (length-prefixed)
    // ...
};

bool CClipboard::CopySelection(const PDFSelection& selection) {
    if (!OpenClipboard(nullptr)) return false;
    EmptyClipboard();

    // Serialize selection data
    std::vector<uint8_t> data = SerializePDFSelection(selection);

    // Copy as custom format
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, data.size());
    void* pMem = GlobalLock(hMem);
    memcpy(pMem, data.data(), data.size());
    GlobalUnlock(hMem);

    SetClipboardData(g_pdfSelectionFormat, hMem);

    // Also copy as text (for pasting into other apps)
    std::wstring text = selection.GetSelectedText();
    if (!text.empty()) {
        CopyText(text);
    }

    // Also copy as image if visual region selected
    if (selection.HasVisualRegion()) {
        HBITMAP hBmp = selection.RenderToBitmap();
        CopyBitmapToDeviceIndependentBitmap(hBmp);
    }

    CloseClipboard();
    return true;
}
```

### Paste Handling

```cpp
// In main window message loop
LRESULT CMainFrame::OnClipboardUpdate(UINT msg, WPARAM wParam, LPARAM lParam) {
    // Check if clipboard has content we can paste
    if (!IsClipboardFormatAvailable(g_pdfSelectionFormat)) {
        // No PDF Elite format - check for plain text/image
        m_pasteAvailable = IsClipboardFormatAvailable(CF_UNICODETEXT) ||
                          IsClipboardFormatAvailable(CF_DIB);
    } else {
        m_pasteAvailable = true;
    }
    UpdatePasteMenuItem();
    return 0;
}

void CMainFrame::OnEditPaste() {
    if (!OpenClipboard(m_hWnd)) return;

    // Priority 1: Custom PDF format
    if (IsClipboardFormatAvailable(g_pdfSelectionFormat)) {
        HANDLE hData = GetClipboardData(g_pdfSelectionFormat);
        if (hData) {
            void* pData = GlobalLock(hData);
            SIZE_T size = GlobalSize(hData);
            HandlePDFSelectionPaste(pData, size);
            GlobalUnlock(hData);
        }
    }
    // Priority 2: Image paste
    else if (IsClipboardFormatAvailable(CF_DIB)) {
        HANDLE hData = GetClipboardData(CF_DIB);
        // Insert as image annotation on current page
    }
    // Priority 3: Text paste
    else if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
            wchar_t* pText = static_cast<wchar_t*>(GlobalLock(hData));
            HandleTextPaste(pText);
            GlobalUnlock(hData);
        }
    }

    CloseClipboard();
}
```

### Clipboard Format Priority Table

| Priority | Format | Action |
|----------|--------|--------|
| 1 | `PDF Elite Selection` (custom) | Paste PDF objects (annotations, selections) |
| 2 | `CF_DIB` | Insert as image on current page |
| 3 | `CF_UNICODETEXT` | Insert as text annotation or inline text edit |
| 4 | `CF_HDROP` | Open referenced file(s) |

### OLE Clipboard (Advanced)

> **ASSUMPTION**: Basic Win32 clipboard API is sufficient for initial release. OLE clipboard (`IDataObject`) should be considered for future enhancements like delayed rendering of large images.

```
OLE Clipboard Benefits:
  ├── Delayed rendering (don't generate data until paste requested)
  ├── Multiple formats in single clipboard set
  ├── COM-based, integrates with shell
  └── Required for drag-drop (OLE drag-drop protocol)

Current Decision: Use basic Win32 API
Future Decision: Migrate to OLE clipboard if performance issues arise
```

### Keyboard Shortcuts

| Shortcut | Action | Context |
|----------|--------|---------|
| `Ctrl+C` | Copy selection (text/image/PDF object) | Text selected or object selected |
| `Ctrl+X` | Cut selection (copy + delete source) | Annotation or image selected |
| `Ctrl+V` | Paste clipboard content | Document has focus |
| `Ctrl+A` | Select all on current page | Document has focus |
| `Ctrl+Shift+C` | Copy page as image | Always available |
| `Esc` | Clear selection | Selection active |

### Context Menu Integration

```
Right-click on PDF page:
  ├── If text selected:
  │     ├── Copy Text
  │     ├── Copy as Image
  │     └── Copy Selection (PDF format)
  ├── If annotation selected:
  │     ├── Cut
  │     ├── Copy
  │     └── Delete
  └── If nothing selected:
        └── Paste
```

---

## Implementation Checklist

- [ ] Implement `CClipboard` class with basic Win32 clipboard API wrappers
- [ ] Implement text copy/paste with `CF_UNICODETEXT`
- [ ] Implement image copy via PDFium render → `CF_DIB`
- [ ] Register custom clipboard format `"PDF Elite Selection"`
- [ ] Implement PDF object serialization for custom clipboard format
- [ ] Implement paste handling with format priority
- [ ] Register `WM_CLIPBOARDUPDATE` for clipboard monitoring
- [ ] Implement keyboard shortcuts (`Ctrl+C/X/V/A`)
- [ ] Implement context menu with copy/paste options
- [ ] Add "Copy Page as Image" feature
- [ ] Test interoperability with other apps (Word, Chrome, etc.)
