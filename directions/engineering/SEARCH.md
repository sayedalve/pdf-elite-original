# Search System

> Engineering doc for rebuilding the PDF Elite search subsystem as a native C++/Win32 application.

## Current Architecture (Tauri/Web)

### Search Implementation

| Component | Technology | Responsibility |
|-----------|-----------|----------------|
| SearchPlugin | EmbedPDF (JavaScript) | Core search engine |
| SearchInterface | React UI component | Search bar UI |
| CustomSearchLayer | Canvas overlay | Highlight rendering |
| ViewerContext.searchActions | TypeScript | API bridge |

### Search Features

| Feature | Implementation | Details |
|---------|---------------|---------|
| Trigger | `Ctrl+F` | Standard keyboard shortcut |
| Debounce | 300ms | On search input |
| Incremental | Yes | Immediate state updates via `registerImmediateSearchUpdate` |
| Navigation | Next/Prev | `SearchInterface` with next/prev/count |
| Highlight | CustomSearchLayer | Canvas overlay on rendered pages |
| Match count | Yes | Shown in search bar |

> **FACT**: SearchPlugin provides search via EmbedPDF with 300ms debounce on input, immediate search state updates via `registerImmediateSearchUpdate` callback.

> **FACT**: `CustomSearchLayer` renders search highlights as a canvas overlay on PDF pages.

> **FACT**: ViewerContext exposes `searchActions`: `search(query)`, `next()`, `previous()`, `clear()`, `goToResult(index)`.

### Current Search Flow

```
User presses Ctrl+F
  └── Search bar appears (SearchInterface)
        │
        ├── User types query
        │     └── 300ms debounce
        │           └── SearchPlugin.search(query)
        │                 ├── registerImmediateSearchUpdate → UI updates count
        │                 └── CustomSearchLayer renders highlights
        │
        ├── User clicks Next / keyboard Enter
        │     └── ViewerContext.searchActions.next()
        │           └── Scroll to next match, highlight active
        │
        └── User presses Escape
              └── ViewerContext.searchActions.clear()
                    └── Remove highlights
```

---

## Proposed Native Architecture (C++/Win32)

### Search Architecture Overview

```
CSearchEngine
├── Text Extraction Layer (PDFium)
│     └── FPDFText_GetText per page
├── Search Thread (background worker)
│     └── Incremental search across pages
├── Index Layer (optional, for large docs)
│     └── Full-text index built on document open
└── Results Layer
      ├── Match list (page, position, context)
      └── Highlight renderer (overlay on page render)
```

### Text Extraction via PDFium

```cpp
class CTextExtractor {
public:
    struct TextBlock {
        int pageIndex;
        double x, y;           // Position on page
        double width, height;
        std::wstring text;
        int charCount;
    };

    struct PageText {
        int pageIndex;
        std::wstring fullText;
        std::vector<TextBlock> blocks;
    };

    // Extract all text from a page
    static PageText ExtractPageText(FPDF_DOCUMENT doc, int pageIndex);

    // Extract text from specific region
    static std::wstring ExtractRegionText(FPDF_DOCUMENT doc,
                                           int pageIndex,
                                           const FPDF_RECTF& rect);
};
```

```cpp
PageText CTextExtractor::ExtractPageText(FPDF_DOCUMENT doc, int pageIndex) {
    PageText result;
    result.pageIndex = pageIndex;

    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
    if (!page) return result;

    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
    if (!textPage) {
        FPDF_ClosePage(page);
        return result;
    }

    int charCount = FPDFText_CountChars(textPage);
    result.fullText.reserve(charCount);

    for (int i = 0; i < charCount; i++) {
        unsigned int codepoint = FPDFText_GetUnicode(textPage, i);
        if (codepoint != 0) {
            // Filter non-printable characters
            if (codepoint >= 32 || codepoint == L'\n' || codepoint == L'\t') {
                result.fullText += static_cast<wchar_t>(codepoint);
            }
        }
    }

    // Extract positioned text blocks for match highlighting
    int blockCount = FPDFText_CountBlocks(textPage);
    for (int b = 0; b < blockCount; b++) {
        TextBlock block;
        block.pageIndex = pageIndex;

        // Get block bounding box
        double left, top, right, bottom;
        FPDFText_GetBlockInfo(textPage, b, &left, &top, &right, &bottom);
        block.x = left;
        block.y = top;
        block.width = right - left;
        block.height = bottom - top;

        // Get block text
        int blockLen = FPDFText_GetBlockText(textPage, b, nullptr, 0);
        if (blockLen > 0) {
            std::vector<ushort> buffer(blockLen);
            FPDFText_GetBlockText(textPage, b, buffer.data(), blockLen * sizeof(ushort));
            // Convert UTF-16LE to wstring
            block.text = UTF16LEToWString(buffer.data(), blockLen);
            block.charCount = blockLen;
        }

        result.blocks.push_back(std::move(block));
    }

    FPDFText_ClosePage(textPage);
    FPDF_ClosePage(page);

    return result;
}
```

