#include "../../utils/Logger.h"
#include "PdfDocument.h"

#include <fstream>
#include <chrono>


static void LogPipeline(const std::string& msg) {
    std::ofstream out("C:\\Users\\sayed\\Downloads\\PDF-Elite\\pipeline.log", std::ios_base::app);
    out << "[" << std::chrono::system_clock::now().time_since_epoch().count() << "] " << msg << "\n";
}


#include "PdfPage.h"
#include <fstream>
#include <filesystem>
#include <string_view>
#include <windows.h>
#include <fpdf_save.h>
#include <fpdf_edit.h>
#include <fpdf_ppo.h>
#include <fpdf_formfill.h>
#include <fpdf_annot.h>
#include <fpdf_doc.h>
#include "PdfFormField.h"

Result<std::unique_ptr<PdfDocument>> PdfDocument::LoadFromFile(const wchar_t* path) {
    LogPipeline("PdfDocument::LoadFromFile start");
    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        LogPipeline("CreateFileW failed");
        return Result<std::unique_ptr<PdfDocument>>::Error(ErrorCode::FileOpenFailed);
    }
    
    LogPipeline("CreateFileW success");
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        LogPipeline("GetFileSizeEx failed");
        return Result<std::unique_ptr<PdfDocument>>::Error(ErrorCode::FileOpenFailed);
    }
    
    LogPipeline("File size checked: " + std::to_string(fileSize.QuadPart));
    if (fileSize.QuadPart > 100LL * 1024 * 1024 * 1024) {
        CloseHandle(hFile);
        LogPipeline("File too large");
        return Result<std::unique_ptr<PdfDocument>>::Error(ErrorCode::FileTooLarge);
    }
    if (fileSize.QuadPart < 5) {
        CloseHandle(hFile);
        LogPipeline("File too small");
        return Result<std::unique_ptr<PdfDocument>>::Error(ErrorCode::InvalidFormat);
    }

    LogPipeline("Reading magic bytes");
    char magic[5] = {0};
    DWORD bytesRead = 0;
    ReadFile(hFile, magic, 5, &bytesRead, NULL);
    if (bytesRead != 5 || std::string_view(magic, 5) != "%PDF-") {
        CloseHandle(hFile);
        LogPipeline("Invalid magic bytes");
        return Result<std::unique_ptr<PdfDocument>>::Error(ErrorCode::InvalidFormat);
    }

    LogPipeline("Creating PdfDocument wrapper");
    auto pdfDoc = std::make_unique<PdfDocument>(nullptr);
    pdfDoc->m_fileHandle = hFile;
    pdfDoc->m_fileAccess.m_FileLen = static_cast<unsigned long>(fileSize.QuadPart);
    pdfDoc->m_fileAccess.m_Param = pdfDoc->m_fileHandle;
    pdfDoc->m_fileAccess.m_GetBlock = [](void* param, unsigned long position, unsigned char* pBuf, unsigned long size) -> int {
        HANDLE h = static_cast<HANDLE>(param);
        if (h == INVALID_HANDLE_VALUE) return 0;
        LARGE_INTEGER li;
        li.QuadPart = position;
        if (!SetFilePointerEx(h, li, NULL, FILE_BEGIN)) return 0;
        DWORD read = 0;
        BOOL res = ReadFile(h, pBuf, size, &read, NULL);
        if (res && read == size) return 1;
        return 0;
    };
    
    LogPipeline("Calling FPDF_LoadCustomDocument");
    FPDF_DOCUMENT doc = FPDF_LoadCustomDocument(&pdfDoc->m_fileAccess, nullptr);
    LogPipeline("FPDF_LoadCustomDocument returned");
    if (!doc) {
        unsigned long err = FPDF_GetLastError();
        LogPipeline("FPDF_LoadCustomDocument failed, err=" + std::to_string(err));
        if (err == FPDF_ERR_PASSWORD) return Result<std::unique_ptr<PdfDocument>>::Error(ErrorCode::AccessDenied);
        return Result<std::unique_ptr<PdfDocument>>::Error(ErrorCode::InvalidFormat);
    }

    pdfDoc->m_doc = doc;
    LogPipeline("PdfDocument::LoadFromFile success");
    return Result<std::unique_ptr<PdfDocument>>::Success(std::move(pdfDoc));
}

