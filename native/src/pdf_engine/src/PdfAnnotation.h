#pragma once
#include "core/interfaces/dom/IAnnotation.h"
#include <fpdfview.h>
#include <fpdf_annot.h>
#include <mutex>

class PdfPage;

class PdfAnnotation : public core::interfaces::dom::IAnnotation {
public:
    PdfAnnotation(FPDF_ANNOTATION annot, PdfPage* page);
    ~PdfAnnotation() override;

    std::string GetId() const override;
    core::interfaces::dom::AnnotationType GetType() const override;

    RectF GetBounds() const override;
    void SetBounds(const RectF& bounds) override;

    double GetRotation() const override;
    void SetRotation(double degrees) override;

    std::string GetContents() const override;
    void SetContents(const std::string& contents) override;
    void GenerateAppearanceStream() override;

    std::vector<QuadF> GetQuadPoints() const override;
    void SetQuadPoints(const std::vector<QuadF>& quads) override;
    
    bool GetColor(int& r, int& g, int& b, int& a) const override;
    void SetColor(int r, int g, int b, int a) override;
    
    bool GetFillColor(int& r, int& g, int& b, int& a) const override;
    void SetFillColor(int r, int g, int b, int a) override;
    
    float GetBorderWidth() const override;
    void SetBorderWidth(float width) override;

    float GetOpacity() const override;
    void SetOpacity(float opacity) override;

    std::string GetAuthor() const override;
    void SetAuthor(const std::string& author) override;

    std::string GetCreationDate() const override;
    void SetCreationDate(const std::string& date) override;

    std::string GetModificationDate() const override;
    void SetModificationDate(const std::string& date) override;

    int GetFlags() const override;
    void SetFlags(int flags) override;

    bool GetLineGeometry(core::interfaces::dom::LineGeometry& outGeometry) const override;
    void SetLineGeometry(const core::interfaces::dom::LineGeometry& geometry) override;

    bool HasAppearance() const override;

    std::vector<std::vector<PointF>> GetInkList() const override;
    bool AddInkStroke(const std::vector<PointF>& points) override;
    bool ClearInkList() override;

    FPDF_ANNOTATION GetHandle() const { return m_annot; }

private:
    FPDF_ANNOTATION m_annot = nullptr;
    PdfPage* m_page = nullptr;
    std::recursive_mutex* m_docMutex = nullptr;
    std::string m_id; // Cached ID
    float m_opacity = 1.0f;
    bool m_hasOpacity = false;
    core::interfaces::dom::LineGeometry m_lineGeometry;
    bool m_hasLineGeometry = false;
    std::vector<std::vector<PointF>> m_inkList;
};
