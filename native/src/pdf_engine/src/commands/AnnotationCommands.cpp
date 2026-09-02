#include "AnnotationCommands.h"
#include "core/interfaces/dom/IPage.h"

namespace pdf_engine {
namespace commands {

// -------------------------------------------------------------------
// AddAnnotationCommand
// -------------------------------------------------------------------

AddAnnotationCommand::AddAnnotationCommand(core::interfaces::dom::IDocument* doc, int pageIndex, core::interfaces::dom::AnnotationType type, const RectF& bounds)
    : m_doc(doc), m_pageIndex(pageIndex), m_type(type), m_bounds(bounds) {
}

bool AddAnnotationCommand::Execute() {
    if (!m_doc) return false;
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page) return false;

    if (!m_annot) {
        m_annot = page->CreateAnnotation(m_type);
        if (m_annot) {
            m_annot->SetBounds(m_bounds);
            if (!m_quads.empty()) {
                m_annot->SetQuadPoints(m_quads);
            }
            if (m_hasColor) {
                m_annot->SetColor(std::get<0>(m_color), std::get<1>(m_color), std::get<2>(m_color), std::get<3>(m_color));
            }
            if (m_hasOpacity) {
                m_annot->SetOpacity(m_opacity);
            }
            if (m_hasBorderWidth) {
                m_annot->SetBorderWidth(m_borderWidth);
            }
            if (m_hasLineGeom) {
                m_annot->SetLineGeometry(m_lineGeom);
            }
        }
    } else {
        // Redo scenario: the annot might have been removed. We need to recreate it.
        auto newAnnot = page->CreateAnnotation(m_type);
        if (newAnnot) {
            newAnnot->SetBounds(m_annot->GetBounds());
            newAnnot->SetRotation(m_annot->GetRotation());
            newAnnot->SetContents(m_annot->GetContents());
            auto quads = m_annot->GetQuadPoints();
            if (!quads.empty()) {
                newAnnot->SetQuadPoints(quads);
            }
            int r,g,b,a;
            if (m_annot->GetColor(r,g,b,a)) newAnnot->SetColor(r,g,b,a);
            if (m_annot->GetFillColor(r,g,b,a)) newAnnot->SetFillColor(r,g,b,a);
            newAnnot->SetBorderWidth(m_annot->GetBorderWidth());
            newAnnot->SetOpacity(m_annot->GetOpacity());
            newAnnot->SetAuthor(m_annot->GetAuthor());
            newAnnot->SetCreationDate(m_annot->GetCreationDate());
            newAnnot->SetModificationDate(m_annot->GetModificationDate());
            newAnnot->SetFlags(m_annot->GetFlags());
            
            core::interfaces::dom::LineGeometry lg;
            if (m_annot->GetLineGeometry(lg)) {
                newAnnot->SetLineGeometry(lg);
            }
            auto inks = m_annot->GetInkList();
            for (const auto& stroke : inks) {
                newAnnot->AddInkStroke(stroke);
            }
            
            m_annot = newAnnot;
        }
    }
    if (page) page->GenerateContent();
    return m_annot != nullptr;
}

bool AddAnnotationCommand::Undo() {
    if (!m_doc || !m_annot) return false;
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page) return false;
    
    return page->RemoveAnnotation(m_annot);
}

// -------------------------------------------------------------------
// MoveAnnotationCommand
// -------------------------------------------------------------------

MoveAnnotationCommand::MoveAnnotationCommand(std::shared_ptr<core::interfaces::dom::IAnnotation> annot, const RectF& oldBounds, const RectF& newBounds, bool hasOldLineGeom, const core::interfaces::dom::LineGeometry& oldLineGeom, bool hasNewLineGeom, const core::interfaces::dom::LineGeometry& newLineGeom)
    : m_annot(annot), m_oldBounds(oldBounds), m_newBounds(newBounds), m_hasLineGeom(hasOldLineGeom && hasNewLineGeom), m_oldLineGeom(oldLineGeom), m_newLineGeom(newLineGeom) {
}

bool MoveAnnotationCommand::Execute() {
    if (!m_annot) return false;
    m_annot->SetBounds(m_newBounds);
    if (m_hasLineGeom) {
        m_annot->SetLineGeometry(m_newLineGeom);
    }
    if (m_annot) m_annot->GenerateAppearanceStream();
    return true;
}

