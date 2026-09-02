#pragma once
#include "core/interfaces/dom/ITextObject.h"
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <memory>
#include <mutex>

class PdfPage;

class PdfTextObject : public core::interfaces::dom::ITextObject {
public:
    PdfTextObject(FPDF_DOCUMENT doc, FPDF_PAGE page, FPDF_PAGEOBJECT textObj, std::recursive_mutex* docMutex = nullptr);
    ~PdfTextObject() override;

    std::wstring GetText() const override;
    bool SetText(const std::wstring& text) override;
    bool SetLines(const std::vector<core::interfaces::dom::TextLineData>& lines) override;

    float GetFontSize() const override;
    bool SetFontSize(float size) override;

    RectF GetBounds() const override;
    
    Matrix3x2F GetTransform() const override;
    bool SetTransform(const Matrix3x2F& matrix) override;
    
    std::string GetFontName() const override;
    
    void GetColor(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const override;
    bool SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;
    
    uint64_t GetId() const override;

    std::vector<FPDF_PAGEOBJECT> GetHandles() const { return m_textObjs; }
    void SetAttached(bool attached) { m_isAttached = attached; }
    void AddHandle(FPDF_PAGEOBJECT handle) { if (handle) m_textObjs.push_back(handle); }
    void SetExplicitBounds(const RectF& bounds) { m_explicitBounds = bounds; }

private:
    FPDF_DOCUMENT m_doc;
    FPDF_PAGE m_page;
    std::vector<FPDF_PAGEOBJECT> m_textObjs;
    bool m_isAttached = true;
    RectF m_explicitBounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::recursive_mutex* m_docMutex = nullptr; // Serializes FPDF_* against the RenderWorker
};
