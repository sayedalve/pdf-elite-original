// RenderWorker.h - Background PDF rendering worker
#pragma once
#include <windows.h>
#include <thread>
#include <queue>
#include <mutex>

namespace PdfElite {

struct RenderTask {
    int pageNumber;
    float zoom;
    bool highQuality;
};

class RenderWorker {
public:
    RenderWorker();
    ~RenderWorker();

    void Start();
    void Stop();
    void QueueTask(const RenderTask& task);
    bool GetRenderedPage(int pageNumber, void** bitmap);

private:
    void WorkerThread();

    std::thread m_thread;
    std::queue<RenderTask> m_queue;
    std::mutex m_mutex;
    bool m_running = false;
    HANDLE m_event = nullptr;
};

} // namespace PdfElite
