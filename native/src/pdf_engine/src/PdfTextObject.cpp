#include "PdfTextObject.h"
#include "PdfPage.h"
#include "core/interfaces/dom/FontManager.h"
#include <vector>
#include <fpdf_text.h>
#include <fpdf_edit.h>
#include <objbase.h>

// PDFium is not thread-safe. Every FPDF_* call below can race the background
// RenderWorker (rendering the same page) or the SearchEngine thread, so each
// public method acquires the owning document's recursive_mutex for its full
// duration. m_docMutex is supplied by PdfPage at construction; the null-guard
// keeps the object usable in tests where no mutex is provided.

PdfTextObject::PdfTextObject(FPDF_DOCUMENT doc, FPDF_PAGE page, FPDF_PAGEOBJECT textObj, std::recursive_mutex* docMutex)
    : m_doc(doc), m_page(page), m_docMutex(docMutex) {
    if (textObj) m_textObjs.push_back(textObj);
}

PdfTextObject::~PdfTextObject() {
    if (!m_isAttached) {
        std::unique_lock<std::recursive_mutex> lock;
        if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
        for (auto handle : m_textObjs) {
            FPDFPageObj_Destroy(handle);
        }
    }
}

std::wstring PdfTextObject::GetText() const {
    if (m_textObjs.empty()) return L"";
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);

    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(m_page);
    if (!textPage) return L"";

    std::wstring fullText;
    for (size_t i = 0; i < m_textObjs.size(); ++i) {
        unsigned long len = FPDFTextObj_GetText(m_textObjs[i], textPage, nullptr, 0);
        if (len > 0) {
            std::vector<FPDF_WCHAR> buffer(len / sizeof(FPDF_WCHAR));
            FPDFTextObj_GetText(m_textObjs[i], textPage, buffer.data(), len);
            fullText += std::wstring(buffer.begin(), buffer.end() - 1);
        }
        if (i < m_textObjs.size() - 1) {
            fullText += L"\n";
        }
    }
    
    FPDFText_ClosePage(textPage);
    return fullText;
}

bool PdfTextObject::SetText(const std::wstring& text) {
    if (m_textObjs.empty()) return false;
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);

    float fontSize = 12.0f;
    FPDFTextObj_GetFontSize(m_textObjs[0], &fontSize);
    if (fontSize <= 0.0f) fontSize = 12.0f;

    unsigned int r = 0, g = 0, b = 0, a = 255;
    FPDFPageObj_GetFillColor(m_textObjs[0], &r, &g, &b, &a);

    float left = 0.0f, bottom = 0.0f, right = 0.0f, top = 0.0f;
    FPDFPageObj_GetBounds(m_textObjs[0], &left, &bottom, &right, &top);

    FS_MATRIX mat;
    if (!FPDFPageObj_GetMatrix(m_textObjs[0], &mat)) {
        mat = { 1.0f, 0.0f, 0.0f, 1.0f, left, bottom };
    }

    static core::interfaces::dom::FontManager fontManager;
    bool needsFallback = false;
    for (wchar_t c : text) {
        if (c > 127) {
            needsFallback = true;
            break;
        }
    }

    FPDF_PAGEOBJECT newObj = nullptr;
    if (needsFallback && fontManager.HasGlyphs(text)) {
        FPDF_FONT fallbackFont = fontManager.LoadFallbackFont(m_doc);
        if (fallbackFont) {
            newObj = FPDFPageObj_CreateTextObj(m_doc, fallbackFont, fontSize);
        }
    }

    if (!newObj) {
        newObj = FPDFPageObj_NewTextObj(m_doc, "Arial", fontSize);
    }

    if (!newObj) return false;

    FPDF_WIDESTRING widestr = reinterpret_cast<FPDF_WIDESTRING>(text.c_str());
    FPDFText_SetText(newObj, widestr);
    FPDFPageObj_SetMatrix(newObj, &mat);
    FPDFPageObj_SetFillColor(newObj, r, g, b, a);

    for (auto handle : m_textObjs) {
        FPDFPage_RemoveObject(m_page, handle);
        FPDFPageObj_Destroy(handle);
    }
    m_textObjs.clear();

    FPDFPage_InsertObject(m_page, newObj);
    m_textObjs.push_back(newObj);
    FPDFPage_GenerateContent(m_page);
    return true;
}

