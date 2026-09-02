#include "TextCommands.h"
#include "core/interfaces/dom/IPage.h"
#include "../PdfPage.h"

namespace pdf_engine {
namespace commands {

EditTextCommand::EditTextCommand(std::shared_ptr<core::interfaces::dom::ITextObject> textObj, const std::wstring& oldText, const std::wstring& newText)
    : m_textObj(textObj), m_oldText(oldText), m_newText(newText) {}

bool EditTextCommand::Execute() {
    return m_textObj->SetText(m_newText);
}

bool EditTextCommand::Undo() {
    return m_textObj->SetText(m_oldText);
}

MoveTextCommand::MoveTextCommand(std::shared_ptr<core::interfaces::dom::ITextObject> textObj, const RectF& oldBounds, const RectF& newBounds)
    : m_textObj(textObj), m_oldBounds(oldBounds), m_newBounds(newBounds) {}

bool MoveTextCommand::Execute() {
    // Basic move using transform translation
    float dx = m_newBounds.left - m_oldBounds.left;
    float dy = m_newBounds.top - m_oldBounds.top;
    auto mat = m_textObj->GetTransform();
    mat.e += dx;
    mat.f += dy;
    return m_textObj->SetTransform(mat);
}

bool MoveTextCommand::Undo() {
    float dx = m_oldBounds.left - m_newBounds.left;
    float dy = m_oldBounds.top - m_newBounds.top;
    auto mat = m_textObj->GetTransform();
    mat.e += dx;
    mat.f += dy;
    return m_textObj->SetTransform(mat);
}

DeleteTextCommand::DeleteTextCommand(core::interfaces::dom::IDocument* doc, int pageIndex, std::shared_ptr<core::interfaces::dom::ITextObject> textObj)
    : m_doc(doc), m_pageIndex(pageIndex), m_textObj(textObj), m_deleted(false) {}

bool DeleteTextCommand::Execute() {
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page) return false;
    
    if (!m_deleted && m_textObj) {
        m_cachedText = m_textObj->GetText();
        m_cachedBounds = m_textObj->GetBounds();
        m_cachedFontName = m_textObj->GetFontName();
        m_cachedFontSize = m_textObj->GetFontSize();
        m_textObj->GetColor(m_cachedR, m_cachedG, m_cachedB, m_cachedA);
    }
    
    m_deleted = page->RemoveTextObject(m_textObj);
    return m_deleted;
}

bool DeleteTextCommand::Undo() {
    if (!m_deleted) return false;
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page) return false;
    
    auto pdfPage = dynamic_cast<PdfPage*>(page.get());
    if (pdfPage && m_textObj) {
        m_deleted = !pdfPage->RestoreTextObject(m_textObj);
        return !m_deleted;
    }
    return false;
}

AddTextCommand::AddTextCommand(core::interfaces::dom::IDocument* doc, int pageIndex, const std::wstring& text, const RectF& bounds, const std::string& fontName, float fontSize, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    : m_doc(doc), m_pageIndex(pageIndex), m_text(text), m_bounds(bounds), m_fontName(fontName), m_fontSize(fontSize), m_r(r), m_g(g), m_b(b), m_a(a) {}

bool AddTextCommand::Execute() {
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page) return false;
    
    if (m_addedObj) {
        auto pdfPage = dynamic_cast<PdfPage*>(page.get());
        if (pdfPage) {
            return pdfPage->RestoreTextObject(m_addedObj);
        }
    }

    m_addedObj = page->InsertTextObject(m_text, m_bounds, m_fontName, m_fontSize);
    if (m_addedObj) {
        m_addedObj->SetColor(m_r, m_g, m_b, m_a);
        return true;
    }
    return false;
}

bool AddTextCommand::Undo() {
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page || !m_addedObj) return false;
    
    return page->RemoveTextObject(m_addedObj);
}

EditMultilineTextCommand::EditMultilineTextCommand(
    std::shared_ptr<core::interfaces::dom::ITextObject> textObj, 
    const std::vector<core::interfaces::dom::TextLineData>& oldLines,
    const std::vector<core::interfaces::dom::TextLineData>& newLines)
    : m_textObj(textObj), m_oldLines(oldLines), m_newLines(newLines) {}

bool EditMultilineTextCommand::Execute() {
    return m_textObj->SetLines(m_newLines);
}

bool EditMultilineTextCommand::Undo() {
    if (!m_textObj) return false;
    return m_textObj->SetLines(m_oldLines);
}

EditTextStyleCommand::EditTextStyleCommand(std::shared_ptr<core::interfaces::dom::ITextObject> textObj, 
                                           float oldSize, float newSize,
                                           uint8_t oldR, uint8_t oldG, uint8_t oldB, uint8_t oldA,
                                           uint8_t newR, uint8_t newG, uint8_t newB, uint8_t newA)
    : m_textObj(textObj), m_oldSize(oldSize), m_newSize(newSize),
      m_oldR(oldR), m_oldG(oldG), m_oldB(oldB), m_oldA(oldA),
      m_newR(newR), m_newG(newG), m_newB(newB), m_newA(newA) {}

bool EditTextStyleCommand::Execute() {
    if (!m_textObj) return false;
    bool s1 = m_textObj->SetFontSize(m_newSize);
    bool s2 = m_textObj->SetColor(m_newR, m_newG, m_newB, m_newA);
    return s1 || s2;
}

bool EditTextStyleCommand::Undo() {
    if (!m_textObj) return false;
    bool s1 = m_textObj->SetFontSize(m_oldSize);
    bool s2 = m_textObj->SetColor(m_oldR, m_oldG, m_oldB, m_oldA);
    return s1 || s2;
}

} // namespace commands
} // namespace pdf_engine
