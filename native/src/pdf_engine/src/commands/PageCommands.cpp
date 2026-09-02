#include "PageCommands.h"
#include <fpdf_edit.h>
#include <fpdf_transformpage.h>
#include <string>
#include <mutex>

namespace pdf_engine {
namespace commands {

// -------------------------------------------------------------------
// DeletePageCommand
// -------------------------------------------------------------------

DeletePageCommand::DeletePageCommand(PdfDocument* doc, int pageIndex)
    : m_doc(doc), m_pageIndex(pageIndex) {
}

DeletePageCommand::~DeletePageCommand() {
    if (m_backupDoc) {
        FPDF_CloseDocument(m_backupDoc);
    }
}

bool DeletePageCommand::Execute() {
    if (!m_doc) return false;
    // These FPDF_* calls operate directly on the shared main-document handle, so
    // they must serialize with the RenderWorker. DeletePage() re-locks the same
    // recursive_mutex (safe).
    std::lock_guard<std::recursive_mutex> lock(m_doc->GetMutex());
    auto handle = m_doc->GetHandle();

    // Backup the page before deleting it
    if (!m_backupDoc) {
        m_backupDoc = FPDF_CreateNewDocument();
        // Import the page to backup doc
        std::string pageRange = std::to_string(m_pageIndex + 1); // 1-based
        if (!FPDF_ImportPages(m_backupDoc, handle, pageRange.c_str(), 0)) {
            FPDF_CloseDocument(m_backupDoc);
            m_backupDoc = nullptr;
            return false;
        }
    }

    return m_doc->DeletePage(m_pageIndex);
}

bool DeletePageCommand::Undo() {
    if (!m_doc || !m_backupDoc) return false;

    // Restore the page from backupDoc
    std::vector<int> srcIndices = { 0 }; // The backup doc only has 1 page at index 0
    // We cannot use m_doc->InsertPagesFrom directly with core::interfaces::dom::IDocument, we need to call PDFium directly
    // Wait, m_doc->InsertPagesFrom takes core::interfaces::dom::IDocument*, which is tricky if we don't wrap m_backupDoc
    // We'll call PDFium directly since we are inside pdf_engine
    std::lock_guard<std::recursive_mutex> lock(m_doc->GetMutex());
    auto handle = m_doc->GetHandle();
    std::string pageRange = "1";
    if (FPDF_ImportPages(handle, m_backupDoc, pageRange.c_str(), m_pageIndex)) {
        return true;
    }
    return false;
}

// -------------------------------------------------------------------
// InsertBlankPageCommand
// -------------------------------------------------------------------

InsertBlankPageCommand::InsertBlankPageCommand(PdfDocument* doc, int pageIndex, double width, double height)
    : m_doc(doc), m_pageIndex(pageIndex), m_width(width), m_height(height) {
}

bool InsertBlankPageCommand::Execute() {
    if (!m_doc) return false;
    return m_doc->InsertBlankPage(m_pageIndex, m_width, m_height);
}

bool InsertBlankPageCommand::Undo() {
    if (!m_doc) return false;
    return m_doc->DeletePage(m_pageIndex);
}

// -------------------------------------------------------------------
// RotatePageCommand
// -------------------------------------------------------------------

RotatePageCommand::RotatePageCommand(PdfDocument* doc, int pageIndex, int rotationDegrees)
    : m_doc(doc), m_pageIndex(pageIndex), m_rotationDegrees(rotationDegrees) {
}

bool RotatePageCommand::Execute() {
    if (!m_doc) return false;
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page) return false;
    
