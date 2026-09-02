#pragma once

#include "core/interfaces/dom/ICommand.h"
#include "core/interfaces/dom/IDocument.h"
#include "core/interfaces/dom/IAnnotation.h"
#include <windows.h>
#include <memory>
#include <string>

namespace pdf_engine {
namespace commands {

struct LinkParams {
    int pageIndex = 0;              // 0-based page index where link is placed
    double x = 50.0;                // PDF points
    double y = 50.0;
    double width = 150.0;
    double height = 30.0;
    bool isUrl = true;              // true = URL, false = internal page navigation
    std::wstring url = L"https://";
    int targetPage = 1;             // 1-based target page
    bool drawBorder = false;
    COLORREF borderColor = RGB(0, 102, 204);
    int totalPages = 1;
};

class AddLinkCommand : public core::interfaces::dom::ICommand {
public:
    AddLinkCommand(std::shared_ptr<core::interfaces::dom::IDocument> doc, const LinkParams& params);
    AddLinkCommand(core::interfaces::dom::IDocument* doc, const LinkParams& params);
    ~AddLinkCommand() override = default;

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Add Link Annotation"; }

    std::shared_ptr<core::interfaces::dom::IAnnotation> GetAnnotation() const { return m_annot; }

private:
    core::interfaces::dom::IDocument* GetDoc() const;

    std::shared_ptr<core::interfaces::dom::IDocument> m_docShared;
    core::interfaces::dom::IDocument* m_docRaw = nullptr;
    LinkParams m_params;
    std::shared_ptr<core::interfaces::dom::IAnnotation> m_annot;
};

} // namespace commands
} // namespace pdf_engine
