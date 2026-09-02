#include "PdfPage.h"
#include "PdfTextPage.h"
#include "PdfImage.h"
#include "PdfTextObject.h"
#include <fpdf_edit.h>
#include <fpdf_text.h>
#include <fpdf_formfill.h>
#include "PdfDocument.h"
#include <fpdf_doc.h>
#include <algorithm>

using namespace pdf_engine;

PdfPage::PdfPage(PdfDocument* parent, FPDF_DOCUMENT doc, FPDF_PAGE page, int pageIndex) 
    : m_parentDoc(parent), m_doc(doc), m_page(page), m_pageIndex(pageIndex) {}

PdfPage::~PdfPage() {
    printf("~PdfPage start %p with parent %p\n", this, m_parentDoc); fflush(stdout);
    if (m_parentDoc) {
        m_parentDoc->ReleasePage(m_pageIndex);
    }
    printf("~PdfPage end\n"); fflush(stdout);
}

std::recursive_mutex& PdfPage::GetDocMutex() const {
    return m_parentDoc->GetMutex();
}

void PdfPage::InvalidateTextIndex() {
    // Forces PDFium to re-parse the page's object list and rebuild internal caches.
    // Must serialize with the RenderWorker: FPDF_* is not thread-safe.
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_page) {
        FPDFPage_CountObjects(m_page);
    }
}

SizeF PdfPage::GetSize() const {
    // Called from the UI thread on every layout/paint while the worker renders the
    // same FPDF_PAGE. FPDF_GetPageWidth/Height read shared page state, so lock.
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    double width = FPDF_GetPageWidth(m_page);
    double height = FPDF_GetPageHeight(m_page);
    return { static_cast<float>(width), static_cast<float>(height) };
}

int PdfPage::GetRotation() const {
    // 0: normal, 1: 90 CW, 2: 180, 3: 270 CW
    // Hot path: called from PdfViewer::Render and OnMouseMove during scroll.
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    return FPDFPage_GetRotation(m_page) * 90;
}

void PdfPage::SetRotation(int degrees) {
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    int rot = (degrees / 90) % 4;
    if (rot < 0) rot += 4;
    FPDFPage_SetRotation(m_page, rot);
}

std::vector<uint8_t> PdfPage::RenderToBitmap(double zoom, int startX, int startY, int sizeX, int sizeY, bool darkMode) const {
    std::vector<uint8_t> buffer(sizeX * sizeY * 4, 0); // BGRA

    FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(sizeX, sizeY, FPDFBitmap_BGRA, buffer.data(), sizeX * 4);
    if (!bitmap) return buffer;

    // Fill white
    FPDFBitmap_FillRect(bitmap, 0, 0, sizeX, sizeY, 0xFFFFFFFF);

    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);

    int fullWidth = static_cast<int>(FPDF_GetPageWidth(m_page) * zoom);
    int fullHeight = static_cast<int>(FPDF_GetPageHeight(m_page) * zoom);
    
    FPDF_RenderPageBitmap(bitmap, m_page, -startX, -startY, fullWidth, fullHeight, 0, FPDF_ANNOT | FPDF_LCD_TEXT);
    
    if (m_parentDoc->GetFormHandle()) {
        FPDF_FFLDraw((FPDF_FORMHANDLE)m_parentDoc->GetFormHandle(), bitmap, m_page, -startX, -startY, fullWidth, fullHeight, 0, FPDF_ANNOT | FPDF_LCD_TEXT);
    }
    
    if (darkMode) {
        for (size_t i = 0; i < buffer.size(); i += 4) {
            buffer[i]     = 255 - buffer[i];     // B
            buffer[i + 1] = 255 - buffer[i + 1]; // G
            buffer[i + 2] = 255 - buffer[i + 2]; // R
            // Leave A alone
        }
    }

    FPDFBitmap_Destroy(bitmap);    
    return buffer;
}

void PdfPage::RenderForPrint(void* hdc, int startX, int startY, int sizeX, int sizeY, int rotate) const {
    // Lock order matches every other method: document mutex first, then page mutex.
    // Without the doc lock this FPDF_RenderPage could run concurrently with the
    // RenderWorker's FPDF_RenderPageBitmap on the same page.
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_page) return;
    
    // Convert HDC. FPDF_PRINTING flag (0x800) enables print optimizations
    FPDF_RenderPage(static_cast<HDC>(hdc), m_page, startX, startY, sizeX, sizeY, rotate, FPDF_PRINTING);
}

std::unique_ptr<core::interfaces::dom::ITextPage> PdfPage::LoadTextPage() {
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    FPDF_TEXTPAGE textPage = FPDFText_LoadPage(m_page);
    if (!textPage) {
        return nullptr;
    }
    return std::make_unique<PdfTextPage>(textPage, &m_parentDoc->GetMutex());
}

