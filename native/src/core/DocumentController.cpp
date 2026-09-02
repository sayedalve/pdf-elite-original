#include "DocumentController.h"
#include "RecentFilesManager.h"

namespace core {

bool DocumentController::OpenDocument(const std::wstring& path) {
    if (!m_factory) return false;
    
    auto engine = m_factory();
    if (!engine->Open(path)) {
        return false;
    }
    
    TabManager::Instance().CreateSession(path, std::move(engine));
    RecentFilesManager::Instance().AddFile(path);
    return true;
}

bool DocumentController::CloseDocument(const std::wstring& id) {
    return TabManager::Instance().CloseSession(id);
}

bool DocumentController::SaveDocument(const std::wstring& id) {
    auto session = TabManager::Instance().GetSession(id);
    if (session && session->GetEngine()) {
        bool saved = session->GetEngine()->Save();
        if (saved) session->SetModified(false);
        return saved;
    }
    return false;
}

bool DocumentController::SaveAsDocument(const std::wstring& id, const std::wstring& path) {
    auto session = TabManager::Instance().GetSession(id);
    if (session && session->GetEngine()) {
        bool saved = session->GetEngine()->SaveAs(path);
        if (saved) session->SetModified(false);
        return saved;
    }
    return false;
}

bool DocumentController::ActivateDocument(const std::wstring& id) {
    return TabManager::Instance().ActivateSession(id);
}

void DocumentController::GoToPage(const std::wstring& id, int pageIndex) {
    auto session = TabManager::Instance().GetSession(id);
    if (session) {
        session->SetCurrentPage(pageIndex);
    }
}

void DocumentController::SetZoom(const std::wstring& id, float zoom) {
    auto session = TabManager::Instance().GetSession(id);
    if (session) {
        session->SetZoom(zoom);
    }
}



bool DocumentController::Undo(const std::wstring& id) {
    auto session = TabManager::Instance().GetSession(id);
    if (!session || !session->GetEngine()) return false;
    bool result = session->GetEngine()->Undo();
    if (result) session->IncrementGeneration();
    return result;
}

bool DocumentController::Redo(const std::wstring& id) {
    auto session = TabManager::Instance().GetSession(id);
    if (!session || !session->GetEngine()) return false;
    bool result = session->GetEngine()->Redo();
    if (result) session->IncrementGeneration();
    return result;
}

bool DocumentController::CanUndo(const std::wstring& id) {
    auto session = TabManager::Instance().GetSession(id);
    if (!session || !session->GetEngine()) return false;
    return session->GetEngine()->CanUndo();
}

bool DocumentController::CanRedo(const std::wstring& id) {
    auto session = TabManager::Instance().GetSession(id);
    if (!session || !session->GetEngine()) return false;
    return session->GetEngine()->CanRedo();
}

bool DocumentController::ExecuteMoveAnnotation(const std::wstring& id, int pageIndex, const std::string& annotId, float oldLeft, float oldTop, float oldRight, float oldBottom, float newLeft, float newTop, float newRight, float newBottom) {
    auto session = TabManager::Instance().GetSession(id);
    if (!session || !session->GetEngine()) return false;
    return session->GetEngine()->ExecuteMoveAnnotation(pageIndex, annotId, oldLeft, oldTop, oldRight, oldBottom, newLeft, newTop, newRight, newBottom);
}

bool DocumentController::ExecuteDeleteAnnotation(const std::wstring& id, int pageIndex, const std::string& annotId) {
    auto session = TabManager::Instance().GetSession(id);
    if (!session || !session->GetEngine()) return false;
    return session->GetEngine()->ExecuteDeleteAnnotation(pageIndex, annotId);
}

bool DocumentController::ExecuteMoveText(const std::wstring& id, int pageIndex, uint64_t textId, float oldLeft, float oldTop, float oldRight, float oldBottom, float newLeft, float newTop, float newRight, float newBottom) {
    auto session = TabManager::Instance().GetSession(id);
    if (!session || !session->GetEngine()) return false;
    return session->GetEngine()->ExecuteMoveText(pageIndex, textId, oldLeft, oldTop, oldRight, oldBottom, newLeft, newTop, newRight, newBottom);
}

bool DocumentController::ExecuteDeleteText(const std::wstring& id, int pageIndex, uint64_t textId) {
    auto session = TabManager::Instance().GetSession(id);
    if (!session || !session->GetEngine()) return false;
    return session->GetEngine()->ExecuteDeleteText(pageIndex, textId);
}

bool DocumentController::ExecuteMoveImage(const std::wstring& id, int pageIndex, uint64_t imageId, float oldLeft, float oldTop, float oldRight, float oldBottom, float newLeft, float newTop, float newRight, float newBottom) {
    auto session = TabManager::Instance().GetSession(id);
    if (!session || !session->GetEngine()) return false;
    return session->GetEngine()->ExecuteMoveImage(pageIndex, imageId, oldLeft, oldTop, oldRight, oldBottom, newLeft, newTop, newRight, newBottom);
}

bool DocumentController::ExecuteDeleteImage(const std::wstring& id, int pageIndex, uint64_t imageId) {
    auto session = TabManager::Instance().GetSession(id);
    if (!session || !session->GetEngine()) return false;
    return session->GetEngine()->ExecuteDeleteImage(pageIndex, imageId);
}

bool DocumentController::ExecuteAddMarkup(const std::wstring& id, int pageIndex, const std::wstring& type, const std::vector<models::QuadF>& quads) {
    auto session = TabManager::Instance().GetSession(id);
    if (!session || !session->GetEngine()) return false;
    return session->GetEngine()->ExecuteAddMarkup(pageIndex, type, quads);
}

} // namespace core

