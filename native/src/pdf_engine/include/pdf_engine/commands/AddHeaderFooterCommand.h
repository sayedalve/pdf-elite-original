#pragma once

#include "core/interfaces/dom/ICommand.h"
#include "core/interfaces/dom/IDocument.h"
#include <windows.h>
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace pdf_engine {
namespace commands {

struct HeaderFooterParams {
    std::wstring leftHeader;
    std::wstring centerHeader;
    std::wstring rightHeader;
    std::wstring leftFooter;
    std::wstring centerFooter;
    std::wstring rightFooter;
    std::wstring fontName = L"Helvetica";
    float fontSize = 10.0f;
    COLORREF color = RGB(0, 0, 0);
    float topMargin = 36.0f;                // Margins in PDF points (36 pt = 0.5 inch)
    float bottomMargin = 36.0f;
    float leftMargin = 36.0f;
    float rightMargin = 36.0f;
    int pageScope = 0;                      // 0 = All pages, 1 = Custom range
    std::wstring pageRange = L"1";
    int startPageNum = 1;
    int currentPage = 1;
    int totalPages = 1;
};

class AddHeaderFooterCommand : public core::interfaces::dom::ICommand {
public:
    AddHeaderFooterCommand(std::shared_ptr<core::interfaces::dom::IDocument> doc, const HeaderFooterParams& params);
    AddHeaderFooterCommand(core::interfaces::dom::IDocument* doc, const HeaderFooterParams& params);
    ~AddHeaderFooterCommand() override = default;

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Add Header & Footer"; }

private:
    core::interfaces::dom::IDocument* GetDoc() const;
    std::wstring FormatTokens(const std::wstring& templateStr, int pageNum, int totalPages) const;

    std::shared_ptr<core::interfaces::dom::IDocument> m_docShared;
    core::interfaces::dom::IDocument* m_docRaw = nullptr;
    HeaderFooterParams m_params;
    std::vector<std::pair<int, FPDF_PAGEOBJECT>> m_createdObjects; // (pageIndex, FPDF_PAGEOBJECT)
    bool m_applied = false;
};

} // namespace commands
} // namespace pdf_engine
