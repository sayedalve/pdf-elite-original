#pragma once
#include "core/interfaces/dom/ICommand.h"
#include "core/interfaces/dom/IAnnotation.h"
#include "core/interfaces/dom/IDocument.h"
#include <memory>
#include <string>

namespace pdf_engine {
namespace commands {

struct AnnotationState {
    core::interfaces::dom::AnnotationType type = core::interfaces::dom::AnnotationType::Unknown;
    RectF bounds;
    std::string contents;
    std::vector<QuadF> quads;
    
    // Extended properties
    int colorR = 0, colorG = 0, colorB = 0, colorA = 0;
    bool hasColor = false;
    int fillColorR = 0, fillColorG = 0, fillColorB = 0, fillColorA = 0;
    bool hasFillColor = false;
    float borderWidth = 1.0f;
    float opacity = 1.0f;
    std::string author;
    std::string creationDate;
    std::string modificationDate;
    int flags = 0;
    
    // Geometry
    bool hasLineGeom = false;
    core::interfaces::dom::LineGeometry lineGeom;

    // Ink strokes
    std::vector<std::vector<PointF>> inkList;
};

class AddAnnotationCommand : public core::interfaces::dom::ICommand {
public:
    AddAnnotationCommand(core::interfaces::dom::IDocument* doc, int pageIndex, core::interfaces::dom::AnnotationType type, const RectF& bounds);
    ~AddAnnotationCommand() override = default;

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Add Annotation"; }
    
    // Allows retrieving the added annotation
    std::shared_ptr<core::interfaces::dom::IAnnotation> GetAnnotation() const { return m_annot; }

    void SetQuads(const std::vector<QuadF>& quads) { m_quads = quads; }
    void SetColor(int r, int g, int b, int a) { m_color = {r, g, b, a}; m_hasColor = true; }
    void SetOpacity(float opacity) { m_opacity = opacity; m_hasOpacity = true; }
    void SetBorderWidth(float width) { m_borderWidth = width; m_hasBorderWidth = true; }
    void SetLineGeometry(const core::interfaces::dom::LineGeometry& geom) { m_lineGeom = geom; m_hasLineGeom = true; }

private:
    core::interfaces::dom::IDocument* m_doc;
    int m_pageIndex;
    core::interfaces::dom::AnnotationType m_type;
    RectF m_bounds;
    std::vector<QuadF> m_quads;
    std::tuple<int, int, int, int> m_color;
    bool m_hasColor = false;
    float m_opacity = 1.0f;
    bool m_hasOpacity = false;
    float m_borderWidth = 1.0f;
    bool m_hasBorderWidth = false;
    core::interfaces::dom::LineGeometry m_lineGeom;
    bool m_hasLineGeom = false;

    
    std::shared_ptr<core::interfaces::dom::IAnnotation> m_annot;
};

class MoveAnnotationCommand : public core::interfaces::dom::ICommand {
public:
    MoveAnnotationCommand(std::shared_ptr<core::interfaces::dom::IAnnotation> annot, const RectF& oldBounds, const RectF& newBounds, bool hasOldLineGeom = false, const core::interfaces::dom::LineGeometry& oldLineGeom = {}, bool hasNewLineGeom = false, const core::interfaces::dom::LineGeometry& newLineGeom = {});

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Move Annotation"; }

private:
    std::shared_ptr<core::interfaces::dom::IAnnotation> m_annot;
    RectF m_oldBounds;
    RectF m_newBounds;
    bool m_hasLineGeom = false;
    core::interfaces::dom::LineGeometry m_oldLineGeom;
    core::interfaces::dom::LineGeometry m_newLineGeom;
};

class ResizeAnnotationCommand : public core::interfaces::dom::ICommand {
public:
    ResizeAnnotationCommand(std::shared_ptr<core::interfaces::dom::IAnnotation> annot, const RectF& oldBounds, const RectF& newBounds, bool hasOldLineGeom = false, const core::interfaces::dom::LineGeometry& oldLineGeom = {}, bool hasNewLineGeom = false, const core::interfaces::dom::LineGeometry& newLineGeom = {});

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Resize Annotation"; }

private:
    std::shared_ptr<core::interfaces::dom::IAnnotation> m_annot;
    RectF m_oldBounds;
    RectF m_newBounds;
    bool m_hasLineGeom = false;
    core::interfaces::dom::LineGeometry m_oldLineGeom;
    core::interfaces::dom::LineGeometry m_newLineGeom;
};

class ModifyAnnotationPropertiesCommand : public core::interfaces::dom::ICommand {
public:
    ModifyAnnotationPropertiesCommand(std::shared_ptr<core::interfaces::dom::IAnnotation> annot, const AnnotationState& oldState, const AnnotationState& newState);

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Modify Annotation Properties"; }

private:
    void ApplyState(const AnnotationState& state);

    std::shared_ptr<core::interfaces::dom::IAnnotation> m_annot;
    AnnotationState m_oldState;
    AnnotationState m_newState;
};

class AddInkAnnotationCommand : public core::interfaces::dom::ICommand {
public:
    AddInkAnnotationCommand(core::interfaces::dom::IDocument* doc, int pageIndex, const std::vector<std::vector<PointF>>& strokes, const RectF& bounds, int r = 0, int g = 0, int b = 0, int a = 255, float borderWidth = 2.0f);
    ~AddInkAnnotationCommand() override = default;

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Add Ink Annotation"; }

    std::shared_ptr<core::interfaces::dom::IAnnotation> GetAnnotation() const { return m_annot; }

private:
    core::interfaces::dom::IDocument* m_doc;
    int m_pageIndex;
    std::vector<std::vector<PointF>> m_strokes;
    RectF m_bounds;
    int m_r, m_g, m_b, m_a;
    float m_borderWidth;

    std::shared_ptr<core::interfaces::dom::IAnnotation> m_annot;
};

class DeleteAnnotationCommand : public core::interfaces::dom::ICommand {
public:
    DeleteAnnotationCommand(core::interfaces::dom::IDocument* doc, int pageIndex, std::shared_ptr<core::interfaces::dom::IAnnotation> annot);

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Delete Annotation"; }

private:
    core::interfaces::dom::IDocument* m_doc;
    int m_pageIndex;
    std::shared_ptr<core::interfaces::dom::IAnnotation> m_annot;
    AnnotationState m_savedState;
    bool m_deleted = false;
};

} // namespace commands
} // namespace pdf_engine
