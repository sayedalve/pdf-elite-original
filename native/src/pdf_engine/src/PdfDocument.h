#pragma once
#include "core/interfaces/dom/IDocument.h"

#include "core/Result.h"
#include <fpdfview.h>
#include <mutex>
#include <vector>
#include <map>

enum class TextGroupingMode {
    Line,
    Paragraph
};

class PdfDocument : public core::interfaces::dom::IDocument {
public:
    static Result<std::unique_ptr<PdfDocument>> LoadFromFile(const wchar_t* path);
    static Result<std::unique_ptr<PdfDocument>> LoadFromMemory(const uint8_t* data, size_t len);

    PdfDocument(FPDF_DOCUMENT doc);
    ~PdfDocument() override;

    // Grouping Mode
    TextGroupingMode GetGroupingMode() const { return m_groupingMode; }
    void SetGroupingMode(TextGroupingMode mode) { m_groupingMode = mode; }

    // Internal handle access for commands
    FPDF_DOCUMENT GetHandle() const { return m_doc; }

    PdfDocument(const PdfDocument&) = delete;
    PdfDocument& operator=(const PdfDocument&) = delete;
    static Result<std::shared_ptr<PdfDocument>> LoadFromFile(const std::wstring& path);
    
    bool Save() override;
    bool SaveAs(const std::wstring& path) override;

    int PageCount() const override;
    
    // Command History
    core::interfaces::dom::CommandStack& GetCommandStack() override { return m_commandStack; }
    
    std::shared_ptr<core::interfaces::dom::IPage> GetPage(int index) override;
    SizeF GetPageSize(int index) const override;
    std::string GetMetadata(const std::string& key) const override;
    
    // Bookmarks
    std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> GetBookmarks() override;

    // Page Operations
    bool DeletePage(int index) override;
    bool InsertBlankPage(int index, double width, double height) override;
    bool DuplicatePage(int index) override;
    bool MovePage(int sourceIndex, int destIndex) override;
    bool RotatePage(int index, int rotationDegrees) override;
    
    // Document Operations
    std::unique_ptr<core::interfaces::dom::IDocument> ExtractPages(const std::vector<int>& indices) override;
    bool InsertPagesFrom(core::interfaces::dom::IDocument* sourceDoc, const std::vector<int>& sourceIndices, int destIndex) override;
    std::unique_ptr<core::interfaces::dom::IDocument> Clone() override;
    
    // Internal API for getting FPDF_DOCUMENT
    FPDF_DOCUMENT GetFpdfDocument() const { return m_doc; }
    
    // Forms
    std::shared_ptr<core::interfaces::dom::IFormFillCallback> GetFormCallback() const { return m_formCallback; }

    bool HasForms() const override;
    bool InitializeFormFill(std::shared_ptr<core::interfaces::dom::IFormFillCallback> callback) override;
    void ShutdownFormFill() override;
    std::vector<std::shared_ptr<core::interfaces::dom::IFormField>> GetFormFields() override;

    // Form Interaction
    bool OnLButtonDown(int pageIndex, double page_x, double page_y, int modifiers) override;
    bool OnLButtonUp(int pageIndex, double page_x, double page_y, int modifiers) override;
    bool OnMouseMove(int pageIndex, double page_x, double page_y, int modifiers) override;
    bool OnKeyDown(int keyCode, int modifiers) override;
    bool OnChar(int charCode, int modifiers) override;
    void KillFocus() override;
    
    // Internal API for rendering
    void* GetFormHandle() const;

    // Mutex for thread-safe PDFium access
    std::recursive_mutex& GetMutex() const { return m_mutex; }
    
    // Internal API for page lifecycle management
    void ReleasePage(int index);
    void InvalidateOpenPages() override;

private:
    FPDF_DOCUMENT m_doc = nullptr;
    mutable std::recursive_mutex m_mutex;
    TextGroupingMode m_groupingMode = TextGroupingMode::Paragraph;
    std::vector<uint8_t> m_buffer; // For memory-loaded docs
    
    // Custom file access to avoid locking the original file against renaming
    HANDLE m_fileHandle = INVALID_HANDLE_VALUE;
    FPDF_FILEACCESS m_fileAccess = {};
    
    core::interfaces::dom::CommandStack m_commandStack;
    
    std::map<int, FPDF_PAGE> m_openPages;
    std::map<int, int> m_pageRefCounts;
    
    // Forms
    void* m_formHandle = nullptr;
    std::shared_ptr<core::interfaces::dom::IFormFillCallback> m_formCallback;
    void* m_formFillInfo = nullptr; // Actually FPDF_FORMFILLINFO*
};