### Search Engine

```cpp
class CSearchEngine {
public:
    struct SearchMatch {
        int pageIndex;
        int startCharIndex;     // Position in page text
        int endCharIndex;
        double x, y;            // Approximate position for highlighting
        double width, height;
        std::wstring context;   // Surrounding text snippet
    };

    struct SearchOptions {
        bool caseSensitive = false;
        bool wholeWord = false;
        bool matchDiacritics = false;
        int maxResults = 10000;
    };

    using ProgressCallback = std::function<void(int pagesSearched, int matchesFound)>;
    using CompletionCallback = std::function<void(const std::vector<SearchMatch>&)>;

    // Synchronous search (for small documents or single-page search)
    std::vector<SearchMatch> Search(FPDF_DOCUMENT doc,
                                     const std::wstring& query,
                                     const SearchOptions& options = {});

    // Asynchronous search (background thread)
    void SearchAsync(FPDF_DOCUMENT doc,
                     const std::wstring& query,
                     const SearchOptions& options,
                     ProgressCallback onProgress,
                     CompletionCallback onComplete);

    // Cancel ongoing search
    void CancelSearch();

    // Results navigation
    const std::vector<SearchMatch>& GetResults() const { return m_results; }
    size_t GetActiveMatchIndex() const { return m_activeMatchIndex; }
    void SetActiveMatchIndex(size_t index);
    const SearchMatch& GetActiveMatch() const;
    void GoToNextMatch();
    void GoToPrevMatch();

    // Clear
    void ClearResults();

private:
    std::vector<SearchMatch> m_results;
    size_t m_activeMatchIndex = 0;
    std::atomic<bool> m_cancelled{false};
    std::thread m_searchThread;
};
```

### Background Search Thread

```cpp
void CSearchEngine::SearchAsync(FPDF_DOCUMENT doc,
                                  const std::wstring& query,
                                  const SearchOptions& options,
                                  ProgressCallback onProgress,
                                  CompletionCallback onComplete) {
    m_cancelled = false;

    m_searchThread = std::thread([this, doc, query, options,
                                    onProgress, onComplete]() {
        std::vector<SearchMatch> allMatches;
        int pageCount = FPDF_GetPageCount(doc);

        for (int page = 0; page < pageCount; page++) {
            if (m_cancelled) break;

            PageText pageText = CTextExtractor::ExtractPageText(doc, page);
            SearchInPage(pageText, query, options, allMatches);

            if (onProgress) {
                onProgress(page + 1, static_cast<int>(allMatches.size()));
            }
        }

        if (!m_cancelled) {
            m_results = std::move(allMatches);
            m_activeMatchIndex = m_results.empty() ? 0 : 0;
            if (onComplete) onComplete(m_results);
        }
    });
}
```

### Match Finding Algorithm

```cpp
void SearchInPage(const PageText& pageText,
                  const std::wstring& query,
                  const SearchOptions& options,
                  std::vector<SearchMatch>& results) {
    std::wstring text = pageText.fullText;
    std::wstring searchQuery = query;

    if (!options.caseSensitive) {
        // Case-insensitive comparison
        std::transform(text.begin(), text.end(), text.begin(), ::towlower);
        std::transform(searchQuery.begin(), searchQuery.end(),
                       searchQuery.begin(), ::towlower);
    }

    size_t pos = 0;
    while (pos < text.length()) {
        size_t found = text.find(searchQuery, pos);
        if (found == std::wstring::npos) break;

        // Whole word check
        if (options.wholeWord) {
            bool wordStart = (found == 0 || !iswalnum(text[found - 1]));
            bool wordEnd = (found + searchQuery.length() >= text.length() ||
                           !iswalnum(text[found + searchQuery.length()]));
            if (!wordStart || !wordEnd) {
                pos = found + 1;
                continue;
            }
        }

        SearchMatch match;
        match.pageIndex = pageText.pageIndex;
        match.startCharIndex = static_cast<int>(found);
        match.endCharIndex = static_cast<int>(found + searchQuery.length());

        // Find position from text blocks
        FindMatchPosition(pageText, match);

        // Extract context (50 chars before and after)
        int ctxStart = std::max(0, match.startCharIndex - 50);
        int ctxEnd = std::min(static_cast<int>(pageText.fullText.length()),
                              match.endCharIndex + 50);
        match.context = pageText.fullText.substr(ctxStart, ctxEnd - ctxStart);

        results.push_back(std::move(match));
        pos = found + 1;
    }
}
```

### Highlight Rendering

```
Two approaches for highlight rendering:

Approach 1: Re-render page with highlight overlay (accurate, slower)
  └── Render page → apply highlight rectangles on bitmap → display

Approach 2: Semi-transparent overlay on display (fast, requires position accuracy)
  └── Render page normally → draw highlight rectangles as overlay → composite
```

