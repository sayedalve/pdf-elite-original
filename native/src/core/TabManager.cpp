#include "RenderController.h"
#include "TabManager.h"

namespace core {

DocumentSession* TabManager::CreateSession(const std::wstring& path, std::unique_ptr<interfaces::IDocumentEngine> engine) {
    std::wstring id = L"doc_" + std::to_wstring(m_nextId++);
    auto session = std::make_unique<DocumentSession>(id, path, std::move(engine));
    DocumentSession* ptr = session.get();
    m_sessions.push_back(std::move(session));
    RenderController::Instance().RegisterDocument(id, ptr->GetEngine());
    ActivateSession(id);
    return ptr;
}

bool TabManager::CloseSession(const std::wstring& id) {
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if ((*it)->GetId() == id) {
            if (m_activeSession == it->get()) {
                m_activeSession = nullptr;
            }
            RenderController::Instance().UnregisterDocument(id);
            m_sessions.erase(it);
            if (!m_sessions.empty() && !m_activeSession) {
                m_activeSession = m_sessions.back().get();
            }
            return true;
        }
    }
    return false;
}

DocumentSession* TabManager::GetSession(const std::wstring& id) {
    for (const auto& session : m_sessions) {
        if (session->GetId() == id) return session.get();
    }
    return nullptr;
}

bool TabManager::ActivateSession(const std::wstring& id) {
    DocumentSession* session = GetSession(id);
    if (session) {
        m_activeSession = session;
        return true;
    }
    return false;
}

} // namespace core