bool MoveAnnotationCommand::Undo() {
    if (!m_annot) return false;
    m_annot->SetBounds(m_oldBounds);
    if (m_hasLineGeom) {
        m_annot->SetLineGeometry(m_oldLineGeom);
    }
    if (m_annot) m_annot->GenerateAppearanceStream();
    return true;
}

// -------------------------------------------------------------------
// ResizeAnnotationCommand
// -------------------------------------------------------------------

ResizeAnnotationCommand::ResizeAnnotationCommand(std::shared_ptr<core::interfaces::dom::IAnnotation> annot, const RectF& oldBounds, const RectF& newBounds, bool hasOldLineGeom, const core::interfaces::dom::LineGeometry& oldLineGeom, bool hasNewLineGeom, const core::interfaces::dom::LineGeometry& newLineGeom)
    : m_annot(annot), m_oldBounds(oldBounds), m_newBounds(newBounds), m_hasLineGeom(hasOldLineGeom && hasNewLineGeom), m_oldLineGeom(oldLineGeom), m_newLineGeom(newLineGeom) {
}

bool ResizeAnnotationCommand::Execute() {
    if (!m_annot) return false;
    m_annot->SetBounds(m_newBounds);
    if (m_hasLineGeom) {
        m_annot->SetLineGeometry(m_newLineGeom);
    }
    if (m_annot) m_annot->GenerateAppearanceStream();
    return true;
}

bool ResizeAnnotationCommand::Undo() {
    if (!m_annot) return false;
    m_annot->SetBounds(m_oldBounds);
    if (m_hasLineGeom) {
        m_annot->SetLineGeometry(m_oldLineGeom);
    }
    if (m_annot) m_annot->GenerateAppearanceStream();
    return true;
}

// -------------------------------------------------------------------
// ModifyAnnotationPropertiesCommand
// -------------------------------------------------------------------

ModifyAnnotationPropertiesCommand::ModifyAnnotationPropertiesCommand(std::shared_ptr<core::interfaces::dom::IAnnotation> annot, const AnnotationState& oldState, const AnnotationState& newState)
    : m_annot(annot), m_oldState(oldState), m_newState(newState) {
}

void ModifyAnnotationPropertiesCommand::ApplyState(const AnnotationState& state) {
    if (!m_annot) return;
    if (state.hasColor) {
        m_annot->SetColor(state.colorR, state.colorG, state.colorB, state.colorA);
    }
    if (state.hasFillColor) {
        m_annot->SetFillColor(state.fillColorR, state.fillColorG, state.fillColorB, state.fillColorA);
    }
    m_annot->SetBorderWidth(state.borderWidth);
    m_annot->SetOpacity(state.opacity);
    m_annot->SetContents(state.contents);
    m_annot->SetAuthor(state.author);
    m_annot->SetModificationDate(state.modificationDate);
    m_annot->SetFlags(state.flags);
    if (state.hasLineGeom) {
        m_annot->SetLineGeometry(state.lineGeom);
    }
}

bool ModifyAnnotationPropertiesCommand::Execute() {
    if (!m_annot) return false;
    ApplyState(m_newState);
    if (m_annot) m_annot->GenerateAppearanceStream();
    return true;
}

bool ModifyAnnotationPropertiesCommand::Undo() {
    if (!m_annot) return false;
    ApplyState(m_oldState);
    if (m_annot) m_annot->GenerateAppearanceStream();
    return true;
}

// -------------------------------------------------------------------
// AddInkAnnotationCommand
// -------------------------------------------------------------------

AddInkAnnotationCommand::AddInkAnnotationCommand(core::interfaces::dom::IDocument* doc, int pageIndex, const std::vector<std::vector<PointF>>& strokes, const RectF& bounds, int r, int g, int b, int a, float borderWidth)
    : m_doc(doc), m_pageIndex(pageIndex), m_strokes(strokes), m_bounds(bounds), m_r(r), m_g(g), m_b(b), m_a(a), m_borderWidth(borderWidth) {
}

