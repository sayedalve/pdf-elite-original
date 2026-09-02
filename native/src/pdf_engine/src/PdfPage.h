#pragma once
#include "core/interfaces/dom/IPage.h"
#include "core/interfaces/dom/IAnnotation.h"
#include "core/interfaces/dom/ITextObject.h"
#include <fpdfview.h>
#include <mutex>
#include <vector>

class PdfDocument;

class PdfPage : public core::interfaces::dom::IPage {
public:
    PdfPage(PdfDocument* parent, FPDF_DOCUMENT doc, FPDF_PAGE page, int pageIndex);
    ~PdfPage() override;

    PdfPage(const PdfPage&) = delete;
    PdfPage& operator=(const PdfPage&) = delete;

    SizeF GetSize() const override;
    int GetRotation() const override;
    void SetRotation(int degrees) override;
    void GenerateContent() override;
    std::vector<uint8_t> RenderToBitmap(double zoom, int startX, int startY, int sizeX, int sizeY, bool darkMode = false) const override;
    void RenderForPrint(void* hdc, int startX, int startY, int sizeX, int sizeY, int rotate) const override;
    
    void InvalidateTextIndex() override;
    std::unique_ptr<core::interfaces::dom::ITextPage> LoadTextPage() override;
    
    // Links
    std::vector<core::interfaces::dom::PdfLink> GetLinks() override;

    // Annotations
    std::vector<std::shared_ptr<core::interfaces::dom::IAnnotation>> GetAnnotations() override;
    std::shared_ptr<core::interfaces::dom::IAnnotation> CreateAnnotation(core::interfaces::dom::AnnotationType type) override;
    bool RemoveAnnotation(std::shared_ptr<core::interfaces::dom::IAnnotation> annot) override;

    // Images
    std::vector<std::shared_ptr<core::interfaces::dom::IImage>> GetImages() override;
    std::shared_ptr<core::interfaces::dom::IImage> InsertImage(const std::wstring& imagePath, const RectF& bounds) override;
    std::shared_ptr<core::interfaces::dom::IImage> InsertImageFromMemory(const std::vector<uint8_t>& imageData, int width, int height, const RectF& bounds) override;
    bool RemoveImage(std::shared_ptr<core::interfaces::dom::IImage> image) override;

    // Text Objects
    std::vector<std::shared_ptr<core::interfaces::dom::ITextObject>> GetTextObjects() override;
    std::shared_ptr<core::interfaces::dom::ITextObject> InsertTextObject(const std::wstring& text, const RectF& bounds, const std::string& fontName, float fontSize) override;
    bool RemoveTextObject(std::shared_ptr<core::interfaces::dom::ITextObject> textObj) override;
    bool RestoreTextObject(std::shared_ptr<core::interfaces::dom::ITextObject> textObj);

    FPDF_PAGE GetHandle() const { return m_page; }

    // Returns the owning document's recursive_mutex so sub-objects that only hold
    // a PdfPage* (PdfAnnotation, PdfImage) can serialize their FPDF_* calls against
    // the RenderWorker. Defined in PdfPage.cpp (needs the full PdfDocument type).
    std::recursive_mutex& GetDocMutex() const;

private:
    PdfDocument* m_parentDoc;
    int m_pageIndex;
    FPDF_DOCUMENT m_doc;
    FPDF_PAGE m_page;
    mutable std::mutex m_mutex;
    std::vector<std::shared_ptr<core::interfaces::dom::IAnnotation>> m_annotations;
    std::vector<std::shared_ptr<core::interfaces::dom::IImage>> m_images;
};


