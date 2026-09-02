#include "RenderController.h"

namespace core {

void RenderController::Initialize(std::function<void(std::unique_ptr<models::RenderResult>)> onResultCallback) {
    m_callback = std::move(onResultCallback);
}

void RenderController::SetWorker(std::unique_ptr<IRenderWorker> worker) {
    m_worker = std::move(worker);
}

void RenderController::EnqueueRequest(const models::RenderRequest& req) {
    if (m_worker) {
        m_worker->Enqueue(req);
    }
}

void RenderController::CancelAll() {
    if (m_worker) {
        m_worker->CancelAll();
    }
}

void RenderController::RegisterDocument(const std::wstring& id, std::shared_ptr<interfaces::IDocumentEngine> engine) {
    if (m_worker) m_worker->RegisterDocument(id, engine);
}

void RenderController::UnregisterDocument(const std::wstring& id) {
    if (m_worker) m_worker->UnregisterDocument(id);
}

void RenderController::SetCurrentGeneration(int gen) {
    m_currentGeneration = gen;
    if (m_worker) {
        m_worker->SetCurrentGeneration(gen);
    }
}

void RenderController::SetViewport(float screenCy, float viewHeight) {
    if (m_worker) {
        m_worker->SetViewport(screenCy, viewHeight);
    }
}

void RenderController::OnResultReady(models::RenderResult* resultPtr) {
    // Explicitly take ownership of the result emitted by the worker
    std::unique_ptr<models::RenderResult> result(resultPtr);
    if (m_callback && result) {
        m_callback(std::move(result));
    }
}

} // namespace core