Result<std::unique_ptr<PdfDocument>> PdfDocument::LoadFromMemory(const uint8_t* data, size_t len) {
    FPDF_DOCUMENT doc = FPDF_LoadMemDocument(data, static_cast<int>(len), nullptr);
    if (!doc) {
        printf("[LoadFromFile] Invalid format\n"); fflush(stdout); printf("[LoadFromFile] Invalid format FPDF_LoadCustomDocument\n"); fflush(stdout); return Result<std::unique_ptr<PdfDocument>>::Error(ErrorCode::InvalidFormat);
    }
    return Result<std::unique_ptr<PdfDocument>>::Success(std::unique_ptr<PdfDocument>(new PdfDocument(doc)));
}

PdfDocument::PdfDocument(FPDF_DOCUMENT doc) : m_doc(doc) {
}

void PdfDocument::InvalidateOpenPages() {
    for (auto& pair : m_openPages) {
        if (pair.second) {
            if (m_formHandle) {
                FORM_OnBeforeClosePage(pair.second, (FPDF_FORMHANDLE)m_formHandle);
            }
            FPDF_ClosePage(pair.second);
        }
    }
    m_openPages.clear();
    m_pageRefCounts.clear();
}

PdfDocument::~PdfDocument() {
    printf("~PdfDocument start\n"); fflush(stdout);
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    printf("~PdfDocument 1: Clear command stack\n"); fflush(stdout);
    m_commandStack.Clear();
    printf("~PdfDocument 2: Invalidate open pages\n"); fflush(stdout);
    InvalidateOpenPages();

    printf("~PdfDocument 3: Close FPDF_DOCUMENT %p\n", m_doc); fflush(stdout);
    if (m_doc) {
        FPDF_CloseDocument(m_doc);
        m_doc = nullptr;
    }
    printf("~PdfDocument 4: Close file handle\n"); fflush(stdout);
    if (m_fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_fileHandle);
        m_fileHandle = INVALID_HANDLE_VALUE;
    }
    printf("~PdfDocument end\n"); fflush(stdout);
}

int PdfDocument::PageCount() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return FPDF_GetPageCount(m_doc);
}

bool PdfDocument::Save() {
    // Stub for in-place save
    return false;
}

struct FileWriter : public FPDF_FILEWRITE {
    std::ofstream file;
    FileWriter(const std::wstring& path) : file(path, std::ios::binary) {
        version = 1;
        WriteBlock = [](FPDF_FILEWRITE* pThis, const void* pData, unsigned long size) -> int {
            auto* writer = static_cast<FileWriter*>(pThis);
            if (writer->file.write(static_cast<const char*>(pData), size)) {
                return 1;
            }
            return 0;
        };
    }
};

bool PdfDocument::SaveAs(const std::wstring& path) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    
    KillFocus(); // Commit any active form fields
    
    // Atomic save: write to temp file then rename
    std::wstring tempPath = path + L".tmp";
    
    FileWriter writer(tempPath);
    if (!writer.file.is_open()) return false;

    bool success = FPDF_SaveAsCopy(m_doc, &writer, 0);
    writer.file.close();

    if (success) {
        if (ReplaceFileW(path.c_str(), tempPath.c_str(), NULL, REPLACEFILE_IGNORE_MERGE_ERRORS, NULL, NULL)) {
            return true;
        } else {
            // ReplaceFileW fails if the target doesn't exist yet, so we fallback to rename
            std::error_code ec;
            std::filesystem::rename(tempPath, path, ec);
            if (!ec) return true;
            
            std::filesystem::copy_file(tempPath, path, std::filesystem::copy_options::overwrite_existing, ec);
            if (!ec) {
                std::filesystem::remove(tempPath);
                return true;
            }
            success = false;
        }
    } else {
        std::filesystem::remove(tempPath);
    }

    return success;
}

