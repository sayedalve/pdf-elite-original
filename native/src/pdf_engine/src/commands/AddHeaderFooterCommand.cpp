#include "pdf_engine/commands/AddHeaderFooterCommand.h"
#include "CommandUtils.h"
#include "PdfDocument.h"
#include "PdfPage.h"
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <fpdf_text.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace pdf_engine {
namespace commands {

AddHeaderFooterCommand::AddHeaderFooterCommand(std::shared_ptr<core::interfaces::dom::IDocument> doc, const HeaderFooterParams& params)
    : m_docShared(doc), m_docRaw(nullptr), m_params(params) {
}

AddHeaderFooterCommand::AddHeaderFooterCommand(core::interfaces::dom::IDocument* doc, const HeaderFooterParams& params)
    : m_docShared(nullptr), m_docRaw(doc), m_params(params) {
}

core::interfaces::dom::IDocument* AddHeaderFooterCommand::GetDoc() const {
    return m_docShared ? m_docShared.get() : m_docRaw;
}

std::wstring AddHeaderFooterCommand::FormatTokens(const std::wstring& templateStr, int pageNum, int totalPages) const {
    if (templateStr.empty()) return L"";

    std::wstring result = templateStr;

    auto replaceAll = [](std::wstring& str, const std::wstring& from, const std::wstring& to) {
        if (from.empty()) return;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::wstring::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    };

    // Replace {page} and variations
    std::wstring pageStr = std::to_wstring(pageNum);
    replaceAll(result, L"{page}", pageStr);
    replaceAll(result, L"{Page}", pageStr);
    replaceAll(result, L"{PAGE}", pageStr);

    // Replace {total} / {pages} and variations
    std::wstring totalStr = std::to_wstring(totalPages);
    replaceAll(result, L"{total}", totalStr);
    replaceAll(result, L"{Total}", totalStr);
    replaceAll(result, L"{TOTAL}", totalStr);
    replaceAll(result, L"{pages}", totalStr);
    replaceAll(result, L"{Pages}", totalStr);
    replaceAll(result, L"{PAGES}", totalStr);

    // Replace {date} with current date
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_s(&tm_now, &now_time);
    std::wstringstream dateSs;
    dateSs << std::put_time(&tm_now, L"%Y-%m-%d");
    std::wstring dateStr = dateSs.str();
    replaceAll(result, L"{date}", dateStr);
    replaceAll(result, L"{Date}", dateStr);
    replaceAll(result, L"{DATE}", dateStr);

    return result;
}

bool AddHeaderFooterCommand::Execute() {
    auto doc = GetDoc();
    if (!doc) return false;

    PdfDocument* pdfDoc = dynamic_cast<PdfDocument*>(doc);
    std::unique_lock<std::recursive_mutex> lock;
    if (pdfDoc) {
        lock = std::unique_lock<std::recursive_mutex>(pdfDoc->GetMutex());
    }

    int totalPages = doc->PageCount();
    if (totalPages <= 0) return false;

    std::vector<int> targetPages = ParsePageRange(m_params.pageScope, m_params.pageRange, m_params.currentPage, totalPages);
    if (targetPages.empty()) return false;

    std::string fontName = "Helvetica";
    if (!m_params.fontName.empty()) {
        fontName = WideToNarrow(m_params.fontName);
    }

    uint8_t r = GetRValue(m_params.color);
    uint8_t g = GetGValue(m_params.color);
    uint8_t b = GetBValue(m_params.color);

    m_createdObjects.clear();

    for (int pageIdx : targetPages) {
        auto page = doc->GetPage(pageIdx);
        if (!page) continue;

        PdfPage* pdfPage = dynamic_cast<PdfPage*>(page.get());
        if (!pdfPage) continue;

        FPDF_PAGE pageHandle = pdfPage->GetHandle();
        double pageWidth = FPDF_GetPageWidth(pageHandle);
        double pageHeight = FPDF_GetPageHeight(pageHandle);
        FPDF_DOCUMENT docHandle = pdfDoc ? pdfDoc->GetHandle() : nullptr;

        int pageNum = m_params.startPageNum + pageIdx;

        struct Slot {
            std::wstring text;
            float x;
            float y;
        };

        std::vector<Slot> slots;

        // Headers
        float headerY = static_cast<float>(pageHeight) - m_params.topMargin - m_params.fontSize;
        if (!m_params.leftHeader.empty()) {
            std::wstring t = FormatTokens(m_params.leftHeader, pageNum, totalPages);
            slots.push_back({ t, m_params.leftMargin, headerY });
        }
        if (!m_params.centerHeader.empty()) {
            std::wstring t = FormatTokens(m_params.centerHeader, pageNum, totalPages);
            float approxW = static_cast<float>(t.length()) * m_params.fontSize * 0.5f;
            float x = (static_cast<float>(pageWidth) - approxW) / 2.0f;
            slots.push_back({ t, x, headerY });
        }
        if (!m_params.rightHeader.empty()) {
            std::wstring t = FormatTokens(m_params.rightHeader, pageNum, totalPages);
            float approxW = static_cast<float>(t.length()) * m_params.fontSize * 0.5f;
            float x = static_cast<float>(pageWidth) - m_params.rightMargin - approxW;
            slots.push_back({ t, x, headerY });
        }

        // Footers
        float footerY = m_params.bottomMargin;
        if (!m_params.leftFooter.empty()) {
            std::wstring t = FormatTokens(m_params.leftFooter, pageNum, totalPages);
            slots.push_back({ t, m_params.leftMargin, footerY });
        }
        if (!m_params.centerFooter.empty()) {
            std::wstring t = FormatTokens(m_params.centerFooter, pageNum, totalPages);
            float approxW = static_cast<float>(t.length()) * m_params.fontSize * 0.5f;
            float x = (static_cast<float>(pageWidth) - approxW) / 2.0f;
            slots.push_back({ t, x, footerY });
        }
        if (!m_params.rightFooter.empty()) {
            std::wstring t = FormatTokens(m_params.rightFooter, pageNum, totalPages);
            float approxW = static_cast<float>(t.length()) * m_params.fontSize * 0.5f;
            float x = static_cast<float>(pageWidth) - m_params.rightMargin - approxW;
            slots.push_back({ t, x, footerY });
        }

        for (const auto& slot : slots) {
            if (slot.text.empty()) continue;

            FPDF_PAGEOBJECT textObj = FPDFPageObj_NewTextObj(docHandle, fontName.c_str(), m_params.fontSize);
            if (!textObj) continue;

            FPDFText_SetText(textObj, reinterpret_cast<FPDF_WIDESTRING>(slot.text.c_str()));
            FPDFPageObj_SetFillColor(textObj, r, g, b, 255);

            FS_MATRIX mat = { 1.0f, 0.0f, 0.0f, 1.0f, slot.x, slot.y };
            FPDFPageObj_SetMatrix(textObj, &mat);

            FPDFPage_InsertObject(pageHandle, textObj);
            m_createdObjects.push_back({ pageIdx, textObj });
        }

        FPDFPage_GenerateContent(pageHandle);
        page->InvalidateTextIndex();
    }

    m_applied = !m_createdObjects.empty();
    return m_applied;
}

bool AddHeaderFooterCommand::Undo() {
    auto doc = GetDoc();
    if (!doc || m_createdObjects.empty()) return false;

    PdfDocument* pdfDoc = dynamic_cast<PdfDocument*>(doc);
    std::unique_lock<std::recursive_mutex> lock;
    if (pdfDoc) {
        lock = std::unique_lock<std::recursive_mutex>(pdfDoc->GetMutex());
    }

    for (const auto& [pageIdx, obj] : m_createdObjects) {
        auto page = doc->GetPage(pageIdx);
        if (!page) continue;

        PdfPage* pdfPage = dynamic_cast<PdfPage*>(page.get());
        if (!pdfPage) continue;

        FPDF_PAGE pageHandle = pdfPage->GetHandle();
        FPDFPage_RemoveObject(pageHandle, obj);
        FPDFPageObj_Destroy(obj);
        FPDFPage_GenerateContent(pageHandle);
        page->InvalidateTextIndex();
    }

    m_createdObjects.clear();
    m_applied = false;
    return true;
}

} // namespace commands
} // namespace pdf_engine