> **RECOMMENDATION**: Use Approach 2 (overlay) for active search highlighting. It avoids re-rendering the entire page and provides smooth visual feedback.

```cpp
class CSearchHighlightRenderer {
public:
    // Draw highlights on page render context
    void DrawHighlights(HDC hdc,
                        const std::vector<SearchMatch>& matches,
                        int visiblePageIndex,
                        size_t activeMatchIndex,
                        const RENDER_TRANSFORM& transform);

private:
    // Highlight colors
    static constexpr COLORREF ACTIVE_HIGHLIGHT = RGB(255, 200, 0);    // Yellow
    static constexpr COLORREF MATCH_HIGHLIGHT  = RGB(255, 255, 0);    // Light yellow
    static constexpr BYTE HIGHLIGHT_ALPHA       = 80;                  // Semi-transparent
};
```

### Search UI

```
┌─────────────────────────────────────────────────────────┐
│ 🔍 [Search query_____________] 3 of 47  [▲] [▼] [✕]    │
│                                                         │
│ Options: [x] Case Sensitive  [x] Whole Word  [ ] Regex│
└─────────────────────────────────────────────────────────┘
```

```cpp
class CSearchBar : public CWindow {
public:
    void Show();
    void Hide();
    void SetFocus();
    void UpdateResults(size_t total, size_t current);
    void UpdateProgress(int pagesSearched, int matchesFound);
    void SetSearching(bool searching);

private:
    // Controls
    CEdit m_editQuery;
    CStatic m_labelCount;
    CButton m_btnNext;
    CButton m_btnPrev;
    CButton m_btnClose;
    CButton m_btnCaseSensitive;
    CButton m_btnWholeWord;

    // Event handlers
    void OnQueryChanged();
    void OnNextMatch();
    void OnPrevMatch();
    void OnClose();
};
```

### Search Options

| Option | Default | Description |
|--------|---------|-------------|
| Case sensitive | Off | Match exact case |
| Whole word | Off | Match complete words only |
| Match diacritics | Off | Distinguish accented characters |
| Regular expression | Off | Use regex pattern (stretch goal) |
| Max results | 10,000 | Limit search results |

### Performance Optimization: Text Indexing

> **RECOMMENDATION**: For large documents (>100 pages), build a text index on document open to accelerate subsequent searches.

```cpp
class CTextIndex {
public:
    // Build index for entire document (called once on open)
    void BuildIndex(FPDF_DOCUMENT doc);

    // Search using index (fast)
    std::vector<SearchMatch> Search(const std::wstring& query,
                                     const SearchOptions& options);

    // Index statistics
    size_t GetTotalCharacters() const;
    size_t GetIndexedPages() const;

private:
    // Per-page text cache
    std::vector<PageText> m_pageTexts;
    // Optional: inverted index for very large documents
    std::unordered_map<std::wstring, std::vector<PageIndexAndPos>> m_invertedIndex;
};
```

### Search Across Multiple Documents

> **RECOMMENDATION**: Support multi-document search in a "Search All" mode, searching across all open tabs.

```
Search All Documents mode:
  1. Iterate all open documents
  2. Search each in parallel (thread pool)
  3. Aggregate results, sorted by document then page
  4. Grouped display: "Document1.pdf (5 matches)", "Document2.pdf (3 matches)"
```

### Search Results Panel

```
┌─────────────────────────────────────┐
│ Search Results (47 matches)         │
├─────────────────────────────────────┤
│ ▼ report.pdf (32 matches)           │
│   Page 1: "...the quarterly report   │
│     showed a 15% increase in..."    │
│   Page 3: "...report generated on    │
│     January 15, 2025..."             │
│   Page 7: "...according to the report│
│     findings..."                     │
│ ▼ invoice.pdf (15 matches)          │
│   Page 1: "...invoice number..."     │
│   ...                                │
└─────────────────────────────────────┘
```

### Stretch Goals

| Feature | Priority | Complexity |
|---------|----------|------------|
| Regular expression search | Low | High (need regex engine like RE2) |
| Replace text | Low | Very High (requires content stream editing) |
| Search in file explorer | Low | Medium (Win32 shell integration) |
| Phonetic search | Very Low | High |
| Search result bookmarks | Low | Medium |

---

## Implementation Checklist

- [ ] Implement `CTextExtractor` using PDFium `FPDFText_GetText` APIs
- [ ] Implement `CSearchEngine` with synchronous search
- [ ] Implement background search thread with cancellation
- [ ] Implement case-sensitive and whole-word search options
- [ ] Implement search highlight overlay rendering
- [ ] Build `CSearchBar` UI with search field and navigation
- [ ] Implement match navigation (next/prev/count)
- [ ] Implement `Ctrl+F` / `Escape` keyboard shortcuts
- [ ] Add progress indication for large document searches
- [ ] Implement `CTextIndex` for large document optimization
- [ ] Add multi-document search ("Search All" mode)
- [ ] Add search results panel (stretch goal)
