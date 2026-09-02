#pragma once
#include <string>
#include <vector>
#include <memory>
#include "DocumentSession.h"

namespace core {

class TabManager {
public:
    static TabManager& Instance() {
        static TabManager instance;
        return instance;
    }

    DocumentSession* CreateSession(const std::wstring& path, std::unique_ptr<interfaces::IDocumentEngine> engine);
    bool CloseSession(const std::wstring& id);
    
    DocumentSession* GetSession(const std::wstring& id);
    DocumentSession* GetActiveSession() { return m_activeSession; }
    
    bool ActivateSession(const std::wstring& id);
    
    const std::vector<std::unique_ptr<DocumentSession>>& GetSessions() const { return m_sessions; }

private:
    TabManager() = default;
    
    std::vector<std::unique_ptr<DocumentSession>> m_sessions;
    DocumentSession* m_activeSession = nullptr;
    int m_nextId = 1;
};

} // namespace core
