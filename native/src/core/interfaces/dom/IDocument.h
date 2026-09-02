#pragma once
#include "IPage.h"
#include <memory>
#include "IAnnotation.h"
#include "CommandStack.h"
#include <string>
#include <vector>
#include "IFormFillCallback.h"
#include "IFormField.h"
#include <optional>
#include "Navigation.h"


namespace core {
namespace interfaces {
namespace dom {
class IDocument {
public:
    virtual ~IDocument() = default;
    virtual int PageCount() const = 0;
    
    // Command History
    virtual core::interfaces::dom::CommandStack& GetCommandStack() = 0;

    virtual std::shared_ptr<IPage> GetPage(int index) = 0;
    virtual SizeF GetPageSize(int index) const = 0;
    virtual std::string GetMetadata(const std::string& key) const = 0;
    
    // Bookmarks
    virtual std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> GetBookmarks() = 0;
    
    virtual bool Save() = 0;
    virtual bool SaveAs(const std::wstring& path) = 0;
    virtual void InvalidateOpenPages() = 0;

    // Page Operations
    virtual bool DeletePage(int index) = 0;
    virtual bool InsertBlankPage(int index, double width, double height) = 0;
    virtual bool DuplicatePage(int index) = 0;
    virtual bool MovePage(int sourceIndex, int destIndex) = 0;
    virtual bool RotatePage(int index, int rotationDegrees) = 0; // 0, 90, 180, 270
    
    // Document Operations
    virtual std::unique_ptr<IDocument> ExtractPages(const std::vector<int>& indices) = 0;
    virtual bool InsertPagesFrom(IDocument* sourceDoc, const std::vector<int>& sourceIndices, int destIndex) = 0;
    virtual std::unique_ptr<IDocument> Clone() = 0;
    
    // Forms
    virtual bool HasForms() const = 0;
    virtual bool InitializeFormFill(std::shared_ptr<core::interfaces::dom::IFormFillCallback> callback) = 0;
    virtual void ShutdownFormFill() = 0;
    virtual std::vector<std::shared_ptr<core::interfaces::dom::IFormField>> GetFormFields() = 0;

    // Form Interaction
    virtual bool OnLButtonDown(int pageIndex, double page_x, double page_y, int modifiers) = 0;
    virtual bool OnLButtonUp(int pageIndex, double page_x, double page_y, int modifiers) = 0;
    virtual bool OnMouseMove(int pageIndex, double page_x, double page_y, int modifiers) = 0;
    virtual bool OnKeyDown(int keyCode, int modifiers) = 0;
    virtual bool OnChar(int charCode, int modifiers) = 0;
    virtual void KillFocus() = 0;
};

} // namespace dom
} // namespace interfaces
} // namespace core
