#pragma once
#include "core/interfaces/dom/ICommand.h"
#include "core/interfaces/dom/IImage.h"
#include "core/interfaces/dom/IDocument.h"
#include <memory>
#include <string>

namespace pdf_engine {
namespace commands {

class InsertImageCommand : public core::interfaces::dom::ICommand {
public:
    InsertImageCommand(core::interfaces::dom::IDocument* doc, int pageIndex, const std::vector<uint8_t>& imageData, int width, int height, const RectF& bounds);

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Insert Image"; }
    
    std::shared_ptr<core::interfaces::dom::IImage> GetImage() const { return m_image; }

private:
    core::interfaces::dom::IDocument* m_doc;
    int m_pageIndex;
    std::vector<uint8_t> m_imageData;
    int m_width;
    int m_height;
    RectF m_bounds;
    
    std::shared_ptr<core::interfaces::dom::IImage> m_image;
};

class DeleteImageCommand : public core::interfaces::dom::ICommand {
public:
    DeleteImageCommand(core::interfaces::dom::IDocument* doc, int pageIndex, std::shared_ptr<core::interfaces::dom::IImage> image);

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Delete Image"; }

private:
    core::interfaces::dom::IDocument* m_doc;
    int m_pageIndex;
    std::shared_ptr<core::interfaces::dom::IImage> m_image;
    
    std::vector<uint8_t> m_savedData;
    int m_savedWidth;
    int m_savedHeight;
    RectF m_savedBounds;
    bool m_deleted = false;
};

class MoveImageCommand : public core::interfaces::dom::ICommand {
public:
    MoveImageCommand(std::shared_ptr<core::interfaces::dom::IImage> image, const RectF& oldBounds, const RectF& newBounds);

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Move/Resize Image"; }

private:
    std::shared_ptr<core::interfaces::dom::IImage> m_image;
    RectF m_oldBounds;
    RectF m_newBounds;
};

} // namespace commands
} // namespace pdf_engine
