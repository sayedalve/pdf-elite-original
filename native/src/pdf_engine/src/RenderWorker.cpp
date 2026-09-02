#include "../../utils/Logger.h"
#include "RenderWorker.h"
#include <string>
#include <windows.h>
#include "../../utils/Logger.h"
// We need core::interfaces::dom::IDocument for Clone() since IDocumentEngine doesn't have it natively, 
// or we can cast it if we know it's a PdfDocument.
// For now, let's include PdfDocument.h since we are in pdf_engine!
#include "EngineAdapter.h"

#define WM_APP_TILE_READY (WM_APP + 3)

namespace pdf_engine {

RenderWorker::RenderWorker(HWND hwndNotify, int numThreads) 
    : m_hwndNotify(hwndNotify) {
    for (int i = 0; i < numThreads; ++i) {
        m_threads.emplace_back(&RenderWorker::WorkerThread, this, i);
    }
}

RenderWorker::~RenderWorker() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_cv.notify_all();
    for (auto& t : m_threads) {
        if (t.joinable()) t.join();
    }
}

void RenderWorker::RegisterDocument(const std::wstring& id, std::shared_ptr<core::interfaces::IDocumentEngine> engine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto* adapter = static_cast<EngineAdapter*>(engine.get());
    utils::Logger::Log("REGISTER_DOCUMENT start, threads=" + std::to_string(m_threads.size()));
    std::vector<std::shared_ptr<core::interfaces::IDocumentEngine>> clones;
    for (size_t i = 0; i < m_threads.size(); ++i) {
        auto c = adapter->GetInner()->Clone();
        if (c) {
            clones.push_back(std::make_shared<EngineAdapter>(std::move(c)));
        } else {
            clones.push_back(std::make_shared<EngineAdapter>(adapter->GetInner()));
        }
    }
    m_threadDocs[id] = std::move(clones);
}

void RenderWorker::UnregisterDocument(const std::wstring& id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threadDocs.erase(id);
    std::priority_queue<RenderTaskWrapper> newQueue;
    while (!m_queue.empty()) {
        auto task = m_queue.top();
        m_queue.pop();
        if (task.req.documentId != id) {
            newQueue.push(task);
        }
    }
    m_queue = std::move(newQueue);
}

void RenderWorker::Enqueue(const core::models::RenderRequest& req) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push({req});
    m_cv.notify_one();
}

void RenderWorker::CancelAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue = std::priority_queue<RenderTaskWrapper>();
}

void RenderWorker::WorkerThread(int threadIndex) {
    while (true) {
        RenderTaskWrapper taskWrapper;
        std::shared_ptr<core::interfaces::IDocumentEngine> localDoc;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return m_stop || !m_queue.empty(); });
            if (m_stop) break;
            taskWrapper = m_queue.top();
            m_queue.pop();
            
            auto it = m_threadDocs.find(taskWrapper.req.documentId);
            if (it != m_threadDocs.end() && threadIndex < it->second.size()) {
                localDoc = it->second[threadIndex];
            }
        }

        const auto& req = taskWrapper.req;
        if (req.generation < m_currentGeneration || !localDoc) continue;

        float currentCy = m_screenCy.load(std::memory_order_relaxed);
        float vHeight = m_viewHeight.load(std::memory_order_relaxed);
        
        // Evict if tile is more than 3 viewports away from current viewport center.
        // This stops stale requests from stalling the worker queue during rapid scrolling.
        if (vHeight > 0.0f && req.tileCy != 0.0f) {
            if (std::abs(currentCy - req.tileCy) > vHeight * 3.0f) {
                continue; 
            }
        }

        auto result = localDoc->Render(req);
        if (m_stop) break;

        if (result) {
            PostMessage(m_hwndNotify, WM_APP_TILE_READY, reinterpret_cast<WPARAM>(result.release()), 0);
        }
    }
}

} // namespace pdf_engine
