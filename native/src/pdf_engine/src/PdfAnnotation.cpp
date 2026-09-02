#include "PdfAnnotation.h"
#include "PdfPage.h"
#include <fpdf_edit.h>
#include "pdf_engine/LineAnnotationAdapter.h"
#include <windows.h>
#include <objbase.h> // For CoCreateGuid
#include <cmath>

// PDFium is not thread-safe. FPDFAnnot_*/FPDFPage_* calls here can race the
// background RenderWorker rendering the same page, so every method serializes on
// the owning document's recursive_mutex (reached via m_docMutex).

PdfAnnotation::PdfAnnotation(FPDF_ANNOTATION annot, PdfPage* page)
    : m_annot(annot), m_page(page), m_docMutex(page ? &page->GetDocMutex() : nullptr) {
    
    // Generate a temporary ID or extract from PDF if exists
    GUID guid;
    if (SUCCEEDED(CoCreateGuid(&guid))) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X",
                 guid.Data1, guid.Data2, guid.Data3,
                 guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                 guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
        m_id = buf;
    } else {
        m_id = "annot-unknown";
    }
}

PdfAnnotation::~PdfAnnotation() {
    if (m_annot) {
        std::unique_lock<std::recursive_mutex> lock;
        if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
        FPDFPage_CloseAnnot(m_annot);
        m_annot = nullptr;
    }
}

std::string PdfAnnotation::GetId() const {
    return m_id;
}

core::interfaces::dom::AnnotationType PdfAnnotation::GetType() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    int type = FPDFAnnot_GetSubtype(m_annot);
    return static_cast<core::interfaces::dom::AnnotationType>(type);
}

RectF PdfAnnotation::GetBounds() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    FS_RECTF rect;
    if (FPDFAnnot_GetRect(m_annot, &rect)) {
        return { rect.left, rect.top, rect.right, rect.bottom };
    }
    return {0, 0, 0, 0};
}

void PdfAnnotation::SetBounds(const RectF& bounds) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    FS_RECTF rect = { bounds.left, bounds.top, bounds.right, bounds.bottom };;
    FPDFAnnot_SetRect(m_annot, &rect);
}

std::string PdfAnnotation::GetContents() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    unsigned long len = FPDFAnnot_GetStringValue(m_annot, "Contents", nullptr, 0);
    if (len > 0) {
        std::vector<wchar_t> buf(len / 2 + 1);
        FPDFAnnot_GetStringValue(m_annot, "Contents", reinterpret_cast<FPDF_WCHAR*>(buf.data()), len);
        
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, buf.data(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Len > 0) {
            std::string utf8(utf8Len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, buf.data(), -1, utf8.data(), utf8Len, nullptr, nullptr);
            return utf8;
        }
    }
    return "";
}

void PdfAnnotation::SetContents(const std::string& contents) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    if (contents.empty()) {
        FPDFAnnot_SetStringValue(m_annot, "Contents", reinterpret_cast<FPDF_WIDESTRING>(L""));
        return;
    };
    
    int utf16Len = MultiByteToWideChar(CP_UTF8, 0, contents.c_str(), -1, nullptr, 0);
    if (utf16Len > 0) {
        std::vector<wchar_t> utf16(utf16Len);
        MultiByteToWideChar(CP_UTF8, 0, contents.c_str(), -1, utf16.data(), utf16Len);
        FPDFAnnot_SetStringValue(m_annot, "Contents", reinterpret_cast<FPDF_WIDESTRING>(utf16.data()));
    }
}

std::vector<QuadF> PdfAnnotation::GetQuadPoints() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    std::vector<QuadF> result;
    size_t count = FPDFAnnot_CountAttachmentPoints(m_annot);
    for (size_t i = 0; i < count; ++i) {
        FS_QUADPOINTSF quad;
        if (FPDFAnnot_GetAttachmentPoints(m_annot, i, &quad)) {
            result.push_back({
                {quad.x1, quad.y1},
                {quad.x2, quad.y2},
                {quad.x3, quad.y3},
                {quad.x4, quad.y4}
            });
        }
    }
    return result;
}

void PdfAnnotation::SetQuadPoints(const std::vector<QuadF>& quads) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    size_t existingCount = FPDFAnnot_CountAttachmentPoints(m_annot);
    for (size_t i = 0; i < quads.size(); ++i) {
        FS_QUADPOINTSF q = { quads[i].p1.x, quads[i].p1.y, quads[i].p2.x, quads[i].p2.y, quads[i].p3.x, quads[i].p3.y, quads[i].p4.x, quads[i].p4.y };;
        if (i < existingCount) {
            FPDFAnnot_SetAttachmentPoints(m_annot, i, &q);
        } else {
            FPDFAnnot_AppendAttachmentPoints(m_annot, &q);
        }
    }
}

void PdfAnnotation::SetColor(int r, int g, int b, int a) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    m_opacity = static_cast<float>(a) / 255.0f;
    m_hasOpacity = true;
    FPDFAnnot_SetColor(m_annot, FPDFANNOT_COLORTYPE_Color, r, g, b, a);
}


