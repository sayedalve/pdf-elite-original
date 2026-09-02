#include "ImageCommands.h"
#include "core/interfaces/dom/IPage.h"

namespace pdf_engine {
namespace commands {

// -------------------------------------------------------------------
// InsertImageCommand
// -------------------------------------------------------------------

InsertImageCommand::InsertImageCommand(core::interfaces::dom::IDocument* doc, int pageIndex, const std::vector<uint8_t>& imageData, int width, int height, const RectF& bounds)
    : m_doc(doc), m_pageIndex(pageIndex), m_imageData(imageData), m_width(width), m_height(height), m_bounds(bounds) {
}

bool InsertImageCommand::Execute() {
    if (!m_doc) return false;
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page) return false;

    if (!m_image) {
        m_image = page->InsertImageFromMemory(m_imageData, m_width, m_height, m_bounds);
    } else {
        // Redo scenario
        auto newImage = page->InsertImageFromMemory(m_imageData, m_width, m_height, m_image->GetBounds());
        m_image = newImage;
    }
    return m_image != nullptr;
}

bool InsertImageCommand::Undo() {
    if (!m_doc || !m_image) return false;
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page) return false;
    
    return page->RemoveImage(m_image);
}

// -------------------------------------------------------------------
// DeleteImageCommand
// -------------------------------------------------------------------

DeleteImageCommand::DeleteImageCommand(core::interfaces::dom::IDocument* doc, int pageIndex, std::shared_ptr<core::interfaces::dom::IImage> image)
    : m_doc(doc), m_pageIndex(pageIndex), m_image(image) {
}

bool DeleteImageCommand::Execute() {
    if (!m_doc || !m_image) return false;
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page) return false;

    if (!m_deleted) {
        m_savedWidth = m_image->GetWidth();
        m_savedHeight = m_image->GetHeight();
        m_savedBounds = m_image->GetBounds();
        m_savedData = m_image->GetBitmapData();
        m_deleted = true;
    }

    return page->RemoveImage(m_image);
}

bool DeleteImageCommand::Undo() {
    if (!m_doc || !m_deleted) return false;
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page) return false;

    auto restored = page->InsertImageFromMemory(m_savedData, m_savedWidth, m_savedHeight, m_savedBounds);
    if (restored) {
        m_image = restored;
        return true;
    }
    return false;
}

// -------------------------------------------------------------------
// MoveImageCommand
// -------------------------------------------------------------------

MoveImageCommand::MoveImageCommand(std::shared_ptr<core::interfaces::dom::IImage> image, const RectF& oldBounds, const RectF& newBounds)
    : m_image(image), m_oldBounds(oldBounds), m_newBounds(newBounds) {
}

bool MoveImageCommand::Execute() {
    if (!m_image) return false;
    m_image->SetBounds(m_newBounds);
    return true;
}

bool MoveImageCommand::Undo() {
    if (!m_image) return false;
    m_image->SetBounds(m_oldBounds);
    return true;
}

} // namespace commands
} // namespace pdf_engine
