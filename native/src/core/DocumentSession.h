#pragma once
#include <string>
#include <memory>
#include "interfaces/IDocumentEngine.h"
#include "models/SearchResult.h"
#include <vector>

namespace core {

class DocumentSession {
public:
    DocumentSession(const std::wstring& id, const std::wstring& path, std::unique_ptr<interfaces::IDocumentEngine> engine)
        : m_id(id), m_path(path), m_engine(std::move(engine)) {}
    ~DocumentSession() = default;

    std::wstring GetId() const { return m_id; }
    std::wstring GetPath() const { return m_path; }
    
    std::shared_ptr<interfaces::IDocumentEngine> GetEngine() { return m_engine; }
    
    int GetGeneration() const { return m_generation; }
    void IncrementGeneration() { m_generation++; }

    bool IsModified() const { return m_modified; }
    void SetModified(bool modified) { m_modified = modified; }

    int GetCurrentPage() const { return m_currentPage; }
    void SetCurrentPage(int page) { m_currentPage = page; }

    float GetZoom() const { return m_zoom; }
    void SetZoom(float zoom) { m_zoom = zoom; }

    float GetScrollX() const { return m_scrollX; }
    float GetScrollY() const { return m_scrollY; }
    
    const std::vector<models::SearchResult>& GetSearchResults() const { return m_searchResults; }
    void SetSearchResults(const std::vector<models::SearchResult>& results) { m_searchResults = results; }
    
    int GetActiveSearchIndex() const { return m_activeSearchIndex; }
    void SetActiveSearchIndex(int index) { m_activeSearchIndex = index; }
    void SetScroll(float x, float y) { m_scrollX = x; m_scrollY = y; }

private:
    std::wstring m_id;
    std::wstring m_path;
    std::shared_ptr<interfaces::IDocumentEngine> m_engine;
    
    int m_generation = 0;
    bool m_modified = false;
    int m_currentPage = 0;
    float m_zoom = 1.0f;
    float m_scrollX = 0.0f;
    float m_scrollY = 0.0f;
    std::vector<models::SearchResult> m_searchResults;
    int m_activeSearchIndex = -1;
};

} // namespace core

