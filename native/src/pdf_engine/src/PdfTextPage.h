#pragma once
#include "core/interfaces/dom/ITextPage.h"
#include <fpdf_text.h>
#include <memory>
#include <mutex>

class PdfTextPage : public core::interfaces::dom::ITextPage {
public:
    PdfTextPage(FPDF_TEXTPAGE textPage, std::recursive_mutex* docMutex);
    ~PdfTextPage() override;

    PdfTextPage(const PdfTextPage&) = delete;
    PdfTextPage& operator=(const PdfTextPage&) = delete;

    int GetCharCount() const override;
    std::wstring GetText(int startCharIndex, int charCount) const override;
    RectF GetCharBox(int charIndex) const override;
    int GetCharIndexAtPos(double x, double y, double xTolerance, double yTolerance) const override;
    std::vector<RectF> GetRects(int startCharIndex, int charCount) const override;

private:
    FPDF_TEXTPAGE m_textPage = nullptr;
    std::recursive_mutex* m_docMutex = nullptr;
};
