#pragma once

#include "core/interfaces/dom/ICommand.h"
#include "core/interfaces/dom/IDocument.h"
#include <windows.h>
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace pdf_engine {
namespace commands {

struct BackgroundParams {
    bool isColor = true;                    // true = solid color, false = image file
    COLORREF color = RGB(245, 245, 245);
    double opacity = 1.0;                   // 0.0 to 1.0
    std::wstring imagePath;
    int pageScope = 0;                      // 0 = All pages, 1 = Current page, 2 = Custom range
    std::wstring pageRange = L"1";
    int currentPage = 1;                    // 1-based current page
    int totalPages = 1;                     // Total document pages
};

class AddBackgroundCommand : public core::interfaces::dom::ICommand {
public:
    AddBackgroundCommand(std::shared_ptr<core::interfaces::dom::IDocument> doc, const BackgroundParams& params);
    AddBackgroundCommand(core::interfaces::dom::IDocument* doc, const BackgroundParams& params);
    ~AddBackgroundCommand() override;

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Add Background"; }

private:
    core::interfaces::dom::IDocument* GetDoc() const;

    std::shared_ptr<core::interfaces::dom::IDocument> m_docShared;
    core::interfaces::dom::IDocument* m_docRaw = nullptr;
    BackgroundParams m_params;
    std::map<int, FPDF_PAGEOBJECT> m_createdObjects; // pageIndex -> FPDF_PAGEOBJECT
    bool m_applied = false;
};

} // namespace commands
} // namespace pdf_engine
