#pragma once
#include <string>
#include <memory>
#include <vector>
#include "core/models/RenderRequest.h"
#include "core/models/RenderResult.h"
#include "core/models/SearchResult.h"
#include "core/models/QuadF.h"
#include <functional>

namespace core {
namespace interfaces {

class IDocumentEngine {
public:
    virtual ~IDocumentEngine() = default;

    // Lifecycle
    virtual bool Open(const std::wstring& path) = 0;
    virtual void Close() = 0;
    virtual bool Save() = 0;
    virtual bool SaveAs(const std::wstring& path) = 0;

    virtual bool Undo() = 0;
    virtual bool Redo() = 0;
    virtual bool CanUndo() const = 0;
    virtual bool CanRedo() const = 0;

    // Document info
    virtual int GetPageCount() const = 0;
    
    // Rendering
    virtual std::unique_ptr<models::RenderResult> Render(const models::RenderRequest& request) = 0;

    // Search
    virtual void SearchAsync(const std::wstring& query, bool matchCase, bool wholeWord, std::function<void(const std::vector<models::SearchResult>&)> onComplete) = 0;
    virtual void CancelSearch() = 0;
    
    // Page operations
    virtual bool InsertPage(int index, float width, float height) = 0;
    virtual bool DeletePage(int index) = 0;
    virtual bool RotatePage(int index, int rotation) = 0;
    virtual bool ExtractPage(int index, const std::wstring& destPath) = 0;

    // Editing (high-level, no FPDF handles)
    virtual bool AddText(int pageIndex, float x, float y, const std::wstring& text, const std::wstring& font, float size) = 0;
    virtual bool DeleteText(int pageIndex, const std::wstring& textId) = 0;
    
    // Annotations (high-level)
    virtual bool AddAnnotation(int pageIndex, const std::wstring& type, float x, float y, float width, float height) = 0;
    virtual bool MoveAnnotation(int pageIndex, const std::string& annotId, float oldX, float oldY, float newX, float newY) = 0;
    virtual bool MoveText(int pageIndex, uint64_t textId, float oldX, float oldY, float newX, float newY) = 0;
    virtual bool MoveImage(int pageIndex, uint64_t imageId, float oldX, float oldY, float newX, float newY) = 0;
    virtual bool DeleteAnnotation(int pageIndex, const std::wstring& annotId) = 0;

    // High-level editing (UI calls these instead of building commands)
    virtual bool ExecuteMoveAnnotation(int pageIndex, const std::string& annotId, float oldLeft, float oldTop, float oldRight, float oldBottom, float newLeft, float newTop, float newRight, float newBottom) = 0;
    virtual bool ExecuteDeleteAnnotation(int pageIndex, const std::string& annotId) = 0;
    
    virtual bool ExecuteMoveText(int pageIndex, uint64_t textId, float oldLeft, float oldTop, float oldRight, float oldBottom, float newLeft, float newTop, float newRight, float newBottom) = 0;
    virtual bool ExecuteDeleteText(int pageIndex, uint64_t textId) = 0;
    
    virtual bool ExecuteMoveImage(int pageIndex, uint64_t imageId, float oldLeft, float oldTop, float oldRight, float oldBottom, float newLeft, float newTop, float newRight, float newBottom) = 0;
    virtual bool ExecuteDeleteImage(int pageIndex, uint64_t imageId) = 0;
    
    virtual bool ExecuteAddMarkup(int pageIndex, const std::wstring& type, const std::vector<models::QuadF>& quads) = 0;

};

} // namespace interfaces
} // namespace core

