#pragma once
#include "core/interfaces/IDocumentEngine.h"
#include "core/interfaces/dom/IDocument.h"
#include "SearchEngine.h"
#include <memory>

namespace pdf_engine {

class EngineAdapter : public core::interfaces::IDocumentEngine {
public:
    EngineAdapter(std::shared_ptr<core::interfaces::dom::IDocument> doc);
    
    bool Open(const std::wstring&) override { return false; }
    void Close() override {}
    bool Save() override { return false; }
    bool SaveAs(const std::wstring&) override { return false; }
    bool Undo() override { return m_doc ? m_doc->GetCommandStack().Undo() : false; }
    bool Redo() override { return m_doc ? m_doc->GetCommandStack().Redo() : false; }
    bool CanUndo() const override { return m_doc ? m_doc->GetCommandStack().CanUndo() : false; }
    bool CanRedo() const override { return m_doc ? m_doc->GetCommandStack().CanRedo() : false; }
    int GetPageCount() const override { return m_doc ? m_doc->PageCount() : 0; }
    
    std::unique_ptr<core::models::RenderResult> Render(const core::models::RenderRequest& req) override;
    
    void SearchAsync(const std::wstring& query, bool matchCase, bool wholeWord, std::function<void(const std::vector<core::models::SearchResult>&)> onComplete) override;
    void CancelSearch() override;
    
    bool InsertPage(int, float, float) override { return false; }
    bool DeletePage(int) override { return false; }
    bool RotatePage(int, int) override { return false; }
    bool ExtractPage(int, const std::wstring&) override { return false; }

    bool AddText(int, float, float, const std::wstring&, const std::wstring&, float) override { return false; }
    bool DeleteText(int, const std::wstring&) override { return false; }
    
    bool AddAnnotation(int, const std::wstring&, float, float, float, float) override { return false; }
    bool MoveAnnotation(int, const std::string&, float, float, float, float) override { return false; }
    bool MoveText(int, uint64_t, float, float, float, float) override { return false; }
    bool MoveImage(int, uint64_t, float, float, float, float) override { return false; }
    bool DeleteAnnotation(int, const std::wstring&) override { return false; }

    bool ExecuteMoveAnnotation(int pageIndex, const std::string& annotId, float oldLeft, float oldTop, float oldRight, float oldBottom, float newLeft, float newTop, float newRight, float newBottom) override;
    bool ExecuteDeleteAnnotation(int pageIndex, const std::string& annotId) override;
    
    bool ExecuteMoveText(int pageIndex, uint64_t textId, float oldLeft, float oldTop, float oldRight, float oldBottom, float newLeft, float newTop, float newRight, float newBottom) override;
    bool ExecuteDeleteText(int pageIndex, uint64_t textId) override;
    
    bool ExecuteMoveImage(int pageIndex, uint64_t imageId, float oldLeft, float oldTop, float oldRight, float oldBottom, float newLeft, float newTop, float newRight, float newBottom) override;
    bool ExecuteDeleteImage(int pageIndex, uint64_t imageId) override;
    
    bool ExecuteAddMarkup(int pageIndex, const std::wstring& type, const std::vector<core::models::QuadF>& quads) override;

    
    std::shared_ptr<core::interfaces::dom::IDocument> GetInner() const { return m_doc; }
    
private:
    std::shared_ptr<core::interfaces::dom::IDocument> m_doc;
    std::unique_ptr<SearchEngine> m_search;
};

} // namespace pdf_engine
