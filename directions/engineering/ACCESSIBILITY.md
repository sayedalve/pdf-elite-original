# Accessibility

> Engineering doc for PDF Elite native C++/Win32/PDFium rebuild
> Target: File 28 of 31 in engineering documentation set

---

## 1. Current State

**FACT: The current web application has NO accessibility support.**

| Aspect | Current State |
|--------|---------------|
| ARIA attributes | None — no `role`, `aria-label`, `aria-describedby` on any UI element |
| Keyboard navigation | Custom `HotkeyContext` with configurable bindings only — no Tab/Arrow/Enter navigation |
| Screen reader support | None — document text is rendered on `<canvas>`, invisible to screen readers |
| Focus management | Handled via React refs, no programmatic focus indicators |
| High contrast | Not supported |
| DPI scaling | Handled by browser, not by application code |
| Semantic HTML | Minimal — most UI is React components without semantic markup |

### 1.1 What Exists vs. What's Missing

The app has keyboard **shortcuts** (Ctrl+F for search, Ctrl+Z for undo, etc.) but these are **not accessibility features**. They are power-user conveniences implemented via a custom `HotkeyContext` system with capture-phase listeners and pause/resume for modals.

True accessibility requires:
- Standard Windows keyboard navigation (Tab, Arrow, Enter, Escape)
- Screen reader announcements for all UI state changes
- Accessible names and descriptions for all controls
- Focus indicators visible to all users
- Support for Windows accessibility APIs (UI Automation, MSAA)

---

## 2. Windows Accessibility Frameworks

### 2.1 Framework Comparison

| Framework | API | Complexity | Coverage |
|-----------|-----|------------|----------|
| MSAA | `IAccessible` | Low | Legacy, screen readers still support it |
| UI Automation | `IUIAutomationProvider` | Medium | Modern, preferred by Narrator/NVDA/JAWS |
| Win32 Controls v6 | Built-in | Zero | Common controls are automatically accessible |

**RECOMMENDATION:** Use Win32 common controls v6 for all standard UI elements (buttons, menus, toolbars, list views, tree views, edit controls, combo boxes). These automatically implement both MSAA and UI Automation providers. Only implement custom `IUIAutomationProvider` for the PDF canvas and other non-standard elements.

---

## 3. Proposed Accessibility Architecture

### 3.1 UI Element Accessibility Map

| UI Element | Win32 Control | Accessibility Source | Custom Work Needed |
|-------------|---------------|---------------------|-------------------|
| Menu bar | Menu bar (v6) | Built-in MSAA + UIA | None |
| Toolbar | Toolbar (v6) | Built-in | Add tooltips as accessible names |
| Sidebar (pages) | ListView (v6) | Built-in | None |
| Property panel | Dialog with controls (v6) | Built-in | None |
| Status bar | StatusBar (v6) | Built-in | None |
| Tab controls | TabCtrl (v6) | Built-in | None |
| Find dialog | Edit + Button (v6) | Built-in | None |
| **PDF canvas** | Custom `HWND` | **Custom UIA provider** | **Significant** |
| **Annotation overlay** | Custom `HWND` | **Custom UIA provider** | **Moderate** |

### 3.2 PDF Canvas Accessibility

The PDF canvas is the most complex accessibility challenge. The document is rendered as a bitmap, so text content is not natively accessible.

**Solution: Implement `IRawElementProviderFragment` for the canvas**

```
PDF Canvas (HWND)
├── Page 1 (IRawElementProviderFragment)
│   ├── Paragraph 1 (IRawElementProviderFragment)
│   │   ├── Text run 1
│   │   └── Text run 2
│   ├── Annotation: Highlight (IRawElementProviderFragment)
│   └── Form field: Text input (IRawElementProviderFragment)
├── Page 2 (IRawElementProviderFragment)
│   └── ...
└── ...
```

Each fragment implements:

| Property | Source |
|-----------|--------|
| `ControlType` | `UIA_DocumentControlTypeId` for pages, `UIA_TextControlTypeId` for text runs |
| `Name` | Page label or text content |
| `Value` | Text content for text runs |
| `BoundingRectangle` | Computed from PDFium text bounds |
| `Children` | Child fragments (paragraphs, annotations, form fields) |

### 3.3 Text Extraction for Screen Readers

Leverage PDFium's text extraction APIs to populate the UI Automation tree:

```cpp
class PdfCanvasProvider : public IRawElementProviderFragment {
    // Build text tree from PDFium
    void BuildAccessibilityTree();

    // IRawElementProviderFragment
    HRESULT GetPropertyValue(PROPERTYID propId, VARIANT* pRetVal) override;
    HRESULT GetPatternProvider(PATTERNID patternId, IUnknown** ppRetVal) override;
    HRESULT Navigate(NavigateDirection direction, IRawElementProviderFragment** ppRetVal) override;

    // ITextProvider implementation for screen reader text access
    CComPtr<ITextProvider> text_provider_;
};
```

---

## 4. Keyboard Navigation

### 4.1 Standard Windows Navigation

**RECOMMENDATION:** Implement full standard Windows keyboard navigation for all UI elements, in addition to the existing custom shortcuts.