bool PdfAnnotation::GetColor(int& r, int& g, int& b, int& a) const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    unsigned int rOut, gOut, bOut, aOut;
    if (FPDFAnnot_GetColor(m_annot, FPDFANNOT_COLORTYPE_Color, &rOut, &gOut, &bOut, &aOut)) {
        r = static_cast<int>(rOut);
        g = static_cast<int>(gOut);
        b = static_cast<int>(bOut);
        a = m_hasOpacity ? static_cast<int>(std::round(m_opacity * 255.0f)) : static_cast<int>(aOut);
        return true;
    }
    return false;
}

void PdfAnnotation::SetFillColor(int r, int g, int b, int a) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    FPDFAnnot_SetColor(m_annot, FPDFANNOT_COLORTYPE_InteriorColor, r, g, b, a);
}


bool PdfAnnotation::GetFillColor(int& r, int& g, int& b, int& a) const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    unsigned int rOut, gOut, bOut, aOut;
    if (FPDFAnnot_GetColor(m_annot, FPDFANNOT_COLORTYPE_InteriorColor, &rOut, &gOut, &bOut, &aOut)) {
        r = static_cast<int>(rOut);
        g = static_cast<int>(gOut);
        b = static_cast<int>(bOut);
        a = static_cast<int>(aOut);
        return true;
    }
    return false;
}

void PdfAnnotation::SetBorderWidth(float width) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    FPDFAnnot_SetBorder(m_annot, width, 0, 0);
}


float PdfAnnotation::GetBorderWidth() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    float width = 0.0f;
    float hRadius = 0.0f;
    float vRadius = 0.0f;
    if (FPDFAnnot_GetBorder(m_annot, &width, &hRadius, &vRadius)) {
        return width;
    }
    return 1.0f;
}

float PdfAnnotation::GetOpacity() const {
    if (m_hasOpacity) {
        return m_opacity;
    }
    int r, g, b, a;
    if (GetColor(r, g, b, a)) {
        return static_cast<float>(a) / 255.0f;
    }
    return 1.0f;
}

void PdfAnnotation::SetOpacity(float opacity) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    m_opacity = opacity;
    m_hasOpacity = true;
    int r = 0, g = 0, b = 0;
    unsigned int rOut, gOut, bOut, aOut;
    if (FPDFAnnot_GetColor(m_annot, FPDFANNOT_COLORTYPE_Color, &rOut, &gOut, &bOut, &aOut)) {
        r = static_cast<int>(rOut);
        g = static_cast<int>(gOut);
        b = static_cast<int>(bOut);
    };
    FPDFAnnot_SetColor(m_annot, FPDFANNOT_COLORTYPE_Color, r, g, b, static_cast<int>(std::round(opacity * 255.0f)));
}

std::string PdfAnnotation::GetAuthor() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    unsigned long len = FPDFAnnot_GetStringValue(m_annot, "T", nullptr, 0);
    if (len > 0) {
        std::vector<wchar_t> buf(len / 2 + 1);
        FPDFAnnot_GetStringValue(m_annot, "T", reinterpret_cast<FPDF_WCHAR*>(buf.data()), len);
        
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, buf.data(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Len > 0) {
            std::string utf8(utf8Len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, buf.data(), -1, utf8.data(), utf8Len, nullptr, nullptr);
            return utf8;
        }
    }
    return "";
}

void PdfAnnotation::SetAuthor(const std::string& author) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    if (author.empty()) {
        FPDFAnnot_SetStringValue(m_annot, "T", reinterpret_cast<FPDF_WIDESTRING>(L""));
        return;
    }
    
    int utf16Len = MultiByteToWideChar(CP_UTF8, 0, author.c_str(), -1, nullptr, 0);
    if (utf16Len > 0) {
        std::vector<wchar_t> utf16(utf16Len);
        MultiByteToWideChar(CP_UTF8, 0, author.c_str(), -1, utf16.data(), utf16Len);
        FPDFAnnot_SetStringValue(m_annot, "T", reinterpret_cast<FPDF_WIDESTRING>(utf16.data()));
    }
}

std::string PdfAnnotation::GetCreationDate() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    unsigned long len = FPDFAnnot_GetStringValue(m_annot, "CreationDate", nullptr, 0);
    if (len > 0) {
        std::vector<wchar_t> buf(len / 2 + 1);
        FPDFAnnot_GetStringValue(m_annot, "CreationDate", reinterpret_cast<FPDF_WCHAR*>(buf.data()), len);
        
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, buf.data(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Len > 0) {
            std::string utf8(utf8Len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, buf.data(), -1, utf8.data(), utf8Len, nullptr, nullptr);
            return utf8;
        }
    }
    return "";
}

