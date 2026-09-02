#include "EngineAdapter.h"
#include "core/models/RenderResult.h"

namespace pdf_engine {

EngineAdapter::EngineAdapter(std::shared_ptr<core::interfaces::dom::IDocument> doc) : m_doc(doc) {}

std::unique_ptr<core::models::RenderResult> EngineAdapter::Render(const core::models::RenderRequest& req) {
    if (!m_doc) return nullptr;
    try {
        auto page = m_doc->GetPage(req.pageIndex);
        if (!page) return nullptr;

        int pStartX = static_cast<int>(req.tileRect.left * req.dpi);
        int pStartY = static_cast<int>(req.tileRect.top * req.dpi);
        int pWidth = static_cast<int>((req.tileRect.right - req.tileRect.left) * req.dpi);
        int pHeight = static_cast<int>((req.tileRect.bottom - req.tileRect.top) * req.dpi);
        
        if (pWidth <= 0 || pHeight <= 0 || pWidth > 8192 || pHeight > 8192) {
            return nullptr;
        }
        
        auto buffer = page->RenderToBitmap(req.renderScale * req.dpi, pStartX, pStartY, pWidth, pHeight, req.darkMode);
        if (buffer.empty()) return nullptr;

        auto result = std::make_unique<core::models::RenderResult>();
        result->documentId = req.documentId;
        result->generation = req.generation;
        result->pageIndex = req.pageIndex;
        result->viewport = req.viewport;
        result->tileRect = req.tileRect;
        result->renderScale = req.renderScale;
        result->dpi = req.dpi;
        result->width = pWidth;
        result->height = pHeight;
        result->stride = pWidth * 4;
        result->pixelFormat = core::models::PixelFormat::BGRA8;
        result->pixelBuffer = std::move(buffer);
        
        return result;
    } catch (...) {
        return nullptr;
    }
}
void EngineAdapter::SearchAsync(const std::wstring& query, bool matchCase, bool wholeWord, std::function<void(const std::vector<core::models::SearchResult>&)> onComplete) {
    if (!m_search) {
        m_search = std::make_unique<SearchEngine>(m_doc);
    }
    m_search->SearchAsync(query, matchCase, wholeWord, onComplete);
}

void EngineAdapter::CancelSearch() {
    if (m_search) {
        m_search->Cancel();
    }
}

bool EngineAdapter::ExecuteMoveAnnotation(int /*pageIndex*/, const std::string& /*annotId*/, float /*oldLeft*/, float /*oldTop*/, float /*oldRight*/, float /*oldBottom*/, float /*newLeft*/, float /*newTop*/, float /*newRight*/, float /*newBottom*/) { return false; }
bool EngineAdapter::ExecuteDeleteAnnotation(int /*pageIndex*/, const std::string& /*annotId*/) { return false; }
bool EngineAdapter::ExecuteMoveText(int /*pageIndex*/, uint64_t /*textId*/, float /*oldLeft*/, float /*oldTop*/, float /*oldRight*/, float /*oldBottom*/, float /*newLeft*/, float /*newTop*/, float /*newRight*/, float /*newBottom*/) { return false; }
bool EngineAdapter::ExecuteDeleteText(int /*pageIndex*/, uint64_t /*textId*/) { return false; }
bool EngineAdapter::ExecuteMoveImage(int /*pageIndex*/, uint64_t /*imageId*/, float /*oldLeft*/, float /*oldTop*/, float /*oldRight*/, float /*oldBottom*/, float /*newLeft*/, float /*newTop*/, float /*newRight*/, float /*newBottom*/) { return false; }
bool EngineAdapter::ExecuteDeleteImage(int /*pageIndex*/, uint64_t /*imageId*/) { return false; }
bool EngineAdapter::ExecuteAddMarkup(int /*pageIndex*/, const std::wstring& /*type*/, const std::vector<core::models::QuadF>& /*quads*/) { return false; }


} // namespace pdf_engine