extern std::optional<core::interfaces::dom::NavigationTarget> ResolveDest(FPDF_DOCUMENT doc, FPDF_DEST dest); // from PdfDocument.cpp

std::vector<core::interfaces::dom::PdfLink> PdfPage::GetLinks() {
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<core::interfaces::dom::PdfLink> links;
    
    int start_pos = 0;
    FPDF_LINK link_annot;
    while (FPDFLink_Enumerate(m_page, &start_pos, &link_annot)) {
        core::interfaces::dom::PdfLink lnk;
        
        FS_RECTF rect;
        if (FPDFLink_GetAnnotRect(link_annot, &rect)) {
            lnk.bounds = { rect.left, rect.top, rect.right, rect.bottom };
            
            // Get Dest
            FPDF_DEST dest = FPDFLink_GetDest(m_doc, link_annot);
            if (dest) {
                lnk.destination = ResolveDest(m_doc, dest);
            } else {
                // Get Action
                FPDF_ACTION action = FPDFLink_GetAction(link_annot);
                if (action) {
                    unsigned long type = FPDFAction_GetType(action);
                    if (type == PDFACTION_GOTO) {
                        dest = FPDFAction_GetDest(m_doc, action);
                        if (dest) lnk.destination = ResolveDest(m_doc, dest);
                    } else if (type == PDFACTION_URI) {
                        unsigned long uriLen = FPDFAction_GetURIPath(m_doc, action, nullptr, 0);
                        if (uriLen > 0) {
                            std::vector<char> uriBuf(uriLen);
                            FPDFAction_GetURIPath(m_doc, action, uriBuf.data(), uriLen);
                            lnk.uri = std::string(uriBuf.data(), uriLen - 1); // remove null terminator
                        }
                    }
                }
            }
            links.push_back(lnk);
        }
    }
    return links;
}

#include "PdfAnnotation.h"
#include "core/interfaces/dom/FontManager.h"
#include <algorithm>

std::vector<std::shared_ptr<core::interfaces::dom::IAnnotation>> PdfPage::GetAnnotations() {
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    
    int count = FPDFPage_GetAnnotCount(m_page);
    std::vector<std::shared_ptr<core::interfaces::dom::IAnnotation>> result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) {
        FPDF_ANNOTATION annot = FPDFPage_GetAnnot(m_page, i);
        if (annot) {
            std::shared_ptr<core::interfaces::dom::IAnnotation> matched = nullptr;
            for (const auto& existing : m_annotations) {
                auto pdfExisting = std::dynamic_pointer_cast<PdfAnnotation>(existing);
                if (pdfExisting && pdfExisting->GetHandle() == annot) {
                    matched = existing;
                    break;
                }
            }
            if (matched) {
                result.push_back(matched);
                FPDFPage_CloseAnnot(annot);
            } else {
                result.push_back(std::shared_ptr<PdfAnnotation>(new PdfAnnotation(annot, this)));
            }
        }
    }
    m_annotations = result;
    return m_annotations;
}

std::shared_ptr<core::interfaces::dom::IAnnotation> PdfPage::CreateAnnotation(core::interfaces::dom::AnnotationType type) {
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    int subtype = static_cast<int>(type);
    FPDF_ANNOTATION annot = FPDFPage_CreateAnnot(m_page, subtype);
    if (annot) {
        auto pdfAnnot = std::shared_ptr<PdfAnnotation>(new PdfAnnotation(annot, this));
        m_annotations.push_back(pdfAnnot);
        return pdfAnnot;
    }
    return nullptr;
}

bool PdfPage::RemoveAnnotation(std::shared_ptr<core::interfaces::dom::IAnnotation> annot) {
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    auto pdfAnnot = std::dynamic_pointer_cast<PdfAnnotation>(annot);
    if (pdfAnnot && pdfAnnot->GetHandle()) {
        int index = FPDFPage_GetAnnotIndex(m_page, pdfAnnot->GetHandle());
        if (index >= 0) {
            FPDFPage_RemoveAnnot(m_page, index);
            FPDFPage_GenerateContent(m_page);
            auto it = std::find(m_annotations.begin(), m_annotations.end(), annot);
            if (it != m_annotations.end()) {
                m_annotations.erase(it);
            }
            return true;
        }
    }
    return false;
}



void PdfPage::GenerateContent() {
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_page) {
        FPDFPage_GenerateContent(m_page);
    }
}

std::shared_ptr<core::interfaces::dom::IImage> PdfPage::InsertImage(const std::wstring& imagePath, const RectF& bounds) {
    (void)imagePath; (void)bounds;
    return nullptr;
}

std::shared_ptr<core::interfaces::dom::IImage> PdfPage::InsertImageFromMemory(const std::vector<uint8_t>& imageData, int width, int height, const RectF& bounds) {
    (void)imageData; (void)width; (void)height; (void)bounds;
    return nullptr;
}