std::optional<core::interfaces::dom::NavigationTarget> ResolveDest(FPDF_DOCUMENT doc, FPDF_DEST dest) {
    if (!dest) return std::nullopt;
    
    core::interfaces::dom::NavigationTarget target;
    target.pageIndex = FPDFDest_GetDestPageIndex(doc, dest);
    if (target.pageIndex < 0) return std::nullopt;
    
    unsigned long numParams = 0;
    FS_FLOAT viewParams[8] = {0};
    unsigned long type = FPDFDest_GetView(dest, &numParams, viewParams);
    if (numParams > 0) {
        // type 0 = XYZ, 1 = Fit, 2 = FitH, 3 = FitV, 4 = FitR, 5 = FitB, 6 = FitBH, 7 = FitBV
        if (type == 0 && numParams >= 3) {
            target.left = viewParams[0];
            target.top = viewParams[1];
            target.zoom = viewParams[2];
            // 0 in PDFium means "keep current", but we'll let UI decide if nullopt or 0.
            if (target.left == 0 && viewParams[0] == 0) target.left = std::nullopt; // Simplify
        } else if (type == 2 && numParams >= 1) { // FitH
            target.top = viewParams[0];
        }
    }
    
    return target;
}

static void ExtractBookmarks(FPDF_DOCUMENT doc, FPDF_BOOKMARK rootParent, 
std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>>& outBookmarks) {
    struct StackEntry {
        FPDF_BOOKMARK bookmark;
        std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>>* targetVector;
    };
    std::vector<StackEntry> stack;
    
    FPDF_BOOKMARK rootFirst = FPDFBookmark_GetFirstChild(doc, rootParent);
    if (rootFirst) {
        stack.push_back({rootFirst, &outBookmarks});
    }
    
    int processedCount = 0;
    const int MAX_BOOKMARKS = 50000;
    
    while (!stack.empty()) {
        if (++processedCount > MAX_BOOKMARKS) break;
        
        auto entry = stack.back();
        stack.pop_back();
        
        FPDF_BOOKMARK bookmark = entry.bookmark;
        std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>>* targetVector = entry.targetVector;
        
        auto bm = std::make_unique<core::interfaces::dom::PdfBookmark>();
        
        unsigned long titleLen = FPDFBookmark_GetTitle(bookmark, nullptr, 0);
        if (titleLen >= 2) {
            std::vector<char16_t> buffer(titleLen / 2);
            FPDFBookmark_GetTitle(bookmark, buffer.data(), titleLen);
            size_t wlen = (titleLen / 2);
            if (wlen > 0 && buffer[wlen - 1] == 0) wlen--;
            bm->title = std::wstring(reinterpret_cast<wchar_t*>(buffer.data()), wlen);
        }
        
        FPDF_DEST dest = FPDFBookmark_GetDest(doc, bookmark);
        if (!dest) {
            FPDF_ACTION action = FPDFBookmark_GetAction(bookmark);
            if (action) {
                unsigned long actionType = FPDFAction_GetType(action);
                if (actionType == PDFACTION_GOTO) {
                    dest = FPDFAction_GetDest(doc, action);
                }
            }
        }
        if (dest) {
            bm->destination = ResolveDest(doc, dest);
        }
        
        targetVector->push_back(std::move(bm));
        
        FPDF_BOOKMARK sibling = FPDFBookmark_GetNextSibling(doc, bookmark);
        if (sibling) {
            stack.push_back({sibling, targetVector});
        }
        
        FPDF_BOOKMARK child = FPDFBookmark_GetFirstChild(doc, bookmark);
        if (child) {
            auto& children = targetVector->back()->children;
            stack.push_back({child, &children});
        }
    }
}

std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> PdfDocument::GetBookmarks() {
    // Runs on the UI thread when building the outline sidebar. ExtractBookmarks/
    // ResolveDest issue many FPDFBookmark_*/FPDFAction_* calls that must serialize
    // with the RenderWorker.
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<std::unique_ptr<core::interfaces::dom::PdfBookmark>> bookmarks;
    if (m_doc) {
        ExtractBookmarks(m_doc, nullptr, bookmarks);
    }
    return bookmarks;
}

std::shared_ptr<core::interfaces::dom::IPage> PdfDocument::GetPage(int index) {
    printf("[TRACE_DOC: GetPage start %d]\n", index); fflush(stdout);
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    printf("[TRACE_DOC: GetPage lock acquired]\n"); fflush(stdout);
    FPDF_PAGE page = nullptr;
    if (m_openPages.find(index) != m_openPages.end()) {
        page = m_openPages[index];
        m_pageRefCounts[index]++;
    } else {
        printf("[TRACE_DOC: calling FPDF_LoadPage]\n"); fflush(stdout);
        page = FPDF_LoadPage(m_doc, index);
        printf("[TRACE_DOC: FPDF_LoadPage returned %p]\n", page); fflush(stdout);
        if (!page) return nullptr;
        m_openPages[index] = page;
        m_pageRefCounts[index] = 1;
        if (m_formHandle) {
            FORM_OnAfterLoadPage(page, (FPDF_FORMHANDLE)m_formHandle);
        }
    }
    auto p = std::shared_ptr<PdfPage>(new PdfPage(this, m_doc, page, index));
    printf("GetPage created %p with parent %p\n", p.get(), this); fflush(stdout);
    return p;
}

