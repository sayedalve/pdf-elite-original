#include "PdfImage.h"
#include "PdfPage.h"
#include <sstream>
#include <mutex>

// PDFium is not thread-safe. FPDF* image calls here can race the background
// RenderWorker rendering the same page, so each method serializes on the owning
// document's recursive_mutex (reached via the parent PdfPage).

namespace pdf_engine {

PdfImage::PdfImage(FPDF_PAGEOBJECT imageObj, PdfPage* page)
    : m_imageObj(imageObj),
      m_pageHandle(page ? page->GetHandle() : nullptr),
      m_docMutex(page ? &page->GetDocMutex() : nullptr) {
}

PdfImage::~PdfImage() {
}

std::string PdfImage::GetId() const {
    std::stringstream ss;
    ss << "img_" << (void*)m_pageHandle << "_" << (void*)m_imageObj;
    return ss.str();
}

RectF PdfImage::GetBounds() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    float left = 0, bottom = 0, right = 0, top = 0;
    FPDFPageObj_GetBounds(m_imageObj, &left, &bottom, &right, &top);
    return { left, top, right, bottom };
}

void PdfImage::SetBounds(const RectF& bounds) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    // In PDFium, an image's bounds are determined by its transformation matrix.
    // The base image is 1x1. The matrix scales it to the desired width and height, and translates it.
    // A simple transform matrix:
    // a = width, b = 0
    // c = 0, d = height
    // e = left, f = bottom
    // We assume the image is upright (no rotation) when using SetBounds directly.
    double width = bounds.right - bounds.left;
    double height = bounds.top - bounds.bottom; // PDF top > bottom
    if (height < 0) height = -height;
    
    // Actually PDF coordinates have Y going UP.
    // In our RectF, `top` is typically larger than `bottom` in PDF coords.
    // Or if our UI uses top < bottom, we need to map correctly. 
    // Assuming UI already passed PDF coordinates (where Y goes up, top > bottom):
    double a = width;
    double b = 0;
    double c = 0;
    double d = bounds.top - bounds.bottom; // positive height
    double e = bounds.left;
    double f = bounds.bottom;

    FPDFPageObj_Transform(m_imageObj, 1, 0, 0, 1, 0, 0); // Need to reset transform?
    // Actually, FPDFPageObj_SetMatrix overwrites the matrix.
    FS_MATRIX matrix = { (float)a, (float)b, (float)c, (float)d, (float)e, (float)f };
    FPDFPageObj_SetMatrix(m_imageObj, &matrix);
}

Matrix3x2F PdfImage::GetTransform() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    FS_MATRIX matrix;
    if (FPDFPageObj_GetMatrix(m_imageObj, &matrix)) {
        return Matrix3x2F{ matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f };
    }
    return Matrix3x2F{1, 0, 0, 1, 0, 0};
}

void PdfImage::SetTransform(const Matrix3x2F& mat) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    FS_MATRIX matrix = { mat.a, mat.b, mat.c, mat.d, mat.e, mat.f };
    FPDFPageObj_SetMatrix(m_imageObj, &matrix);
}

int PdfImage::GetWidth() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    FPDF_BITMAP bitmap = FPDFImageObj_GetBitmap(m_imageObj);
    if (bitmap) {
        int w = FPDFBitmap_GetWidth(bitmap);
        FPDFBitmap_Destroy(bitmap);
        return w;
    }
    return 0;
}

int PdfImage::GetHeight() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    FPDF_BITMAP bitmap = FPDFImageObj_GetBitmap(m_imageObj);
    if (bitmap) {
        int h = FPDFBitmap_GetHeight(bitmap);
        FPDFBitmap_Destroy(bitmap);
        return h;
    }
    return 0;
}

std::vector<uint8_t> PdfImage::GetBitmapData() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    std::vector<uint8_t> data;
    FPDF_BITMAP bitmap = FPDFImageObj_GetBitmap(m_imageObj);
    if (bitmap) {
        int stride = FPDFBitmap_GetStride(bitmap);
        int h = FPDFBitmap_GetHeight(bitmap);
        const uint8_t* buffer = static_cast<const uint8_t*>(FPDFBitmap_GetBuffer(bitmap));
        if (buffer && stride > 0 && h > 0) {
            data.assign(buffer, buffer + (stride * h));
        }
        FPDFBitmap_Destroy(bitmap);
    }
    return data;
}

} // namespace pdf_engine
