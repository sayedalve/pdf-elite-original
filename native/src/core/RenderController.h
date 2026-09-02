#pragma once
#include "core/interfaces/IDocumentEngine.h"
#include <memory>
#include <functional>
#include "core/models/RenderRequest.h"
#include "core/models/RenderResult.h"

namespace core {

class RenderController {
public:
    static RenderController& Instance() {
        static RenderController instance;
        return instance;
    }

    // Initialize with a callback to receive results on the main thread
    void Initialize(std::function<void(std::unique_ptr<models::RenderResult>)> onResultCallback);

    // Provide the RenderWorker pointer/interface (dependency injection)
    class IRenderWorker {
    public:
        virtual ~IRenderWorker() = default;
        virtual void Enqueue(const models::RenderRequest& req) = 0;
        virtual void CancelAll() = 0;
        virtual void SetCurrentGeneration(int gen) = 0;
        virtual void SetViewport(float screenCy, float viewHeight) = 0;
        virtual void RegisterDocument(const std::wstring& id, std::shared_ptr<interfaces::IDocumentEngine> engine) = 0;
        virtual void UnregisterDocument(const std::wstring& id) = 0;
    };
    void SetWorker(std::unique_ptr<IRenderWorker> worker);
    
    // Dispatch a render request
    void EnqueueRequest(const models::RenderRequest& req);
    void CancelAll();
    void SetCurrentGeneration(int gen);
    int m_currentGeneration = 1;
    int GetCurrentGeneration() const { return m_currentGeneration; }
    void SetViewport(float screenCy, float viewHeight);
    void RegisterDocument(const std::wstring& id, std::shared_ptr<interfaces::IDocumentEngine> engine);
    void UnregisterDocument(const std::wstring& id);
    
    // Called by the message pump when WM_APP_TILE_READY is received
    void OnResultReady(models::RenderResult* resultPtr);

private:
    RenderController() = default;
    
    std::function<void(std::unique_ptr<models::RenderResult>)> m_callback;
    std::unique_ptr<IRenderWorker> m_worker;
};

} // namespace core