bool PdfTextObject::SetLines(const std::vector<core::interfaces::dom::TextLineData>& lines) {
    if (m_textObjs.empty() || lines.empty()) return false;
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);

    // Save properties from the first object
    float fontSize = 0;
    FPDFTextObj_GetFontSize(m_textObjs[0], &fontSize);
    
    unsigned int r=255, g=255, b=255, a=255;
    FPDFPageObj_GetFillColor(m_textObjs[0], &r, &g, &b, &a);
    
    // Get transform matrix of first line to use as base for X offset if needed?
    // Wait, the DirectWrite layout provides X and Y offsets relative to the top-left of the box.
    // In PDF, Y goes up. So we have to apply the text layout coordinates relative to the original PDF text box.
    // Let's get the original bounds
    float l, b_bound, right, t;
    FPDFPageObj_GetBounds(m_textObjs[0], &l, &b_bound, &right, &t);
    
    // The top-left of the original text object
    float originX = l;
    float originY = t;
    
    while (m_textObjs.size() > lines.size()) {
        FPDFPage_RemoveObject(m_page, m_textObjs.back());
        FPDFPageObj_Destroy(m_textObjs.back());
        m_textObjs.pop_back();
    }
    
    while (m_textObjs.size() < lines.size()) {
        FPDF_PAGEOBJECT newObj = FPDFPageObj_NewTextObj(m_doc, "Arial", fontSize);
        FPDFPageObj_SetFillColor(newObj, r, g, b, a);
        FPDFPage_InsertObject(m_page, newObj);
        m_textObjs.push_back(newObj);
    }
    
    // Generate a unique BlockID for this multiline block if it has multiple lines
    std::string blockId;
    if (m_textObjs.size() > 1) {
        GUID guid;
        CoCreateGuid(&guid);
        char buf[64];
        snprintf(buf, sizeof(buf), "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                 guid.Data1, guid.Data2, guid.Data3,
                 guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                 guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
        blockId = buf;
    }
    
    for (size_t i = 0; i < lines.size(); ++i) {
        FPDF_WIDESTRING widestr = reinterpret_cast<FPDF_WIDESTRING>(lines[i].text.c_str());
        FPDFText_SetText(m_textObjs[i], widestr);
        
        float pdf_y = originY - lines[i].y - fontSize;
        float pdf_x = originX + lines[i].x;
        
        FS_MATRIX mat = {1, 0, 0, 1, pdf_x, pdf_y};
        FPDFPageObj_SetMatrix(m_textObjs[i], &mat);
        
        if (!blockId.empty()) {
            FPDF_PAGEOBJECTMARK mark = FPDFPageObj_AddMark(m_textObjs[i], "PDFElite_TextBlock");
            FPDFPageObjMark_SetStringParam(m_doc, m_textObjs[i], mark, "BlockID", blockId.c_str());
        }
    }
    
    FPDFPage_GenerateContent(m_page);
    return true;
}

float PdfTextObject::GetFontSize() const {
    if (m_textObjs.empty()) return 0;
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    float size = 0.0f;
    FPDFTextObj_GetFontSize(m_textObjs[0], &size);
    return size;
}

bool PdfTextObject::SetFontSize(float /*size*/) {
    return true;
}

