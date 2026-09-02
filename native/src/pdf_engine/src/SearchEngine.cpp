#include "SearchEngine.h"
#include <algorithm>
#include <cctype>
#include "../../utils/PerfLog.h"

namespace pdf_engine {

SearchEngine::SearchEngine(std::shared_ptr<core::interfaces::dom::IDocument> doc) : m_doc(doc) {}

SearchEngine::~SearchEngine() {
    StopWorker();
}

void SearchEngine::Cancel() {
    m_cancel = true;
}

void SearchEngine::StopWorker() {
    // Cancel and WAIT for the search thread to exit. The worker captures `this`
    // and touches m_doc / m_cancel, so it must be fully joined before this object
    // is destroyed or before m_worker is reassigned by a new search. (The previous
    // implementation detached the thread and only flipped m_cancel, which left the
    // detached thread reading a freed SearchEngine when a tab/app closed mid-search.)
    m_cancel = true;
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

void SearchEngine::SearchAsync(const std::wstring& query, bool matchCase, bool wholeWord,
                               std::function<void(const std::vector<SearchResult>&)> onComplete) {
    // Fully stop any previous search first, so only one thread ever accesses this
    // document (and this object) at a time. onComplete only PostMessages, so the
    // join here does not block on the UI thread.
    StopWorker();
    m_cancel = false;

    std::wstring searchQuery = query;
    if (!matchCase) {
        std::transform(searchQuery.begin(), searchQuery.end(), searchQuery.begin(), ::towlower);
    }

    m_worker = std::thread([this, searchQuery, matchCase, wholeWord, onComplete]() {
        std::vector<SearchResult> results;
        if (!m_doc) {
            if (!m_cancel) onComplete(results);
            return;
        }

        int pageCount = m_doc->PageCount();
        for (int p = 0; p < pageCount; ++p) {
            if (m_cancel) break;

            auto page = m_doc->GetPage(p);
            if (!page) continue;

            auto textPage = page->LoadTextPage();
            if (!textPage) continue;

            int charCount = textPage->GetCharCount();
            if (charCount <= 0) continue;

            std::wstring text = textPage->GetText(0, charCount);
            if (!matchCase) {
                std::transform(text.begin(), text.end(), text.begin(), ::towlower);
            }

            size_t pos = 0;
            while ((pos = text.find(searchQuery, pos)) != std::wstring::npos) {
                if (m_cancel) break;

                if (wholeWord) {
                    bool wordStart = (pos == 0 || !iswalnum(text[pos - 1]));
                    bool wordEnd = (pos + searchQuery.length() >= text.length() || !iswalnum(text[pos + searchQuery.length()]));
                    if (!wordStart || !wordEnd) {
                        pos += 1;
                        continue;
                    }
                }

                results.push_back({ p, static_cast<int>(pos), static_cast<int>(searchQuery.length()) });
                pos += searchQuery.length();
            }
        }

        if (!m_cancel) {
            onComplete(results);
        }
    });
}

}

