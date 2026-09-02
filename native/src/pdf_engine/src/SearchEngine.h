#pragma once
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include "core/interfaces/dom/IDocument.h"
#include "core/interfaces/dom/ITextPage.h"
#include "core/models/SearchResult.h"

namespace pdf_engine {

using SearchResult = core::models::SearchResult;

class SearchEngine {
public:
    SearchEngine(std::shared_ptr<core::interfaces::dom::IDocument> doc);
    ~SearchEngine();

    void SearchAsync(const std::wstring& query, bool matchCase, bool wholeWord,
                     std::function<void(const std::vector<SearchResult>&)> onComplete);

    void Cancel();

private:
    // Signals cancellation and joins any in-flight search thread. Called before
    // starting a new search and from the destructor so the worker (which reads
    // m_doc / m_cancel) can never outlive this object.
    void StopWorker();

    std::shared_ptr<core::interfaces::dom::IDocument> m_doc;
    std::atomic<bool> m_cancel{false};
    std::thread m_worker; // owns the in-flight search; joined before restart/destroy
};

}