RectF PdfTextObject::GetBounds() const {
    if (m_explicitBounds.Width() > 0 && m_explicitBounds.Height() > 0) {
        return m_explicitBounds;
    }
    RectF bounds{0,0,0,0};
    if (m_textObjs.empty()) return bounds;
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);

    float minL = 99999, minB = 99999, maxR = -99999, maxT = -99999;
    for (auto obj : m_textObjs) {
        float l, b, r, t;
        if (FPDFPageObj_GetBounds(obj, &l, &b, &r, &t)) {
            if (l < minL) minL = l;
            if (b < minB) minB = b;
            if (r > maxR) maxR = r;
            if (t > maxT) maxT = t;
        }
    }
    bounds.left = minL == 99999 ? 0 : minL;
    bounds.bottom = minB == 99999 ? 0 : minB;
    bounds.right = maxR == -99999 ? 0 : maxR;
    bounds.top = maxT == -99999 ? 0 : maxT;
    return bounds;
}

Matrix3x2F PdfTextObject::GetTransform() const {
    Matrix3x2F mat{1,0,0,1,0,0};
    if (m_textObjs.empty()) return mat;
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);

    FS_MATRIX fs_mat;
    if (FPDFPageObj_GetMatrix(m_textObjs[0], &fs_mat)) {
        mat.a = fs_mat.a; mat.b = fs_mat.b; mat.c = fs_mat.c; mat.d = fs_mat.d; mat.e = fs_mat.e; mat.f = fs_mat.f;
    }
    return mat;
}

bool PdfTextObject::SetTransform(const Matrix3x2F& matrix) {
    if (m_textObjs.empty()) return false;
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);

    if (m_explicitBounds.Width() > 0 && m_explicitBounds.Height() > 0) {
        FS_MATRIX fs_mat;
        if (FPDFPageObj_GetMatrix(m_textObjs[0], &fs_mat)) {
            float dx = matrix.e - fs_mat.e;
            float dy = matrix.f - fs_mat.f;
            m_explicitBounds.left += dx;
            m_explicitBounds.right += dx;
            m_explicitBounds.top += dy;
            m_explicitBounds.bottom += dy;
        }
    }

    if (m_textObjs.size() == 1) {
        FS_MATRIX fs_mat{matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f};
        bool res = FPDFPageObj_SetMatrix(m_textObjs[0], &fs_mat) == TRUE;
        if (res) FPDFPage_GenerateContent(m_page);
        return res;
    }
    
    FS_MATRIX fs_mat;
    if (FPDFPageObj_GetMatrix(m_textObjs[0], &fs_mat)) {
        float dx = matrix.e - fs_mat.e;
        float dy = matrix.f - fs_mat.f;
        for (auto obj : m_textObjs) {
            FS_MATRIX m;
            if (FPDFPageObj_GetMatrix(obj, &m)) {
                m.e += dx;
                m.f += dy;
                FPDFPageObj_SetMatrix(obj, &m);
            }
        }
    }
    return true;
}

std::string PdfTextObject::GetFontName() const {
    if (m_textObjs.empty()) return "";
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    FPDF_FONT font = FPDFTextObj_GetFont(m_textObjs[0]);
    if (!font) return "";

    size_t len = FPDFFont_GetBaseFontName(font, nullptr, 0);
    if (len == 0) return "";

    std::vector<char> buf(len);
    FPDFFont_GetBaseFontName(font, buf.data(), len);
    return std::string(buf.data());
}

void PdfTextObject::GetColor(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const {
    r = g = b = a = 255;
    if (m_textObjs.empty()) return;
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    unsigned int R, G, B, A;
    if (FPDFPageObj_GetFillColor(m_textObjs[0], &R, &G, &B, &A)) {
        r = static_cast<uint8_t>(R); g = static_cast<uint8_t>(G); b = static_cast<uint8_t>(B); a = static_cast<uint8_t>(A);
    }
}

bool PdfTextObject::SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    std::unique_lock<std::recursive_mutex> lock;
    if (m_docMutex) lock = std::unique_lock<std::recursive_mutex>(*m_docMutex);
    bool ok = true;
    for (auto obj : m_textObjs) {
        if (!FPDFPageObj_SetFillColor(obj, r, g, b, a)) ok = false;
    }
    return ok;
}

uint64_t PdfTextObject::GetId() const {
    if (m_textObjs.empty()) return 0;
    return reinterpret_cast<uint64_t>(m_textObjs[0]);
}