void PdfDocument::ReleasePage(int index) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_pageRefCounts.find(index) != m_pageRefCounts.end()) {
        m_pageRefCounts[index]--;
        if (m_pageRefCounts[index] <= 0) {
            auto it = m_openPages.find(index);
            if (it != m_openPages.end()) {
                FPDF_ClosePage(it->second);
                m_openPages.erase(it);
            }
            m_pageRefCounts.erase(index);
        }
    }
}

SizeF PdfDocument::GetPageSize(int index) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    double width = 0.0;
    double height = 0.0;
    if (FPDF_GetPageSizeByIndex(m_doc, index, &width, &height)) {
        return { static_cast<float>(width), static_cast<float>(height) };
    }
    return { 0.0f, 0.0f };
}

std::string PdfDocument::GetMetadata(const std::string& key) const {
    (void)key;
    return "";
}

bool PdfDocument::DeletePage(int index) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (index < 0 || index >= PageCount()) return false;
    // Don't delete the last remaining page to prevent invalid document state
    if (PageCount() <= 1) return false;
    InvalidateOpenPages();
    FPDFPage_Delete(m_doc, index);
    return true;
}

bool PdfDocument::InsertBlankPage(int index, double width, double height) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    int pc = PageCount();
    if (index < 0 || index > pc) return false;
    InvalidateOpenPages();
    FPDF_PAGE page = FPDFPage_New(m_doc, index, width, height);
    if (!page) return false;
    FPDF_ClosePage(page);
    return true;
}

bool PdfDocument::DuplicatePage(int index) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (index < 0 || index >= PageCount()) return false;
    InvalidateOpenPages();
    // The documentation for FPDF_ImportPagesByIndex says: "The first page is zero."
    int zeroBasedIndices[] = { index };
    return FPDF_ImportPagesByIndex(m_doc, m_doc, zeroBasedIndices, 1, index + 1) != 0;
}

bool PdfDocument::MovePage(int sourceIndex, int destIndex) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    int pc = PageCount();
    if (sourceIndex < 0 || sourceIndex >= pc) return false;
    if (destIndex < 0 || destIndex > pc) return false;
    if (sourceIndex == destIndex || sourceIndex + 1 == destIndex) return true; // No-op
    InvalidateOpenPages();
    int indices[] = { sourceIndex };
    return FPDF_MovePages(m_doc, indices, 1, destIndex) != 0;
}

bool PdfDocument::RotatePage(int index, int rotationDegrees) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (index < 0 || index >= PageCount()) return false;
    bool cached = (m_openPages.find(index) != m_openPages.end());
    FPDF_PAGE page = cached ? m_openPages[index] : FPDF_LoadPage(m_doc, index);
    if (!page) return false;
    
    int targetRot = ((rotationDegrees % 360) + 360) % 360 / 90;
    FPDFPage_SetRotation(page, targetRot);
    if (!cached) {
        FPDF_ClosePage(page);
    }
    return true;
}

std::unique_ptr<core::interfaces::dom::IDocument> PdfDocument::ExtractPages(const std::vector<int>& indices) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    FPDF_DOCUMENT newDoc = FPDF_CreateNewDocument();
    if (!newDoc) return nullptr;
    
    if (!FPDF_ImportPagesByIndex(newDoc, m_doc, indices.data(), static_cast<unsigned long>(indices.size()), 0)) {
        FPDF_CloseDocument(newDoc);
        return nullptr;
    }
    return std::unique_ptr<core::interfaces::dom::IDocument>(new PdfDocument(newDoc));
}

