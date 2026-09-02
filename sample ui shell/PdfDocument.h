// PdfDocument.h - PDF Engine interface, separated from UI
// UI must not contain engine logic, engine must not decide layout
#pragma once
#include <d2d1.h>
#include <string>
#include <vector>

namespace PdfElite {

class PdfDocument {
public:
    PdfDocument();
    ~PdfDocument();

    bool Load(const std::wstring& path);
    void Unload();
    bool IsLoaded() const { return m_loaded; }

    int GetPageCount() const { return m_pageCount; }
    void RenderPage(ID2D1RenderTarget* rt, int pageIndex, const D2D1_RECT_F& rect, float zoom);

    // Search
    struct SearchResult { int page; D2D1_RECT_F bounds; };
    std::vector<SearchResult> Search(const std::wstring& query);
    void ClearSearch() { m_searchResults.clear(); }

    // Editing - preserve existing functionality, don't rewrite engine
    bool CanUndo() const { return m_canUndo; }
    bool CanRedo() const { return m_canRedo; }
    void Undo();
    void Redo();

    // Metadata
    std::wstring GetFileName() const { return m_fileName; }
    std::wstring GetPath() const { return m_path; }

private:
    bool m_loaded = false;
    int m_pageCount = 51; // As in screenshots 4/51
    std::wstring m_path;
    std::wstring m_fileName = L"Lecture 1 & 2.pdf";
    bool m_canUndo = false;
    bool m_canRedo = false;
    std::vector<SearchResult> m_searchResults;

    // Existing PDFium integration would be here - preserve
    void* m_pdfiumDoc = nullptr;
};

} // namespace PdfElite
