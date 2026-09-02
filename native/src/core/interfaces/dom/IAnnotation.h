#pragma once
#include "core/Geometry.h"
#include <string>
#include <memory>
#include <vector>

namespace core {
namespace interfaces {
namespace dom {

enum class AnnotationType {
    Unknown = 0,
    Text = 1,      // FPDF_ANNOT_TEXT (Sticky Note)
    Link = 2,      // FPDF_ANNOT_LINK
    FreeText = 3,  // FPDF_ANNOT_FREETEXT
    Line = 4,      // FPDF_ANNOT_LINE
    Square = 5,    // FPDF_ANNOT_SQUARE (Rectangle)
    Circle = 6,    // FPDF_ANNOT_CIRCLE (Ellipse)
    Polygon = 7,   // FPDF_ANNOT_POLYGON
    Polyline = 8,  // FPDF_ANNOT_POLYLINE
    Highlight = 9, // FPDF_ANNOT_HIGHLIGHT
    Underline = 10,// FPDF_ANNOT_UNDERLINE
    Squiggly = 11, // FPDF_ANNOT_SQUIGGLY
    StrikeOut = 12,// FPDF_ANNOT_STRIKEOUT
    Stamp = 13,    // FPDF_ANNOT_STAMP
    Caret = 14,    // FPDF_ANNOT_CARET
    Ink = 15,      // FPDF_ANNOT_INK
    Popup = 16     // FPDF_ANNOT_POPUP
};

enum class LineEnding {
    None,
    Square,
    Circle,
    Diamond,
    OpenArrow,
    ClosedArrow,
    Butt,
    ROpenArrow,
    RClosedArrow,
    Slash
};

struct LineGeometry {
    PointF start = {0, 0};
    PointF end = {0, 0};
    LineEnding startEnding = LineEnding::None;
    LineEnding endEnding = LineEnding::None;
};

class IAnnotation {
public:
    virtual ~IAnnotation() = default;

    virtual std::string GetId() const = 0;
    virtual AnnotationType GetType() const = 0;

    virtual RectF GetBounds() const = 0;
    virtual void SetBounds(const RectF& bounds) = 0;
    
    virtual double GetRotation() const = 0;
    virtual void SetRotation(double degrees) = 0;

    virtual std::string GetContents() const = 0;
    virtual void SetContents(const std::string& contents) = 0;
    virtual void GenerateAppearanceStream() = 0;

    virtual std::vector<QuadF> GetQuadPoints() const = 0;
    virtual void SetQuadPoints(const std::vector<QuadF>& quads) = 0;
    
    virtual bool GetColor(int& r, int& g, int& b, int& a) const = 0;
    virtual void SetColor(int r, int g, int b, int a) = 0;
    
    virtual bool GetFillColor(int& r, int& g, int& b, int& a) const = 0;
    virtual void SetFillColor(int r, int g, int b, int a) = 0;
    
    virtual float GetBorderWidth() const = 0;
    virtual void SetBorderWidth(float width) = 0;
    
    virtual float GetOpacity() const = 0;
    virtual void SetOpacity(float opacity) = 0;

    virtual std::string GetAuthor() const = 0;
    virtual void SetAuthor(const std::string& author) = 0;

    virtual std::string GetCreationDate() const = 0;
    virtual void SetCreationDate(const std::string& date) = 0;

    virtual std::string GetModificationDate() const = 0;
    virtual void SetModificationDate(const std::string& date) = 0;

    virtual int GetFlags() const = 0;
    virtual void SetFlags(int flags) = 0;

    virtual bool GetLineGeometry(LineGeometry& outGeometry) const = 0;
    virtual void SetLineGeometry(const LineGeometry& geometry) = 0;

    // Returns true if the annotation has a valid appearance stream and can be rendered
    virtual bool HasAppearance() const = 0;

    // Ink Annotation methods (defaults to no-op for non-ink annotations)
    virtual std::vector<std::vector<PointF>> GetInkList() const { return {}; }
    virtual bool AddInkStroke(const std::vector<PointF>& points) { (void)points; return false; }
    virtual bool ClearInkList() { return false; }
};

} // namespace dom
} // namespace interfaces
} // namespace core
