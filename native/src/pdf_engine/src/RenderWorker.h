#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <vector>
#include <map>
#include <windows.h>
#include "core/RenderController.h"

namespace pdf_engine {

class RenderWorker : public core::RenderController::IRenderWorker {
public:
    RenderWorker(HWND hwndNotify, int numThreads = 1);
    ~RenderWorker() override;
    
    void Enqueue(const core::models::RenderRequest& req) override;
    void CancelAll() override;
    void SetCurrentGeneration(int gen) override { m_currentGeneration = gen; }
    void SetViewport(float screenCy, float viewHeight) override {
        m_screenCy.store(screenCy, std::memory_order_relaxed);
        m_viewHeight.store(viewHeight, std::memory_order_relaxed);
    }
    void RegisterDocument(const std::wstring& id, std::shared_ptr<core::interfaces::IDocumentEngine> engine) override;
    void UnregisterDocument(const std::wstring& id) override;

private:
    void WorkerThread(int threadIndex);

    struct RenderTaskWrapper {
        core::models::RenderRequest req;
        bool operator<(const RenderTaskWrapper& other) const {
            if (req.category != other.req.category) {
                return req.category < other.req.category;
            }
            return req.priority < other.req.priority;
        }
    };

    HWND m_hwndNotify;
    
    // Each thread gets a clone of the document for concurrent rendering
    std::map<std::wstring, std::vector<std::shared_ptr<core::interfaces::IDocumentEngine>>> m_threadDocs;
    
    std::priority_queue<RenderTaskWrapper> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<std::thread> m_threads;
    std::atomic<bool> m_stop{false};
    std::atomic<int> m_currentGeneration{0};
    std::atomic<float> m_screenCy{0.0f};
    std::atomic<float> m_viewHeight{0.0f};
};

} // namespace pdf_engine