void PdfAnnotation::SetCreationDate(const std::string& date) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    if (date.empty()) {
        FPDFAnnot_SetStringValue(m_annot, "CreationDate", reinterpret_cast<FPDF_WIDESTRING>(L""));
        return;
    }
    
    int utf16Len = MultiByteToWideChar(CP_UTF8, 0, date.c_str(), -1, nullptr, 0);
    if (utf16Len > 0) {
        std::vector<wchar_t> utf16(utf16Len);
        MultiByteToWideChar(CP_UTF8, 0, date.c_str(), -1, utf16.data(), utf16Len);
        FPDFAnnot_SetStringValue(m_annot, "CreationDate", reinterpret_cast<FPDF_WIDESTRING>(utf16.data()));
    }
}

std::string PdfAnnotation::GetModificationDate() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    unsigned long len = FPDFAnnot_GetStringValue(m_annot, "M", nullptr, 0);
    if (len > 0) {
        std::vector<wchar_t> buf(len / 2 + 1);
        FPDFAnnot_GetStringValue(m_annot, "M", reinterpret_cast<FPDF_WCHAR*>(buf.data()), len);
        
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, buf.data(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Len > 0) {
            std::string utf8(utf8Len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, buf.data(), -1, utf8.data(), utf8Len, nullptr, nullptr);
            return utf8;
        }
    }
    return "";
}

void PdfAnnotation::SetModificationDate(const std::string& date) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    if (date.empty()) {
        FPDFAnnot_SetStringValue(m_annot, "M", reinterpret_cast<FPDF_WIDESTRING>(L""));
        return;
    }
    
    int utf16Len = MultiByteToWideChar(CP_UTF8, 0, date.c_str(), -1, nullptr, 0);
    if (utf16Len > 0) {
        std::vector<wchar_t> utf16(utf16Len);
        MultiByteToWideChar(CP_UTF8, 0, date.c_str(), -1, utf16.data(), utf16Len);
        FPDFAnnot_SetStringValue(m_annot, "M", reinterpret_cast<FPDF_WIDESTRING>(utf16.data()));
    }
}

int PdfAnnotation::GetFlags() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    return FPDFAnnot_GetFlags(m_annot);
}

void PdfAnnotation::SetFlags(int flags) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    FPDFAnnot_SetFlags(m_annot, flags);
}

bool PdfAnnotation::GetLineGeometry(core::interfaces::dom::LineGeometry& outGeometry) const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    if (GetType() != core::interfaces::dom::AnnotationType::Line) {
        return false;
    }
    if (m_hasLineGeometry) {
        outGeometry = m_lineGeometry;
        return true;
    }
    return pdf_engine::LineAnnotationAdapter::GetGeometry(m_annot, outGeometry);
}

void PdfAnnotation::SetLineGeometry(const core::interfaces::dom::LineGeometry& geometry) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    if (GetType() == core::interfaces::dom::AnnotationType::Line) {
        m_lineGeometry = geometry;
        m_hasLineGeometry = true;
        pdf_engine::LineAnnotationAdapter::SetGeometry(m_annot, geometry);
    };
}

bool PdfAnnotation::HasAppearance() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    return FPDFAnnot_HasKey(m_annot, "AP") != 0;
}

double PdfAnnotation::GetRotation() const {
    return 0.0; 
}

void PdfAnnotation::SetRotation(double degrees) {
    (void)degrees;
}


std::vector<std::vector<PointF>> PdfAnnotation::GetInkList() const {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    if (!m_inkList.empty()) {
        return m_inkList;
    }
    std::vector<std::vector<PointF>> result;
    if (GetType() != core::interfaces::dom::AnnotationType::Ink) {
        return result;
    }
    unsigned long count = FPDFAnnot_GetInkListCount(m_annot);
    for (unsigned long i = 0; i < count; ++i) {
        unsigned long numPoints = FPDFAnnot_GetInkListPath(m_annot, i, nullptr, 0);
        if (numPoints > 0) {
            std::vector<FS_POINTF> buf(numPoints);
            FPDFAnnot_GetInkListPath(m_annot, i, buf.data(), numPoints);
            std::vector<PointF> stroke;
            stroke.reserve(numPoints);
            for (const auto& pt : buf) {
                stroke.push_back({pt.x, pt.y});
            }
            result.push_back(stroke);
        }
    }
    return result;
}

bool PdfAnnotation::AddInkStroke(const std::vector<PointF>& points) {
    if (points.empty()) return false;
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    if (GetType() != core::interfaces::dom::AnnotationType::Ink) {
        return false;
    }
    m_inkList.push_back(points);
    std::vector<FS_POINTF> fsPoints;
    fsPoints.reserve(points.size());
    for (const auto& pt : points) {
        fsPoints.push_back({pt.x, pt.y});
    }
    int res = FPDFAnnot_AddInkStroke(m_annot, fsPoints.data(), fsPoints.size());
    return res >= 0;
}

bool PdfAnnotation::ClearInkList() {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    m_inkList.clear();
    if (GetType() != core::interfaces::dom::AnnotationType::Ink) {
        return false;
    }
    return FPDFAnnot_RemoveInkList(m_annot) != 0;
}

void PdfAnnotation::GenerateAppearanceStream() {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    if (m_page) {
        FPDFPage_GenerateContent(m_page->GetHandle());
    }
}