std::vector<std::shared_ptr<core::interfaces::dom::IImage>> PdfPage::GetImages() {
    return {};
}

bool PdfPage::RemoveImage(std::shared_ptr<core::interfaces::dom::IImage> image) {
    (void)image;
    return false;
}

#include "PdfTextObject.h"
#include <map>
#include <string>

std::shared_ptr<core::interfaces::dom::ITextObject> PdfPage::InsertTextObject(const std::wstring& text, const RectF& bounds, const std::string& fontName, float fontSize) {
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    
    FPDF_PAGEOBJECT newObj = FPDFPageObj_NewTextObj(m_doc, fontName.c_str(), fontSize);
    if (!newObj) return nullptr;
    
    FPDF_WIDESTRING widestr = reinterpret_cast<FPDF_WIDESTRING>(text.c_str());
    FPDFText_SetText(newObj, widestr);
    
    FS_MATRIX mat = {1, 0, 0, 1, bounds.left, bounds.bottom};
    FPDFPageObj_SetMatrix(newObj, &mat);
    
    FPDFPage_InsertObject(m_page, newObj);
    FPDFPage_GenerateContent(m_page);
    
    return std::make_shared<PdfTextObject>(m_doc, m_page, newObj, &m_parentDoc->GetMutex());
}

std::vector<std::shared_ptr<core::interfaces::dom::ITextObject>> PdfPage::GetTextObjects() {
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<std::shared_ptr<core::interfaces::dom::ITextObject>> result;
    int count = FPDFPage_CountObjects(m_page);
    
    std::map<std::string, std::shared_ptr<PdfTextObject>> blocks;
    
    for (int i = 0; i < count; ++i) {
        FPDF_PAGEOBJECT obj = FPDFPage_GetObject(m_page, i);
        if (FPDFPageObj_GetType(obj) == FPDF_PAGEOBJ_TEXT) {
            int markCount = FPDFPageObj_CountMarks(obj);
            std::string blockId;
            for (int j = 0; j < markCount; ++j) {
                FPDF_PAGEOBJECTMARK mark = FPDFPageObj_GetMark(obj, j);
                char nameBuf[256] = {0};
                FPDFPageObjMark_GetName(mark, nameBuf, sizeof(nameBuf), nullptr);
                if (std::string(nameBuf) == "PDFElite_TextBlock") {
                    unsigned long len = 0;
                    FPDFPageObjMark_GetParamStringValue(mark, "BlockID", nullptr, 0, &len);
                    if (len > 0) {
                        std::vector<char> valBuf(len);
                        FPDFPageObjMark_GetParamStringValue(mark, "BlockID", valBuf.data(), len, nullptr);
                        blockId = std::string(valBuf.data(), len - 1);
                    }
                    break;
                }
            }
            
            if (!blockId.empty()) {
                if (blocks.find(blockId) != blocks.end()) {
                    blocks[blockId]->AddHandle(obj);
                } else {
                    auto textObj = std::make_shared<PdfTextObject>(m_doc, m_page, obj, &m_parentDoc->GetMutex());
                    blocks[blockId] = textObj;
                    result.push_back(textObj);
                }
            } else {
                result.push_back(std::make_shared<PdfTextObject>(m_doc, m_page, obj, &m_parentDoc->GetMutex()));
            }
        }
    }
    return result;
}

bool PdfPage::RemoveTextObject(std::shared_ptr<core::interfaces::dom::ITextObject> textObject) {
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    auto pdfTextObj = std::dynamic_pointer_cast<PdfTextObject>(textObject);
    if (!pdfTextObj) return false;
    
    bool removedAny = false;
    for (auto handle : pdfTextObj->GetHandles()) {
        FPDFPage_RemoveObject(m_page, handle);
        removedAny = true;
    }
    if (removedAny) {
        pdfTextObj->SetAttached(false);
        FPDFPage_GenerateContent(m_page);
        return true;
    }
    return false;
}

bool PdfPage::RestoreTextObject(std::shared_ptr<core::interfaces::dom::ITextObject> textObj) {
    std::lock_guard<std::recursive_mutex> docLock(m_parentDoc->GetMutex());
    std::lock_guard<std::mutex> lock(m_mutex);
    auto pdfTextObj = std::dynamic_pointer_cast<PdfTextObject>(textObj);
    if (!pdfTextObj) return false;
    
    bool restoredAny = false;
    for (auto handle : pdfTextObj->GetHandles()) {
        FPDFPage_InsertObject(m_page, handle);
        restoredAny = true;
    }
    if (restoredAny) {
        pdfTextObj->SetAttached(true);
        FPDFPage_GenerateContent(m_page);
        return true;
    }
    return false;
}