bool AddInkAnnotationCommand::Execute() {
    if (!m_doc) return false;
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page) return false;

    if (!m_annot) {
        m_annot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Ink);
        if (m_annot) {
            m_annot->SetBounds(m_bounds);
            m_annot->SetColor(m_r, m_g, m_b, m_a);
            m_annot->SetBorderWidth(m_borderWidth);
            for (const auto& stroke : m_strokes) {
                m_annot->AddInkStroke(stroke);
            }
        }
    } else {
        auto newAnnot = page->CreateAnnotation(core::interfaces::dom::AnnotationType::Ink);
        if (newAnnot) {
            newAnnot->SetBounds(m_bounds);
            newAnnot->SetColor(m_r, m_g, m_b, m_a);
            newAnnot->SetBorderWidth(m_borderWidth);
            for (const auto& stroke : m_strokes) {
                newAnnot->AddInkStroke(stroke);
            }
            m_annot = newAnnot;
        }
    }
    return m_annot != nullptr;
}

bool AddInkAnnotationCommand::Undo() {
    if (!m_doc || !m_annot) return false;
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page) return false;

    return page->RemoveAnnotation(m_annot);
}

// -------------------------------------------------------------------
// DeleteAnnotationCommand
// -------------------------------------------------------------------

DeleteAnnotationCommand::DeleteAnnotationCommand(core::interfaces::dom::IDocument* doc, int pageIndex, std::shared_ptr<core::interfaces::dom::IAnnotation> annot)
    : m_doc(doc), m_pageIndex(pageIndex), m_annot(annot) {
}

bool DeleteAnnotationCommand::Execute() {
    if (!m_doc || !m_annot) return false;
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page) return false;

    if (!m_deleted) {
        // Save state before deleting
        m_savedState.type = m_annot->GetType();
        m_savedState.bounds = m_annot->GetBounds();
        m_savedState.contents = m_annot->GetContents();
        m_savedState.quads = m_annot->GetQuadPoints();
        
        m_savedState.hasColor = m_annot->GetColor(m_savedState.colorR, m_savedState.colorG, m_savedState.colorB, m_savedState.colorA);
        m_savedState.hasFillColor = m_annot->GetFillColor(m_savedState.fillColorR, m_savedState.fillColorG, m_savedState.fillColorB, m_savedState.fillColorA);
        m_savedState.borderWidth = m_annot->GetBorderWidth();
        m_savedState.opacity = m_annot->GetOpacity();
        m_savedState.author = m_annot->GetAuthor();
        m_savedState.creationDate = m_annot->GetCreationDate();
        m_savedState.modificationDate = m_annot->GetModificationDate();
        m_savedState.flags = m_annot->GetFlags();
        
        m_savedState.hasLineGeom = m_annot->GetLineGeometry(m_savedState.lineGeom);
        m_savedState.inkList = m_annot->GetInkList();
        
        m_deleted = true;
    }

    return page->RemoveAnnotation(m_annot);
}

bool DeleteAnnotationCommand::Undo() {
    if (!m_doc || !m_deleted) return false;
    auto page = m_doc->GetPage(m_pageIndex);
    if (!page) return false;

    // Restore from saved state
    auto restored = page->CreateAnnotation(m_savedState.type);
    if (restored) {
        restored->SetBounds(m_savedState.bounds);
        restored->SetContents(m_savedState.contents);
        if (!m_savedState.quads.empty()) {
            restored->SetQuadPoints(m_savedState.quads);
        }
        
        if (m_savedState.hasColor) {
            restored->SetColor(m_savedState.colorR, m_savedState.colorG, m_savedState.colorB, m_savedState.colorA);
        }
        if (m_savedState.hasFillColor) {
            restored->SetFillColor(m_savedState.fillColorR, m_savedState.fillColorG, m_savedState.fillColorB, m_savedState.fillColorA);
        }
        restored->SetBorderWidth(m_savedState.borderWidth);
        restored->SetOpacity(m_savedState.opacity);
        restored->SetAuthor(m_savedState.author);
        restored->SetCreationDate(m_savedState.creationDate);
        restored->SetModificationDate(m_savedState.modificationDate);
        restored->SetFlags(m_savedState.flags);
        
        if (m_savedState.hasLineGeom) {
            restored->SetLineGeometry(m_savedState.lineGeom);
        }
        for (const auto& stroke : m_savedState.inkList) {
            restored->AddInkStroke(stroke);
        }

        m_annot = restored;
        if (m_annot) m_annot->GenerateAppearanceStream();
        return true;
    }
    return false;
}

} // namespace commands
} // namespace pdf_engine



