#include "PdfTextPage.h"

// NOTE ON LOCKING:
// PDFium is not thread-safe. Every FPDF_* call below can run concurrently with
// the background RenderWorker (which renders tiles for the same document) and,
// during "find", with the SearchEngine worker thread. All of them must serialize
// on the owning document's recursive_mutex.
//
// The previous implementation wrote:
//     if (m_docMutex) std::lock_guard<std::recursive_mutex> lock(*m_docMutex);
// where the lock_guard is the *single controlled statement of the if*. That guard
// is constructed and immediately destructed on that same line, so the FPDF_* call
// on the following line ran completely UNLOCKED. It looked synchronized but was not.
//
// The fix uses a function-scoped std::unique_lock that stays held for the whole
// method (and is null-safe if no mutex was supplied).

PdfTextPage::PdfTextPage(FPDF_TEXTPAGE textPage, std::recursive_mutex* docMutex) : m_textPage(textPage), m_docMutex(docMutex) {
}

PdfTextPage::~PdfTextPage() {
    if (m_textPage) {
        std::unique_lock<std::recursive_mutex> lock;
        if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
        FPDFText_ClosePage(m_textPage);
    }
}

int PdfTextPage::GetCharCount() const {
    if (!m_textPage) return 0;
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    return FPDFText_CountChars(m_textPage);
}

std::wstring PdfTextPage::GetText(int startCharIndex, int charCount) const {
    if (!m_textPage || charCount <= 0) return L"";
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);

    // FPDFText_GetText requires buffer size in bytes, including null terminator.
    // Each UTF-16 character is 2 bytes.
    int bufferLength = charCount + 1;
    std::vector<unsigned short> buffer(bufferLength);

    int resultCount = FPDFText_GetText(m_textPage, startCharIndex, charCount, buffer.data());

    if (resultCount > 0) {
        // Result includes the null terminator, so the string length is resultCount - 1
        return std::wstring(reinterpret_cast<const wchar_t*>(buffer.data()), resultCount - 1);
    }
    return L"";
}

RectF PdfTextPage::GetCharBox(int charIndex) const {
    if (!m_textPage) return {0, 0, 0, 0};
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);

    double left = 0, top = 0, right = 0, bottom = 0;
    if (FPDFText_GetCharBox(m_textPage, charIndex, &left, &right, &bottom, &top)) {
        return { static_cast<float>(left), static_cast<float>(top), static_cast<float>(right), static_cast<float>(bottom) };
    }
    return {0, 0, 0, 0};
}

int PdfTextPage::GetCharIndexAtPos(double x, double y, double xTolerance, double yTolerance) const {
    if (!m_textPage) return -1;
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    return FPDFText_GetCharIndexAtPos(m_textPage, x, y, xTolerance, yTolerance);
}

std::vector<RectF> PdfTextPage::GetRects(int startCharIndex, int charCount) const {
    std::vector<RectF> rects;
    if (!m_textPage || charCount <= 0) return rects;
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);

    int rectCount = FPDFText_CountRects(m_textPage, startCharIndex, charCount);
    for (int i = 0; i < rectCount; ++i) {
        double left = 0, top = 0, right = 0, bottom = 0;
        if (FPDFText_GetRect(m_textPage, i, &left, &top, &right, &bottom)) {
            rects.push_back({ static_cast<float>(left), static_cast<float>(top), static_cast<float>(right), static_cast<float>(bottom) });
        }
    }
    return rects;
}