bool PdfDocument::InsertPagesFrom(core::interfaces::dom::IDocument* sourceDoc, const std::vector<int>& sourceIndices, int destIndex) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    PdfDocument* src = dynamic_cast<PdfDocument*>(sourceDoc);
    if (!src) return false;
    
    // To prevent deadlock, we must be careful with mutexes. Assuming we only do this when single-threaded or carefully locked.
    // For now, we will assume it's safe to lock the other document
    std::lock_guard<std::recursive_mutex> lock2(src->m_mutex);
    InvalidateOpenPages();
    return FPDF_ImportPagesByIndex(m_doc, src->GetFpdfDocument(), sourceIndices.data(), static_cast<unsigned long>(sourceIndices.size()), destIndex) != 0;
}

struct MemoryWriter : public FPDF_FILEWRITE {
    std::vector<uint8_t> buffer;
    MemoryWriter() {
        version = 1;
        WriteBlock = [](FPDF_FILEWRITE* pThis, const void* pData, unsigned long size) -> int {
            auto* writer = static_cast<MemoryWriter*>(pThis);
            writer->buffer.insert(writer->buffer.end(), static_cast<const uint8_t*>(pData), static_cast<const uint8_t*>(pData) + size);
            return 1;
        };
    }
};

std::unique_ptr<core::interfaces::dom::IDocument> PdfDocument::Clone() {
    // Return nullptr to force RenderWorker to share this instance.
    // PdfPage::RenderToBitmap already uses docLock(m_parentDoc->GetMutex()),
    // so sharing the single FPDF_DOCUMENT instance is fully thread-safe and avoids
    // massive memory serialization overhead (FPDF_SaveAsCopy) for every worker thread.
    return nullptr;
}

// ------------------------------------------------------------------------------------------------
// Form Fill Implementation
// ------------------------------------------------------------------------------------------------

void* PdfDocument::GetFormHandle() const {
    return m_formHandle;
}

bool PdfDocument::HasForms() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_doc) return false;
    int formType = FPDF_GetFormType(m_doc);
    return formType == FORMTYPE_ACRO_FORM || formType == FORMTYPE_XFA_FOREGROUND || formType == FORMTYPE_XFA_FULL;
}

struct MyFormFillInfo : public FPDF_FORMFILLINFO {
    PdfDocument* doc;
};

static void FFI_Invalidate(FPDF_FORMFILLINFO* pThis, FPDF_PAGE page, double left, double top, double right, double bottom) {
    (void)page;
    auto info = static_cast<MyFormFillInfo*>(pThis);
    auto doc = info->doc;
    if (doc && doc->GetFormCallback()) {
        int pageIndex = -1;
        doc->GetFormCallback()->Invalidate(pageIndex, left, top, right, bottom);
    }
}

static void FFI_SetCursor(FPDF_FORMFILLINFO* pThis, int nCursorType) {
    auto info = static_cast<MyFormFillInfo*>(pThis);
    auto doc = info->doc;
    if (doc && doc->GetFormCallback()) doc->GetFormCallback()->SetCursor(nCursorType);
}

static int FFI_SetTimer(FPDF_FORMFILLINFO* pThis, int uElapse, TimerCallback lpTimerFunc) {
    auto info = static_cast<MyFormFillInfo*>(pThis);
    auto doc = info->doc;
    if (doc && doc->GetFormCallback()) return doc->GetFormCallback()->SetTimer(uElapse, lpTimerFunc);
    return 0;
}

static void FFI_KillTimer(FPDF_FORMFILLINFO* pThis, int nTimerID) {
    auto info = static_cast<MyFormFillInfo*>(pThis);
    auto doc = info->doc;
    if (doc && doc->GetFormCallback()) doc->GetFormCallback()->KillTimer(nTimerID);
}

static void FFI_SetTextFieldFocus(FPDF_FORMFILLINFO* pThis, FPDF_WIDESTRING value, unsigned long valueLen, FPDF_BOOL is_focus) {
    auto info = static_cast<MyFormFillInfo*>(pThis);
    auto doc = info->doc;
    if (doc && doc->GetFormCallback()) {
        std::wstring str(reinterpret_cast<const wchar_t*>(value), valueLen);
        doc->GetFormCallback()->SetTextFieldFocus(str, is_focus != 0);
    }
}

static void FFI_OnFocusChange(FPDF_FORMFILLINFO* pThis, FPDF_ANNOTATION annot, int page_index) {
    (void)pThis;
    (void)annot;
    (void)page_index;
    // Optional
}