    m_oldRotation = page->GetRotation();
    return m_doc->RotatePage(m_pageIndex, m_oldRotation + m_rotationDegrees);
}

bool RotatePageCommand::Undo() {
    if (!m_doc) return false;
    return m_doc->RotatePage(m_pageIndex, m_oldRotation);
}

// -------------------------------------------------------------------
// MovePageCommand
// -------------------------------------------------------------------

MovePageCommand::MovePageCommand(PdfDocument* doc, int sourceIndex, int destIndex)
    : m_doc(doc), m_sourceIndex(sourceIndex), m_destIndex(destIndex) {
}

bool MovePageCommand::Execute() {
    if (!m_doc) return false;
    return m_doc->MovePage(m_sourceIndex, m_destIndex);
}

bool MovePageCommand::Undo() {
    if (!m_doc) return false;
    
    // When a page moves from src to dest:
    // If src < dest, the page ends up at dest-1 after the move because pages shift down.
    // Wait, the FPDF_MovePages documentation typically says destIndex is the new location.
    // If src = 1, dest = 5. Page at 1 goes to index 5.
    // The previous pages 2, 3, 4, 5 shift up to 1, 2, 3, 4.
    // To undo, we move from the new index (dest-1 if dest > src, else dest) back to src.
    
    int newIndex = m_destIndex;
    if (m_sourceIndex < m_destIndex) {
        newIndex = m_destIndex - 1; 
    }
    
    int undoDest = m_sourceIndex;
    if (newIndex < m_sourceIndex) {
        undoDest = m_sourceIndex + 1;
    }
    
    return m_doc->MovePage(newIndex, undoDest);
}

// -------------------------------------------------------------------
// CropPageCommand
// -------------------------------------------------------------------

CropPageCommand::CropPageCommand(PdfDocument* doc, int pageIndex, float left, float top, float right, float bottom)
    : m_doc(doc), m_pageIndex(pageIndex), m_left(left), m_top(top), m_right(right), m_bottom(bottom),
      m_oldLeft(0), m_oldTop(0), m_oldRight(0), m_oldBottom(0) {
}

bool CropPageCommand::Execute() {
    if (!m_doc) return false;
    std::lock_guard<std::recursive_mutex> lock(m_doc->GetMutex());
    
    // We need FPDF_PAGE to call fpdf_transformpage.h methods
    // We can just use the handle
    auto pageHandle = FPDF_LoadPage(m_doc->GetHandle(), m_pageIndex);
    if (!pageHandle) return false;
    
    // Save old crop box
    float oldB, oldT; // Note: bottom and top
    if (!FPDFPage_GetCropBox(pageHandle, &m_oldLeft, &oldB, &m_oldRight, &oldT)) {
        // If no crop box, get media box
        if (!FPDFPage_GetMediaBox(pageHandle, &m_oldLeft, &oldB, &m_oldRight, &oldT)) {
            m_oldLeft = 0; oldB = 0; m_oldRight = 0; oldT = 0;
        }
    }
    m_oldTop = oldT;
    m_oldBottom = oldB;
    
    // Set new crop box
    FPDFPage_SetCropBox(pageHandle, m_left, m_bottom, m_right, m_top);
    FPDF_ClosePage(pageHandle);
    return true;
}

bool CropPageCommand::Undo() {
    if (!m_doc) return false;
    std::lock_guard<std::recursive_mutex> lock(m_doc->GetMutex());
    
    auto pageHandle = FPDF_LoadPage(m_doc->GetHandle(), m_pageIndex);
    if (!pageHandle) return false;
    
    FPDFPage_SetCropBox(pageHandle, m_oldLeft, m_oldBottom, m_oldRight, m_oldTop);
    FPDF_ClosePage(pageHandle);
    return true;
}

// -------------------------------------------------------------------
// SetPageSizeCommand
// -------------------------------------------------------------------

SetPageSizeCommand::SetPageSizeCommand(PdfDocument* doc, int pageIndex, float width, float height)
    : m_doc(doc), m_pageIndex(pageIndex), m_width(width), m_height(height),
      m_oldWidth(0), m_oldHeight(0) {
}

bool SetPageSizeCommand::Execute() {
    if (!m_doc) return false;
    std::lock_guard<std::recursive_mutex> lock(m_doc->GetMutex());
    
    auto pageHandle = FPDF_LoadPage(m_doc->GetHandle(), m_pageIndex);
    if (!pageHandle) return false;
    
    float oldL, oldB, oldR, oldT;
    if (FPDFPage_GetMediaBox(pageHandle, &oldL, &oldB, &oldR, &oldT)) {
        m_oldWidth = oldR - oldL;
        m_oldHeight = oldT - oldB;
    } else {
        m_oldWidth = static_cast<float>(FPDF_GetPageWidth(pageHandle));
        m_oldHeight = static_cast<float>(FPDF_GetPageHeight(pageHandle));
    }
    
    // Set MediaBox. Assume starting from (0,0) or adjusting existing bounds
    // A simple way is to set MediaBox to 0, 0, width, height
    FPDFPage_SetMediaBox(pageHandle, 0, 0, m_width, m_height);
    FPDF_ClosePage(pageHandle);
    return true;
}

bool SetPageSizeCommand::Undo() {
    if (!m_doc) return false;
    std::lock_guard<std::recursive_mutex> lock(m_doc->GetMutex());
    
    auto pageHandle = FPDF_LoadPage(m_doc->GetHandle(), m_pageIndex);
    if (!pageHandle) return false;
    
    FPDFPage_SetMediaBox(pageHandle, 0, 0, m_oldWidth, m_oldHeight);
    FPDF_ClosePage(pageHandle);
    return true;
}

} // namespace commands
} // namespace pdf_engine


