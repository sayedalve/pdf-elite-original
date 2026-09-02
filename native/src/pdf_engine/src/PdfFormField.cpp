#include "PdfFormField.h"
#include <fpdf_annot.h>
#include <vector>

// PDFium is not thread-safe. These FPDFAnnot_* form-field queries run on the UI
// thread and must serialize with the background RenderWorker. m_docMutex is the
// owning document's recursive_mutex (supplied by PdfDocument::GetFormFields).
#define FORMFIELD_LOCK() \
    std::unique_lock<std::recursive_mutex> _lock; \
    if (m_docMutex) _lock = std::unique_lock<std::recursive_mutex>(*m_docMutex)

PdfFormField::PdfFormField(FPDF_FORMHANDLE formHandle, FPDF_PAGE page, FPDF_ANNOTATION annot, std::recursive_mutex* docMutex)
    : m_formHandle(formHandle), m_page(page), m_annot(annot), m_docMutex(docMutex) {
}

PdfFormField::~PdfFormField() {
    // Annotation handles are owned by the page, no need to free.
}

core::interfaces::dom::FormFieldType PdfFormField::GetType() const {
    FORMFIELD_LOCK();
    int type = FPDFAnnot_GetFormFieldType(m_formHandle, m_annot);
    switch (type) {
        case FPDF_FORMFIELD_PUSHBUTTON: return core::interfaces::dom::FormFieldType::PushButton;
        case FPDF_FORMFIELD_CHECKBOX: return core::interfaces::dom::FormFieldType::CheckBox;
        case FPDF_FORMFIELD_RADIOBUTTON: return core::interfaces::dom::FormFieldType::RadioButton;
        case FPDF_FORMFIELD_COMBOBOX: return core::interfaces::dom::FormFieldType::ComboBox;
        case FPDF_FORMFIELD_LISTBOX: return core::interfaces::dom::FormFieldType::ListBox;
        case FPDF_FORMFIELD_TEXTFIELD: return core::interfaces::dom::FormFieldType::TextField;
        case FPDF_FORMFIELD_SIGNATURE: return core::interfaces::dom::FormFieldType::Signature;
        default: return core::interfaces::dom::FormFieldType::Unknown;
    }
}

std::string PdfFormField::GetName() const {
    FORMFIELD_LOCK();
    unsigned long len = FPDFAnnot_GetFormFieldName(m_formHandle, m_annot, nullptr, 0);
    if (len <= 2) return "";
    std::vector<char> buf(len);
    FPDFAnnot_GetFormFieldName(m_formHandle, m_annot, reinterpret_cast<FPDF_WCHAR*>(buf.data()), len);
    std::wstring wstr(reinterpret_cast<const wchar_t*>(buf.data()), (len / 2) - 1);
    std::string str;
    for (wchar_t c : wstr) str.push_back(static_cast<char>(c));
    return str;
}

std::string PdfFormField::GetAlternateName() const {
    FORMFIELD_LOCK();
    unsigned long len = FPDFAnnot_GetFormFieldAlternateName(m_formHandle, m_annot, nullptr, 0);
    if (len <= 2) return "";
    std::vector<char> buf(len);
    FPDFAnnot_GetFormFieldAlternateName(m_formHandle, m_annot, reinterpret_cast<FPDF_WCHAR*>(buf.data()), len);
    std::wstring wstr(reinterpret_cast<const wchar_t*>(buf.data()), (len / 2) - 1);
    std::string str;
    for (wchar_t c : wstr) str.push_back(static_cast<char>(c));
    return str;
}

std::string PdfFormField::GetMappingName() const {
    return "";
}

std::wstring PdfFormField::GetValue() const {
    FORMFIELD_LOCK();
    unsigned long len = FPDFAnnot_GetFormFieldValue(m_formHandle, m_annot, nullptr, 0);
    if (len <= 2) return L"";
    std::vector<char> buf(len);
    FPDFAnnot_GetFormFieldValue(m_formHandle, m_annot, reinterpret_cast<FPDF_WCHAR*>(buf.data()), len);
    return std::wstring(reinterpret_cast<const wchar_t*>(buf.data()), (len / 2) - 1);
}

std::wstring PdfFormField::GetDefaultValue() const {
    return L"";
}

bool PdfFormField::IsReadOnly() const {
    return (GetFlags() & 1) != 0; // FPDF_FORMFLAG_READONLY is bit 1? We can just use basic flags
}

bool PdfFormField::IsRequired() const {
    return (GetFlags() & 2) != 0;
}

bool PdfFormField::IsNoExport() const {
    return (GetFlags() & 4) != 0;
}

int PdfFormField::GetPageIndex() const {
    return 0; // Stub
}

std::vector<int> PdfFormField::GetSelectedIndices() const {
    return {}; // Stub
}

RectF PdfFormField::GetBounds() const {
    FORMFIELD_LOCK();
    FS_RECTF rect;
    if (FPDFAnnot_GetRect(m_annot, &rect)) {
        return { rect.left, rect.top, rect.right, rect.bottom };
    }
    return { 0, 0, 0, 0 };
}

std::vector<std::wstring> PdfFormField::GetOptions() const {
    FORMFIELD_LOCK();
    int count = FPDFAnnot_GetOptionCount(m_formHandle, m_annot);
    std::vector<std::wstring> opts;
    for (int i = 0; i < count; ++i) {
        unsigned long len = FPDFAnnot_GetOptionLabel(m_formHandle, m_annot, i, nullptr, 0);
        if (len > 2) {
            std::vector<char> buf(len);
            FPDFAnnot_GetOptionLabel(m_formHandle, m_annot, i, reinterpret_cast<FPDF_WCHAR*>(buf.data()), len);
            opts.push_back(std::wstring(reinterpret_cast<const wchar_t*>(buf.data()), (len / 2) - 1));
        }
    }
    return opts;
}

int PdfFormField::GetFlags() const {
    FORMFIELD_LOCK();
    return FPDFAnnot_GetFormFieldFlags(m_formHandle, m_annot);
}
