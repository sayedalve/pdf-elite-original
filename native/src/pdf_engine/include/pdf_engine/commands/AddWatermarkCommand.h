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

struct WatermarkParams {
    std::wstring text = L"CONFIDENTIAL";
    std::wstring fontName = L"Helvetica";
    float fontSize = 48.0f;
    bool bold = false;
    bool italic = false;
    COLORREF color = RGB(192, 192, 192);
    float opacity = 0.5f;                   // 0.0 to 1.0
    float rotation = 45.0f;                 // rotation in degrees
    int positionIndex = 0;                  // 0=Center, 1=Top-Left, 2=Top-Center, 3=Top-Right, 4=Bottom-Left, 5=Bottom-Center, 6=Bottom-Right
    bool layerOver = true;                  // true = over page content, false = under content (background)
    int pageScope = 0;                      // 0 = All pages, 1 = Current page, 2 = Custom range
    std::wstring pageRange = L"1";
    int currentPage = 1;
    int totalPages = 1;
};

class AddWatermarkCommand : public core::interfaces::dom::ICommand {
public:
    AddWatermarkCommand(std::shared_ptr<core::interfaces::dom::IDocument> doc, const WatermarkParams& params);
    AddWatermarkCommand(core::interfaces::dom::IDocument* doc, const WatermarkParams& params);
    ~AddWatermarkCommand() override = default;

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Add Watermark"; }

private:
    core::interfaces::dom::IDocument* GetDoc() const;

    std::shared_ptr<core::interfaces::dom::IDocument> m_docShared;
    core::interfaces::dom::IDocument* m_docRaw = nullptr;
    WatermarkParams m_params;
    std::map<int, FPDF_PAGEOBJECT> m_createdObjects; // pageIndex -> FPDF_PAGEOBJECT
    bool m_applied = false;
};

} // namespace commands
} // namespace pdf_engine