bool PdfDocument::InitializeFormFill(std::shared_ptr<core::interfaces::dom::IFormFillCallback> callback) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_formHandle) return true; // Already initialized
    
    m_formCallback = callback;
    
    MyFormFillInfo* info = new MyFormFillInfo();
    memset(info, 0, sizeof(MyFormFillInfo));
    info->version = 2; // version 2 is for form fill
    info->doc = this;
    info->FFI_Invalidate = FFI_Invalidate;
    info->FFI_SetCursor = FFI_SetCursor;
    info->FFI_SetTimer = FFI_SetTimer;
    info->FFI_KillTimer = FFI_KillTimer;
    info->FFI_SetTextFieldFocus = FFI_SetTextFieldFocus;
    info->FFI_OnFocusChange = FFI_OnFocusChange;
    
    m_formFillInfo = info;
    m_formHandle = FPDFDOC_InitFormFillEnvironment(m_doc, info);
    
    return m_formHandle != nullptr;
}

void PdfDocument::ShutdownFormFill() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_formHandle) {
        FPDFDOC_ExitFormFillEnvironment((FPDF_FORMHANDLE)m_formHandle);
        m_formHandle = nullptr;
    }
    if (m_formFillInfo) {
        delete static_cast<MyFormFillInfo*>(m_formFillInfo);
        m_formFillInfo = nullptr;
    }
    m_formCallback.reset();
}

std::vector<std::shared_ptr<core::interfaces::dom::IFormField>> PdfDocument::GetFormFields() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<std::shared_ptr<core::interfaces::dom::IFormField>> fields;
    
    int pc = PageCount();
    for (int i = 0; i < pc; ++i) {
        FPDF_PAGE page = FPDF_LoadPage(m_doc, i);
        if (page) {
            int annotCount = FPDFPage_GetAnnotCount(page);
            for (int j = 0; j < annotCount; ++j) {
                FPDF_ANNOTATION annot = FPDFPage_GetAnnot(page, j);
                if (annot) {
                    if (FPDFAnnot_GetSubtype(annot) == FPDF_ANNOT_WIDGET) {
                        fields.push_back(std::make_shared<PdfFormField>((FPDF_FORMHANDLE)m_formHandle, page, annot, &m_mutex));
                    }
                }
            }
            FPDF_ClosePage(page);
        }
    }
    
    return fields;
}

bool PdfDocument::OnLButtonDown(int pageIndex, double page_x, double page_y, int modifiers) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_formHandle || m_openPages.find(pageIndex) == m_openPages.end()) return false;
    return FORM_OnLButtonDown((FPDF_FORMHANDLE)m_formHandle, m_openPages[pageIndex], modifiers, page_x, page_y) != 0;
}

bool PdfDocument::OnLButtonUp(int pageIndex, double page_x, double page_y, int modifiers) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_formHandle || m_openPages.find(pageIndex) == m_openPages.end()) return false;
    return FORM_OnLButtonUp((FPDF_FORMHANDLE)m_formHandle, m_openPages[pageIndex], modifiers, page_x, page_y) != 0;
}

bool PdfDocument::OnMouseMove(int pageIndex, double page_x, double page_y, int modifiers) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_formHandle || m_openPages.find(pageIndex) == m_openPages.end()) return false;
    return FORM_OnMouseMove((FPDF_FORMHANDLE)m_formHandle, m_openPages[pageIndex], modifiers, page_x, page_y) != 0;
}

bool PdfDocument::OnKeyDown(int keyCode, int modifiers) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_formHandle) return false;
    FPDF_PAGE page = nullptr;
    if (!m_openPages.empty()) page = m_openPages.begin()->second;
    if (!page) return false;
    return FORM_OnKeyDown((FPDF_FORMHANDLE)m_formHandle, page, keyCode, modifiers) != 0;
}

bool PdfDocument::OnChar(int charCode, int modifiers) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_formHandle) return false;
    FPDF_PAGE page = nullptr;
    if (!m_openPages.empty()) page = m_openPages.begin()->second;
    if (!page) return false;
    return FORM_OnChar((FPDF_FORMHANDLE)m_formHandle, page, charCode, modifiers) != 0;
}

void PdfDocument::KillFocus() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_formHandle) return;
    FORM_ForceToKillFocus((FPDF_FORMHANDLE)m_formHandle);
}

