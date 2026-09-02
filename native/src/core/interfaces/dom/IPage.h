#pragma once
#include "core/Geometry.h"
#include "core/Result.h"

#include <vector>
#include <cstdint>
#include <memory>

#include "core/interfaces/dom/IAnnotation.h"
#include "core/interfaces/dom/IImage.h"
#include "core/interfaces/dom/ITextObject.h"
#include "Navigation.h"


namespace core {
namespace interfaces {
namespace dom {
class ITextPage;


class IPage {
public:
    virtual ~IPage() = default;
    virtual SizeF GetSize() const = 0;
    virtual int GetRotation() const = 0;
    virtual void SetRotation(int degrees) = 0;
    virtual void GenerateContent() = 0;
    virtual void InvalidateTextIndex() = 0;
    
    // Links
    virtual std::vector<core::interfaces::dom::PdfLink> GetLinks() = 0;
    
    // Render a section of the page to a BGRA buffer.
    // zoom: scale factor (1.0 = 72 dpi)
    // startX, startY: unscaled pixel coordinates of the source rect
    // sizeX, sizeY: output dimensions
    virtual std::vector<uint8_t> RenderToBitmap(double zoom, int startX, int startY, int sizeX, int sizeY, bool darkMode = false) const = 0;

    // Render directly to a Windows HDC (passed as void* to avoid pulling in windows.h here)
    // Used for printing.
    virtual void RenderForPrint(void* hdc, int startX, int startY, int sizeX, int sizeY, int rotate) const = 0;

    virtual std::unique_ptr<ITextPage> LoadTextPage() = 0;

    // Annotation methods
    virtual std::vector<std::shared_ptr<core::interfaces::dom::IAnnotation>> GetAnnotations() = 0;
    virtual std::shared_ptr<core::interfaces::dom::IAnnotation> CreateAnnotation(core::interfaces::dom::AnnotationType type) = 0;
    virtual bool RemoveAnnotation(std::shared_ptr<core::interfaces::dom::IAnnotation> annot) = 0;

    // Image methods
    virtual std::vector<std::shared_ptr<core::interfaces::dom::IImage>> GetImages() = 0;
    virtual std::shared_ptr<core::interfaces::dom::IImage> InsertImage(const std::wstring& imagePath, const RectF& bounds) = 0;
    virtual std::shared_ptr<core::interfaces::dom::IImage> InsertImageFromMemory(const std::vector<uint8_t>& imageData, int width, int height, const RectF& bounds) = 0;
    virtual bool RemoveImage(std::shared_ptr<core::interfaces::dom::IImage> image) = 0;

    // Text Objects
    virtual std::vector<std::shared_ptr<core::interfaces::dom::ITextObject>> GetTextObjects() = 0;
    virtual std::shared_ptr<core::interfaces::dom::ITextObject> InsertTextObject(const std::wstring& text, const RectF& bounds, const std::string& fontName, float fontSize) = 0;
    virtual bool RemoveTextObject(std::shared_ptr<core::interfaces::dom::ITextObject> textObj) = 0;
};

} // namespace dom
} // namespace interfaces
} // namespace core


