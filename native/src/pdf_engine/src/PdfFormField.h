#pragma once
#include "core/interfaces/dom/IFormField.h"
#include <fpdfview.h>
#include <string>
#include <vector>
#include <mutex>

class PdfFormField : public core::interfaces::dom::IFormField {
public:
    PdfFormField(FPDF_FORMHANDLE formHandle, FPDF_PAGE page, FPDF_ANNOTATION annot, std::recursive_mutex* docMutex = nullptr);
    ~PdfFormField() override;

    core::interfaces::dom::FormFieldType GetType() const override;
    std::string GetName() const override;
    std::string GetAlternateName() const override;
    std::string GetMappingName() const override;
    std::wstring GetValue() const override;
    std::wstring GetDefaultValue() const override;
    
    RectF GetBounds() const override;
    
    bool IsReadOnly() const override;
    bool IsRequired() const override;
    bool IsNoExport() const override;
    int GetPageIndex() const override;
    int GetFlags() const override;

    std::vector<std::wstring> GetOptions() const override;
    std::vector<int> GetSelectedIndices() const override;

private:
    FPDF_FORMHANDLE m_formHandle;
    FPDF_PAGE m_page;
    FPDF_ANNOTATION m_annot;
    std::recursive_mutex* m_docMutex = nullptr; // Serializes FPDF_* against the RenderWorker
};
