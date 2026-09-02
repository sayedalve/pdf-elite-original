#pragma once
#include <string>
#include <vector>
#include <functional>
#include "models/SearchResult.h"
#include "TabManager.h"

namespace core {

class SearchController {
public:
    static SearchController& Instance() {
        static SearchController instance;
        return instance;
    }

    void SearchAsync(const std::wstring& documentId, const std::wstring& query, bool matchCase, bool wholeWord) {
        auto session = TabManager::Instance().GetSession(documentId);
        if (!session) return;
        
        auto engine = session->GetEngine();
        if (!engine) return;
        
        engine->CancelSearch();
        
        if (query.empty()) {
            session->SetSearchResults({});
            session->SetActiveSearchIndex(-1);
            if (onSearchResultsUpdated) onSearchResultsUpdated(documentId);
            return;
        }
        
        engine->SearchAsync(query, matchCase, wholeWord, [this, documentId](const std::vector<models::SearchResult>& results) {
            auto session = TabManager::Instance().GetSession(documentId);
            if (!session) return;
            session->SetSearchResults(results);
            session->SetActiveSearchIndex(results.empty() ? -1 : 0);
            if (onSearchResultsUpdated) onSearchResultsUpdated(documentId);
        });
    }

    void CancelSearch(const std::wstring& documentId) {
        auto session = TabManager::Instance().GetSession(documentId);
        if (!session || !session->GetEngine()) return;
        session->GetEngine()->CancelSearch();
    }
    
    void NextMatch(const std::wstring& documentId) {
        auto session = TabManager::Instance().GetSession(documentId);
        if (!session) return;
        auto results = session->GetSearchResults();
        if (results.empty()) return;
        int idx = session->GetActiveSearchIndex();
        idx = static_cast<int>((idx + 1) % results.size());
        session->SetActiveSearchIndex(idx);
        if (onSearchIndexChanged) onSearchIndexChanged(documentId, idx);
    }

    void PrevMatch(const std::wstring& documentId) {
        auto session = TabManager::Instance().GetSession(documentId);
        if (!session) return;
        auto results = session->GetSearchResults();
        if (results.empty()) return;
        int idx = session->GetActiveSearchIndex();
        idx = static_cast<int>((idx - 1 + results.size()) % results.size());
        session->SetActiveSearchIndex(idx);
        if (onSearchIndexChanged) onSearchIndexChanged(documentId, idx);
    }
    
    // UI can subscribe to these
    std::function<void(const std::wstring&)> onSearchResultsUpdated;
    std::function<void(const std::wstring&, int)> onSearchIndexChanged;

private:
    SearchController() = default;
};

} // namespace core

