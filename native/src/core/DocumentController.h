#pragma once
#include <string>
#include <functional>
#include "TabManager.h"
#include "core/models/QuadF.h"
#include <vector>
#include "RenderController.h"

namespace core {

class DocumentController {
public:
    static DocumentController& Instance() {
        static DocumentController instance;
        return instance;
    }

    bool OpenDocument(const std::wstring& path);
    bool CloseDocument(const std::wstring& id);
    bool SaveDocument(const std::wstring& id);
    bool SaveAsDocument(const std::wstring& id, const std::wstring& path);
    
    bool ActivateDocument(const std::wstring& id);
    
    void GoToPage(const std::wstring& id, int pageIndex);
    void SetZoom(const std::wstring& id, float zoom);
    
    bool InsertPage(const std::wstring& id, int index, float width, float height);
    bool DeletePage(const std::wstring& id, int index);
    bool RotatePage(const std::wstring& id, int index, int rotation);

    // High-level editing
    bool ExecuteMoveAnnotation(const std::wstring& id, int pageIndex, const std::string& annotId, float oldLeft, float oldTop, float oldRight, float oldBottom, float newLeft, float newTop, float newRight, float newBottom);
    bool ExecuteDeleteAnnotation(const std::wstring& id, int pageIndex, const std::string& annotId);
    
    bool ExecuteMoveText(const std::wstring& id, int pageIndex, uint64_t textId, float oldLeft, float oldTop, float oldRight, float oldBottom, float newLeft, float newTop, float newRight, float newBottom);
    bool ExecuteDeleteText(const std::wstring& id, int pageIndex, uint64_t textId);
    
    bool ExecuteMoveImage(const std::wstring& id, int pageIndex, uint64_t imageId, float oldLeft, float oldTop, float oldRight, float oldBottom, float newLeft, float newTop, float newRight, float newBottom);
    bool ExecuteDeleteImage(const std::wstring& id, int pageIndex, uint64_t imageId);
    
    bool ExecuteAddMarkup(const std::wstring& id, int pageIndex, const std::wstring& type, const std::vector<models::QuadF>& quads);

    bool Undo(const std::wstring& id);
    bool Redo(const std::wstring& id);
    bool CanUndo(const std::wstring& id);
    bool CanRedo(const std::wstring& id);
    
    using EngineFactory = std::unique_ptr<interfaces::IDocumentEngine>(*)();
    void SetEngineFactory(EngineFactory factory) { m_factory = factory; }
    
    // Events
    std::function<void(const std::wstring&)> onDocumentOpened;
    std::function<void(const std::wstring&)> onDocumentClosed;
    std::function<void(const std::wstring&)> onDocumentStructureChanged;

private:
    DocumentController() = default;
    EngineFactory m_factory = nullptr;
};

} // namespace core