| Key | Action | Scope |
|-----|--------|-------|
| `Tab` / `Shift+Tab` | Move focus between controls | Application-wide |
| `Arrow keys` | Navigate within controls (lists, menus, tabs) | Per-control |
| `Enter` / `Space` | Activate focused control | Per-control |
| `Escape` | Close dialog / cancel operation | Per-context |
| `F6` | Cycle between panes (canvas, sidebar, toolbar) | Application-wide |
| `Ctrl+Tab` | Cycle between open documents | Application-wide |

### 4.2 PDF Canvas Keyboard Navigation

| Key | Action |
|-----|--------|
| `Arrow keys` | Scroll by small increment |
| `Page Up/Down` | Scroll by page height |
| `Home/End` | Jump to first/last page |
| `Ctrl+G` | Go to page dialog |
| `Ctrl+=` / `Ctrl+-` | Zoom in/out |
| `Ctrl+0` | Fit to page |
| `Ctrl+1` | 100% zoom |

### 4.3 Focus Indicators

All interactive elements must show a visible focus indicator:

- **Win32 standard:** Focus rectangles are drawn automatically by common controls v6
- **Custom canvas:** Draw a distinct focus rectangle when the canvas has focus
- **High contrast:** Respect `SystemParametersInfo(SPI_GETHIGHCONTRAST)` and use `COLOR_WINDOWTEXT` / `COLOR_HIGHLIGHT`

---

## 5. Screen Reader Support

### 5.1 Announcements

| Event | Announcement Method |
|-------|---------------------|
| Document opened | `UiaRaiseAutomationEvent` with `UIA_AutomationFocusChangedEventId` |
| Page changed | `LiveRegionChanged` event on page count display |
| Annotation created | `UIA_Text_TextChangedEventId` on canvas |
| Search result found | `UIA_Text_TextChangedEventId` + navigate to result |
| Error occurred | `UIA_AutomationPropertyChangedEventId` on status bar |
| Operation complete | `UIA_Text_TextChangedEventId` on status bar |

### 5.2 Narrator Compatibility

Windows Narrator is the built-in screen reader. Key compatibility points:

- All custom controls must implement `IRawElementProviderSimple` at minimum
- Text content must support `ITextProvider` pattern
- Selection must support `ISelectionProvider` pattern
- Document content must support `ITextProvider2` for rich text

---

## 6. High Contrast Mode

**FACT:** Win32 common controls v6 automatically adapt to high contrast mode.** Custom rendering (PDF canvas, tool overlays) must explicitly check and adapt.

```cpp
BOOL is_high_contrast = FALSE;
HIGHCONTRAST hc = { sizeof(hc) };
SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0);
is_high_contrast = (hc.dwFlags & HCF_HIGHCONTRASTON) != 0;

// Use system colors when in high contrast
COLORREF text_color = is_high_contrast ? GetSysColor(COLOR_WINDOWTEXT) : RGB(0, 0, 0);
COLORREF bg_color   = is_high_contrast ? GetSysColor(COLOR_WINDOW) : RGB(255, 255, 255);
```

### 6.1 UI Element Adaptation

| Element | Standard Mode | High Contrast Mode |
|---------|---------------|-------------------|
| Canvas background | White or user preference | `COLOR_WINDOW` |
| Selection highlight | Custom blue highlight | `COLOR_HIGHLIGHT` |
| Annotation colors | Preset colors | System colors |
| Toolbar icons | Custom colored icons | Monochrome + system colors |
| Focus rectangles | Custom style | `COLOR_WINDOWTEXT` / `COLOR_HIGHLIGHT` |

---

## 7. DPI Scaling

**FACT:** Win32 common controls v6 handle DPI scaling automatically when the application declares DPI awareness.**

```cpp
// In manifest or code:
SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
```

Custom rendering must scale:

| Element | Scaling Approach |
|---------|-----------------|
| PDF canvas | Scale factor applied to render DPI |
| Tool overlays | Scale coordinates and stroke widths |
| Custom drawn controls | Use `GetDpiForWindow()` for calculations |
| Icons | Provide multiple sizes (16x16, 24x24, 32x32, 48x48) |

---

## 8. Implementation Priority

| Priority | Feature | Effort | Impact |
|----------|---------|--------|--------|
| P0 | Common controls v6 for all standard UI | Low (use correct control classes) | High |
| P0 | Standard keyboard navigation (Tab, Enter, Escape) | Medium | Critical |
| P0 | Focus indicators | Low | Critical |
| P1 | Accessible names for toolbar buttons | Low | High |
| P1 | UI Automation provider for PDF canvas | High | High |
| P1 | Text extraction for screen readers | Medium (PDFium provides this) | High |
| P2 | High contrast mode support | Medium | Medium |
| P2 | Narrator event announcements | Medium | Medium |
| P3 | Form field accessibility | Medium | Medium |
| P3 | Annotation accessibility | Medium | Low |

---

## 9. Testing

| Test Type | Tool | Frequency |
|-----------|------|-----------|
| Keyboard navigation | Manual | Every build |
| Inspect.exe / Accessibility Insights | Automated scan | Every build |
| Narrator testing | Manual | Per feature |
| NVDA testing | Manual | Per release |
| UI Automation verification | C++ unit tests | Every build |
| High contrast visual check | Manual | Per release |

---

*Document 28 of 31 — Accessibility*