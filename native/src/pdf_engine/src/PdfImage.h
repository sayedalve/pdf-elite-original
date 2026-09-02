#pragma once
#include "core/interfaces/dom/IImage.h"
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <mutex>

class PdfPage;

namespace pdf_engine {

class PdfImage : public core::interfaces::dom::IImage {
public:
    PdfImage(FPDF_PAGEOBJECT imageObj, PdfPage* page);
    ~PdfImage() override;

    std::string GetId() const override;

    RectF GetBounds() const override;
    void SetBounds(const RectF& bounds) override;

    Matrix3x2F GetTransform() const override;
    void SetTransform(const Matrix3x2F& matrix) override;

    int GetWidth() const override;
    int GetHeight() const override;

    std::vector<uint8_t> GetBitmapData() const override;

    FPDF_PAGEOBJECT GetHandle() const { return m_imageObj; }

private:
    FPDF_PAGEOBJECT m_imageObj;
    FPDF_PAGE m_pageHandle;
    std::recursive_mutex* m_docMutex;
};

} // namespace pdf_engine
