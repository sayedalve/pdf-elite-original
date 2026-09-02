#pragma warning(disable: 4244)
#include "InteractionManager.h"
#include "AnnotationSelectableObject.h"
#include <cmath>
#include "commands/TextCommands.h"
#include "../../../pdf_engine/src/commands/AnnotationCommands.h"
#include "../controls/IconRenderer.h"
namespace ui {
namespace interaction {

InteractionManager::InteractionManager() {
    m_selection.onSelectionChanged = [this]() {
        if (onSelectionChanged) onSelectionChanged();
    };
}
InteractionManager::~InteractionManager() {}

void InteractionManager::SetObjects(const std::vector<std::shared_ptr<ISelectableObject>>& objects) {
    m_objects = objects;
    m_selection.Clear();
}

void InteractionManager::AddObject(std::shared_ptr<ISelectableObject> obj) {
    m_objects.push_back(obj);
}

void InteractionManager::AddObjects(const std::vector<std::shared_ptr<ISelectableObject>>& objects) {
    m_objects.insert(m_objects.end(), objects.begin(), objects.end());
}

void InteractionManager::RemoveObject(const std::string& id) {
    m_selection.Deselect(id);
    auto it = std::remove_if(m_objects.begin(), m_objects.end(), [&](const auto& o) { return !o || o->GetId() == id; });
    m_objects.erase(it, m_objects.end());
}

void InteractionManager::RemoveObjectsForPage(int pageIndex) {
    // Deselect the page's objects BEFORE removing them, iterating the live
    // vector. This used to run the deselect loop over the [it, end) tail left
    // behind by std::remove_if -- but remove_if move-assigns the *kept*
    // elements toward the front, so that tail holds moved-from (null)
    // shared_ptrs, not the removed objects. Dereferencing (*i)->GetId() on a
    // moved-from/null element caused a null-pointer access violation while
    // scrolling a page that had selectable objects out of view. Iterate the
    // intact objects first, then mutate.
    for (const auto& o : m_objects) {
        if (o && o->GetPageIndex() == pageIndex) {
            m_selection.Deselect(o->GetId());
        }
    }
    m_objects.erase(
        std::remove_if(m_objects.begin(), m_objects.end(),
            [&](const auto& o) { return !o || o->GetPageIndex() == pageIndex; }),
        m_objects.end());
}

std::vector<std::shared_ptr<ISelectableObject>> InteractionManager::GetSelection() const {
    return m_selection.GetSelected();
}

void InteractionManager::EnterTextEditMode(std::shared_ptr<TextSelectableObject> obj) {
    if (m_isEditingText) {
        CommitTextEdit();
    }
    m_isEditingText = true;
    m_editingTextObj = obj;
    m_textEditor.SetActive(true);
    
    auto textObj = obj->GetTextObject();
    m_textEditor.SetText(textObj->GetText());
    
    auto fontName = textObj->GetFontName();
    m_textEditor.SetFont(std::wstring(fontName.begin(), fontName.end()), textObj->GetFontSize());
    
    uint8_t r, g, b, a;
    textObj->GetColor(r, g, b, a);
    m_textEditor.SetColor(D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f));
    m_textEditor.SetCaretColor(D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f));
    
    m_textEditor.SetBounds(GetObjectBoundsInView(*obj));
    
    // Phase F: Map PDFTextObject matrix transformations to the DirectWrite TextLayout model
    auto m = textObj->GetTransform();
    m_textEditor.SetTransform(D2D1::Matrix3x2F(m.a, m.b, m.c, m.d, m.e, m.f));
    
    // Capture original lines for undo
    m_editingOriginalLines.clear();
    for (const auto& line : m_textEditor.GetLines()) {
        core::interfaces::dom::TextLineData d;
        d.text = line.text;
        d.x = line.x;
        d.y = line.y;
        d.width = line.width;
        d.height = line.height;
        m_editingOriginalLines.push_back(d);
    }
    if (invalidateView) invalidateView();
}

void InteractionManager::EnterNewTextMode(double vx, double vy) {
    if (m_isEditingText || m_isAddingText) {
        CommitTextEdit();
    }
    
    double px = 0, py = 0;
    int pageIndex = -1;
    if (viewToPage) viewToPage(vx, vy, px, py, pageIndex);
    
    if (pageIndex < 0) return;
    
    m_isAddingText = true;
    m_addingTextPageIndex = pageIndex;
    m_addingTextPx = px;
    m_addingTextPy = py;
    
    m_textEditor.SetActive(true);
    m_textEditor.SetText(L"");
    m_textEditor.SetFont(L"Arial", 12.0f);
    m_textEditor.SetColor(D2D1::ColorF(0, 0, 0, 1.0f));
    m_textEditor.SetCaretColor(D2D1::ColorF(0, 0, 0, 1.0f));
    
    // Set a default width/height for the editor box based on page coordinates
    double startVX = 0, startVY = 0, endVX = 0, endVY = 0;
    if (pageToView) {
        pageToView(px, py, pageIndex, startVX, startVY);
        pageToView(px + 200, py + 50, pageIndex, endVX, endVY);
    }
    
    m_textEditor.SetBounds(D2D1::RectF(static_cast<float>(startVX), static_cast<float>(startVY), static_cast<float>(endVX), static_cast<float>(endVY)));
    
    if (invalidateView) invalidateView();
}
void InteractionManager::EnterAnnotationEditMode(std::shared_ptr<AnnotationSelectableObject> obj) {
    if (m_isEditingText || m_isEditingAnnotationText) {
        CommitTextEdit();
    }
    m_isEditingAnnotationText = true;
    m_editingAnnotObj = obj;
    m_textEditor.SetActive(true);

    auto annot = obj->GetAnnotation();
    std::string contents = annot->GetContents();
    std::wstring wcontents(contents.begin(), contents.end());
    m_textEditor.SetText(wcontents);
    m_textEditor.SetFont(L"Arial", 12.0f);
    m_textEditor.SetColor(D2D1::ColorF(0, 0, 0, 1.0f));
    m_textEditor.SetCaretColor(D2D1::ColorF(0, 0, 0, 1.0f));
    m_textEditor.SetBounds(GetObjectBoundsInView(*obj));

    if (invalidateView) invalidateView();
}

void InteractionManager::CommitTextEdit() {
    if (!m_isEditingText && !m_isAddingText && !m_isEditingAnnotationText) return;
    
    std::wstring newText = m_textEditor.GetText();
    
    if (m_isEditingAnnotationText && m_editingAnnotObj) {
        auto annot = m_editingAnnotObj->GetAnnotation();
        std::string newContents(newText.begin(), newText.end());
        
        pdf_engine::commands::AnnotationState oldState;
        oldState.type = annot->GetType();
        oldState.contents = annot->GetContents();
        oldState.bounds = annot->GetBounds();

        pdf_engine::commands::AnnotationState newState = oldState;
        newState.contents = newContents;

        auto cmd = std::make_unique<pdf_engine::commands::ModifyAnnotationPropertiesCommand>(
            annot, oldState, newState
        );

        if (onCommandRequested) {
            onCommandRequested(std::move(cmd));
        }
    } else if (m_isAddingText) {
        if (!newText.empty() && onCommandRequested) {
            auto addCmd = std::make_unique<pdf_engine::commands::AddTextCommand>(
                nullptr, // The document will be injected by the PdfViewer
                m_addingTextPageIndex,
                newText,
                RectF{static_cast<float>(m_addingTextPx), static_cast<float>(m_addingTextPy), static_cast<float>(m_addingTextPx + 200.f), static_cast<float>(m_addingTextPy + 50.f)},
                "Arial", 12.0f, 0, 0, 0, 255
            );
            onCommandRequested(std::move(addCmd));
        }
    } else if (m_isEditingText && m_editingTextObj) {
        std::vector<core::interfaces::dom::TextLineData> pdfLines;
        for (const auto& line : m_textEditor.GetLines()) {
            core::interfaces::dom::TextLineData d;
            d.text = line.text;
            d.x = line.x;
            d.y = line.y;
            d.width = line.width;
            d.height = line.height;
            pdfLines.push_back(d);
        }
        
        auto editCmd = std::make_unique<pdf_engine::commands::EditMultilineTextCommand>(
            m_editingTextObj->GetTextObject(),
            m_editingOriginalLines,
            pdfLines
        );
        
        if (onCommandRequested) {
            onCommandRequested(std::move(editCmd));
        } else {
            m_editingTextObj->GetTextObject()->SetLines(pdfLines);
        }
        
        if (onObjectCommitted) {
            onObjectCommitted(m_editingTextObj, m_editingTextObj->GetBounds(), m_editingTextObj->GetBounds(), false, core::interfaces::dom::LineGeometry());
        }
    }
    
    m_isEditingText = false;
    m_isAddingText = false;
    m_isEditingAnnotationText = false;
    m_editingTextObj = nullptr;
    m_editingAnnotObj = nullptr;
    m_textEditor.SetActive(false);
    if (invalidateView) invalidateView();
}

void InteractionManager::CancelTextEdit() {
    if (!m_isEditingText && !m_isAddingText && !m_isEditingAnnotationText) return;
    m_isEditingText = false;
    m_isAddingText = false;
    m_isEditingAnnotationText = false;
    m_editingTextObj = nullptr;
    m_editingAnnotObj = nullptr;
    m_textEditor.SetActive(false);
    if (invalidateView) invalidateView();
}


void InteractionManager::GetHandleRects(const D2D1_RECT_F& bounds, float scale, D2D1_RECT_F rects[9]) {
    float hw = 4.0f / scale; // handle half-width
    float cx = (bounds.left + bounds.right) / 2.0f;
    float cy = (bounds.top + bounds.bottom) / 2.0f;
    
    // TopLeft, Top, TopRight
    rects[0] = { bounds.left - hw, bounds.top - hw, bounds.left + hw, bounds.top + hw };
    rects[1] = { cx - hw, bounds.top - hw, cx + hw, bounds.top + hw };
    rects[2] = { bounds.right - hw, bounds.top - hw, bounds.right + hw, bounds.top + hw };
    // Right
    rects[3] = { bounds.right - hw, cy - hw, bounds.right + hw, cy + hw };
    // BottomRight, Bottom, BottomLeft
    rects[4] = { bounds.right - hw, bounds.bottom - hw, bounds.right + hw, bounds.bottom + hw };
    rects[5] = { cx - hw, bounds.bottom - hw, cx + hw, bounds.bottom + hw };
    rects[6] = { bounds.left - hw, bounds.bottom - hw, bounds.left + hw, bounds.bottom + hw };
    // Left
    rects[7] = { bounds.left - hw, cy - hw, bounds.left + hw, cy + hw };
    // Rotation (above top)
    rects[8] = { cx - hw, bounds.top - 20.0f / scale - hw, cx + hw, bounds.top - 20.0f / scale + hw };
}

D2D1_RECT_F InteractionManager::GetObjectBoundsInView(const ISelectableObject& obj) {
    Rect b = obj.GetBounds();
    double vl, vt, vr, vb;
    if (pageToView) {
        double pLeft = std::min(b.left, b.right);
        double pRight = std::max(b.left, b.right);
        double pTop = std::max(b.top, b.bottom);
        double pBottom = std::min(b.top, b.bottom);
        
        pageToView(pLeft, pTop, obj.GetPageIndex(), vl, vt);
        pageToView(pRight, pBottom, obj.GetPageIndex(), vr, vb);
        
        float sl = static_cast<float>(std::min(vl, vr));
        float sr = static_cast<float>(std::max(vl, vr));
        float st = static_cast<float>(std::min(vt, vb));
        float sb = static_cast<float>(std::max(vt, vb));
        
        return D2D1::RectF(sl, st, sr, sb);
    }
    return D2D1::RectF(0, 0, 0, 0);
}

HandleType InteractionManager::HitTestHandles(double vx, double vy, float scale) {

    for (auto& obj : m_selection.GetSelected()) {
        D2D1_RECT_F bounds = GetObjectBoundsInView(*obj);
        D2D1_RECT_F rects[9];
        GetHandleRects(bounds, scale, rects);
        
        for (int i = 0; i < 9; ++i) {
            if (vx >= rects[i].left && vx <= rects[i].right && vy >= rects[i].top && vy <= rects[i].bottom) {
                return static_cast<HandleType>(i + 1);
            }
        }
    }
    return HandleType::None;
}

std::shared_ptr<ISelectableObject> InteractionManager::HitTestObjects(double px, double py, int pageIndex) {
    for (auto it = m_objects.rbegin(); it != m_objects.rend(); ++it) {
        auto& obj = *it;
        if (obj->GetPageIndex() != pageIndex) continue;
        if (obj->HitTest(px, py)) {
            return obj;
        }
    }
    return nullptr;
}

HitResult InteractionManager::OnLButtonDown(double x, double y, bool shiftPressed) {
    if (HandleContextualToolbarHit(x, y)) {
        if (invalidateView) invalidateView();
        return HitResult::Object;
    }

    if (m_isEditingText || m_isAddingText || m_isEditingAnnotationText) {
        // If clicking inside the text editor bounds (or panel), pass it to the editor
        D2D1_RECT_F b = m_textEditor.GetBounds();
        if (m_isEditingAnnotationText) {
            // Expand bounds to cover the panel background, so clicking panel doesn't commit
            b.left -= 12.0f;
            b.top -= 12.0f;
            b.right += 12.0f;
            b.bottom += 12.0f;
        }
        if (x >= b.left && x <= b.right && y >= b.top && y <= b.bottom) {
            m_textEditor.OnLButtonDown(x, y, shiftPressed);
            if (invalidateView) invalidateView();
            return HitResult::Object;
        } else {
            // Clicked outside, commit text edit
            CommitTextEdit();
        }
    }

    HandleType handle = HitTestHandles(x, y, 1.0f); // TODO: actual scale needed for accurate handle hit test
    if (handle != HandleType::None) {
        m_drag.active = true;
        m_drag.handle = handle;
        m_drag.startX = x;
        m_drag.startY = y;
        m_drag.hasOriginalLineGeometry = false;
        // Find which object owns the handle (assuming single selection for resize)
        if (!m_selection.GetSelected().empty()) {
            auto obj = m_selection.GetSelected().front();
            m_drag.objectId = obj->GetId();
            m_drag.originalBounds = obj->GetBounds();
            m_drag.originalRotation = obj->GetRotation();
            
            auto annotObj = std::dynamic_pointer_cast<AnnotationSelectableObject>(obj);
            if (annotObj && annotObj->GetAnnotation()->GetType() == core::interfaces::dom::AnnotationType::Line) {
                if (annotObj->GetAnnotation()->GetLineGeometry(m_drag.originalLineGeometry)) {
                    m_drag.hasOriginalLineGeometry = true;
                }
            }
        }
        return HitResult::Handle;
    }

    double px = 0, py = 0;
    int pageIndex = -1;
    if (viewToPage) viewToPage(x, y, px, py, pageIndex);
    
    auto hitObj = HitTestObjects(px, py, pageIndex);
    if (hitObj) {
        if (shiftPressed) {
            m_selection.ToggleSelect(hitObj);
        } else {
            if (!m_selection.IsSelected(hitObj->GetId())) {
                m_selection.Select(hitObj);
            }
            m_drag.active = true;
            m_drag.handle = HandleType::None;
            m_drag.startX = x;
            m_drag.startY = y;
            m_drag.hasOriginalLineGeometry = false;
            m_drag.objectId = hitObj->GetId();
            m_drag.originalBounds = hitObj->GetBounds();
            
            auto annotObj = std::dynamic_pointer_cast<AnnotationSelectableObject>(hitObj);
            if (annotObj && annotObj->GetAnnotation()->GetType() == core::interfaces::dom::AnnotationType::Line) {
                if (annotObj->GetAnnotation()->GetLineGeometry(m_drag.originalLineGeometry)) {
                    m_drag.hasOriginalLineGeometry = true;
                }
            }
        }
        if (invalidateView) invalidateView();
        return HitResult::Object;
    }

    return HitResult::None;
}

void InteractionManager::StartMarquee(double x, double y, bool shiftPressed) {
    if (!shiftPressed) {
        m_selection.Clear();
    }
    m_drag.active = true;
    m_drag.isMarquee = true;
    m_drag.handle = HandleType::None;
    m_drag.startX = x;
    m_drag.startY = y;
    m_drag.marqueeRect = D2D1::RectF((float)x, (float)y, (float)x, (float)y);
    if (invalidateView) invalidateView();
}

bool InteractionManager::OnMouseMove(double x, double y) {
    if (m_isEditingText) {
        // Pass mouse move to editor
        // We'll need coordinates mapped to editor bounds? 
        // For now not strictly needed unless selecting text
    }

    if (m_drag.active) {
        if (m_drag.isMarquee) {
            m_drag.marqueeRect.left = (float)std::min(m_drag.startX, x);
            m_drag.marqueeRect.right = (float)std::max(m_drag.startX, x);
            m_drag.marqueeRect.top = (float)std::min(m_drag.startY, y);
            m_drag.marqueeRect.bottom = (float)std::max(m_drag.startY, y);
        } else {
            UpdateDrag(x, y);
        }
        if (invalidateView) invalidateView();
        return true;
    }
    
    // Hover logic could go here
    auto handle = HitTestHandles(x, y, 1.0f);
    if (handle != m_hoverHandle) {
        m_hoverHandle = handle;
        if (invalidateView) invalidateView();
        return true;
    }
    
    if (handle == HandleType::None) {
        double px = 0, py = 0;
        int pageIndex = -1;
        if (viewToPage) viewToPage(x, y, px, py, pageIndex);
        auto hitObj = HitTestObjects(px, py, pageIndex);
        
        std::string newHoverObj;
        if (hitObj && m_selection.IsSelected(hitObj->GetId())) {
            newHoverObj = hitObj->GetId();
        }
        
        if (newHoverObj != m_hoverObject) {
            m_hoverObject = newHoverObj;
            if (invalidateView) invalidateView();
            return true;
        }
    }
    
    return false;
}

void InteractionManager::UpdateDrag(double x, double y) {
    if (m_drag.objectId.empty()) return;
    auto it = std::find_if(m_selection.GetSelected().begin(), m_selection.GetSelected().end(), 
        [&](auto& o) { return o->GetId() == m_drag.objectId; });
    if (it == m_selection.GetSelected().end()) return;
    auto obj = *it;

    double dx = x - m_drag.startX; (void)dx;
    double dy = y - m_drag.startY; (void)dy;

    double dpx = 0, dpy = 0;
    // We need to convert view delta to page delta
    if (viewToPage) {
        double px1, py1, px2, py2;
        int pageIndex;
        viewToPage(m_drag.startX, m_drag.startY, px1, py1, pageIndex);
        viewToPage(x, y, px2, py2, pageIndex);
        dpx = px2 - px1;
        dpy = py2 - py1;
    }

    auto annotObj = std::dynamic_pointer_cast<AnnotationSelectableObject>(obj);

    Rect newBounds = m_drag.originalBounds;

    if (m_drag.handle == HandleType::None) { // Move
        newBounds.left += (float)dpx;
        newBounds.right += (float)dpx;
        newBounds.top += (float)dpy;
        newBounds.bottom += (float)dpy;
    } else if (m_drag.handle == HandleType::Rotation) {
        double cx = (m_drag.originalBounds.left + m_drag.originalBounds.right) / 2.0;
        double cy = (m_drag.originalBounds.top + m_drag.originalBounds.bottom) / 2.0;
        
        double px1 = 0, py1 = 0, px2 = 0, py2 = 0;
        int dummyPage = -1;
        if (viewToPage) {
            viewToPage(m_drag.startX, m_drag.startY, px1, py1, dummyPage);
            viewToPage(x, y, px2, py2, dummyPage);
        }
        
        double startAngleRad = atan2(py1 - cy, px1 - cx);
        double angleRad = atan2(py2 - cy, px2 - cx);
        
        double startAngleDeg = startAngleRad * 180.0 / 3.14159265358979323846;
        double angleDeg = angleRad * 180.0 / 3.14159265358979323846;
        
        obj->SetRotation(m_drag.originalRotation + (angleDeg - startAngleDeg)); 
        return;
    } else {
        if (m_drag.handle == HandleType::Right || m_drag.handle == HandleType::TopRight || m_drag.handle == HandleType::BottomRight) {
            newBounds.right += (float)dpx;
        }
        if (m_drag.handle == HandleType::Left || m_drag.handle == HandleType::TopLeft || m_drag.handle == HandleType::BottomLeft) {
            newBounds.left += (float)dpx;
        }
        if (m_drag.handle == HandleType::Bottom || m_drag.handle == HandleType::BottomLeft || m_drag.handle == HandleType::BottomRight) {
            newBounds.bottom += (float)dpy;
        }
        if (m_drag.handle == HandleType::Top || m_drag.handle == HandleType::TopLeft || m_drag.handle == HandleType::TopRight) {
            newBounds.top += (float)dpy;
        }
    }
    
    // Snapping logic (Task 9)
    const double SNAP_THRESHOLD = 5.0; // in PDF points
    double snapDx = 0.0;
    double snapDy = 0.0;
    bool snappedX = false;
    bool snappedY = false;
    
    // Only snap if we are moving or resizing, not rotating
    if (m_drag.handle != HandleType::Rotation) {
        for (const auto& otherObj : m_objects) {
            if (otherObj->GetId() == m_drag.objectId) continue;
            if (otherObj->GetPageIndex() != obj->GetPageIndex()) continue;
            
            Rect other = otherObj->GetBounds();
            
            // X snapping
            if (!snappedX) {
                double checkX = (m_drag.handle == HandleType::None || m_drag.handle == HandleType::Left || m_drag.handle == HandleType::TopLeft || m_drag.handle == HandleType::BottomLeft) ? newBounds.left : newBounds.right;
                if (m_drag.handle == HandleType::None || m_drag.handle == HandleType::Right || m_drag.handle == HandleType::TopRight || m_drag.handle == HandleType::BottomRight) {
                    checkX = newBounds.right;
                }
                
                // For move, check both left and right
                if (m_drag.handle == HandleType::None) {
                    if (std::abs(newBounds.left - other.left) < SNAP_THRESHOLD) { snapDx = other.left - newBounds.left; snappedX = true; }
                    else if (std::abs(newBounds.right - other.right) < SNAP_THRESHOLD) { snapDx = other.right - newBounds.right; snappedX = true; }
                    else {
                        double cx1 = (newBounds.left + newBounds.right) / 2.0;
                        double cx2 = (other.left + other.right) / 2.0;
                        if (std::abs(cx1 - cx2) < SNAP_THRESHOLD) { snapDx = cx2 - cx1; snappedX = true; }
                    }
                } else {
                    // Resize snap
                    if (std::abs(checkX - other.left) < SNAP_THRESHOLD) { snapDx = other.left - checkX; snappedX = true; }
                    else if (std::abs(checkX - other.right) < SNAP_THRESHOLD) { snapDx = other.right - checkX; snappedX = true; }
                }
            }
            
            // Y snapping
            if (!snappedY) {
                double checkY = (m_drag.handle == HandleType::None || m_drag.handle == HandleType::Top || m_drag.handle == HandleType::TopLeft || m_drag.handle == HandleType::TopRight) ? newBounds.top : newBounds.bottom;
                
                if (m_drag.handle == HandleType::None) {
                    if (std::abs(newBounds.top - other.top) < SNAP_THRESHOLD) { snapDy = other.top - newBounds.top; snappedY = true; }
                    else if (std::abs(newBounds.bottom - other.bottom) < SNAP_THRESHOLD) { snapDy = other.bottom - newBounds.bottom; snappedY = true; }
                    else {
                        double cy1 = (newBounds.top + newBounds.bottom) / 2.0;
                        double cy2 = (other.top + other.bottom) / 2.0;
                        if (std::abs(cy1 - cy2) < SNAP_THRESHOLD) { snapDy = cy2 - cy1; snappedY = true; }
                    }
                } else {
                    // Resize snap
                    if (std::abs(checkY - other.top) < SNAP_THRESHOLD) { snapDy = other.top - checkY; snappedY = true; }
                    else if (std::abs(checkY - other.bottom) < SNAP_THRESHOLD) { snapDy = other.bottom - checkY; snappedY = true; }
                }
            }
        }
        
        // Apply snap deltas
        if (m_drag.handle == HandleType::None) {
            newBounds.left += snapDx; newBounds.right += snapDx;
            newBounds.top += snapDy; newBounds.bottom += snapDy;
        } else {
            if (m_drag.handle == HandleType::Left || m_drag.handle == HandleType::TopLeft || m_drag.handle == HandleType::BottomLeft) newBounds.left += snapDx;
            if (m_drag.handle == HandleType::Right || m_drag.handle == HandleType::TopRight || m_drag.handle == HandleType::BottomRight) newBounds.right += snapDx;
            if (m_drag.handle == HandleType::Top || m_drag.handle == HandleType::TopLeft || m_drag.handle == HandleType::TopRight) newBounds.top += snapDy;
            if (m_drag.handle == HandleType::Bottom || m_drag.handle == HandleType::BottomLeft || m_drag.handle == HandleType::BottomRight) newBounds.bottom += snapDy;
        }
    }
    
    // Applying to lines
    if (annotObj && annotObj->GetAnnotation()->GetType() == core::interfaces::dom::AnnotationType::Line) {
        if (m_drag.hasOriginalLineGeometry) {
            core::interfaces::dom::LineGeometry geom = m_drag.originalLineGeometry;
            
            if (m_drag.handle == HandleType::None) {
                geom.start.x += (float)dpx;
                geom.start.y += (float)dpy;
                geom.end.x += (float)dpx;
                geom.end.y += (float)dpy;
            } else if (m_drag.handle == HandleType::TopLeft) { // Start point
                geom.start.x += (float)dpx;
                geom.start.y += (float)dpy;
            } else if (m_drag.handle == HandleType::BottomRight) { // End point
                geom.end.x += (float)dpx;
                geom.end.y += (float)dpy;
            }
            annotObj->GetAnnotation()->SetLineGeometry(geom);
        }
    }
    
    obj->SetBounds(newBounds);
}

bool InteractionManager::OnLButtonUp(double x, double y) {
    (void)x;
    (void)y;
    if (m_drag.active) {
        if (m_drag.isMarquee) {
            // Check intersection
            for (auto& obj : m_objects) {
                D2D1_RECT_F b = GetObjectBoundsInView(*obj);
                if (b.right >= m_drag.marqueeRect.left && b.left <= m_drag.marqueeRect.right &&
                    b.bottom >= m_drag.marqueeRect.top && b.top <= m_drag.marqueeRect.bottom) {
                    m_selection.AddSelect(obj);
                }
            }
        } else if (!m_drag.objectId.empty()) {
            if (onObjectCommitted) {
                auto it = std::find_if(m_objects.begin(), m_objects.end(), [&](auto& obj) { return obj->GetId() == m_drag.objectId; });
                if (it != m_objects.end()) {
                    onObjectCommitted(*it, m_drag.originalBounds, (*it)->GetBounds(), m_drag.hasOriginalLineGeometry, m_drag.originalLineGeometry);
                }
            }
        }
        m_drag.active = false;
        m_drag.isMarquee = false;
        m_drag.hasOriginalLineGeometry = false;
        if (invalidateView) invalidateView();
        return true;
    }
    return false;
}

bool InteractionManager::OnKeyDown(WPARAM wParam, bool shiftPressed, bool ctrlPressed) {
    if (m_isEditingText || m_isAddingText || m_isEditingAnnotationText) {
        if (wParam == VK_ESCAPE) {
            CancelTextEdit();
            return true;
        }
        if (wParam == VK_RETURN && !shiftPressed) {
            CommitTextEdit();
            return true;
        }
        bool handled = m_textEditor.OnKeyDown(wParam, shiftPressed, ctrlPressed);
        if (handled && invalidateView) invalidateView();
        return handled;
    }

    if (wParam == VK_ESCAPE) {
        m_selection.Clear();
        if (invalidateView) invalidateView();
        return true;
    }
    if (wParam == VK_DELETE) {
        if (onDeleteRequested) {
            onDeleteRequested(m_selection.GetSelected());
        }
        m_selection.Clear();
        if (invalidateView) invalidateView();
        return true;
    }
    // Arrow keys for movement
    if (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN) {
        if (!m_selection.GetSelected().empty()) {
            double dpx = 0, dpy = 0;
            double amount = shiftPressed ? 10.0 : 1.0;
            if (wParam == VK_LEFT) dpx = -amount;
            if (wParam == VK_RIGHT) dpx = amount;
            if (wParam == VK_UP) dpy = -amount;
            if (wParam == VK_DOWN) dpy = amount;
            
        
    for (auto& obj : m_selection.GetSelected()) {
                Rect b = obj->GetBounds();
                b.left += dpx; b.right += dpx;
                b.top += dpy; b.bottom += dpy;
                obj->SetBounds(b);
                
                auto annotObj = std::dynamic_pointer_cast<AnnotationSelectableObject>(obj);
                if (annotObj && annotObj->GetAnnotation()->GetType() == core::interfaces::dom::AnnotationType::Line) {
                    core::interfaces::dom::LineGeometry geom;
                    if (annotObj->GetAnnotation()->GetLineGeometry(geom)) {
                        geom.start.x += (float)dpx; geom.start.y += (float)dpy;
                        geom.end.x += (float)dpx; geom.end.y += (float)dpy;
                        annotObj->GetAnnotation()->SetLineGeometry(geom);
                    }
                }
            }
            if (invalidateView) invalidateView();
            return true;
        }
    }
    return false;
}

bool InteractionManager::OnChar(WPARAM wParam) {
    if (m_isEditingText || m_isAddingText || m_isEditingAnnotationText) {
        bool handled = m_textEditor.OnChar(wParam);
        if (handled && invalidateView) invalidateView();
        return handled;
    }
    return false;
}

void InteractionManager::Render(ID2D1RenderTarget* renderTarget, float scale) {
    if (!renderTarget) return;

    
    if (!this->pageToView) return;

    ID2D1SolidColorBrush* brush = nullptr;
    ID2D1SolidColorBrush* fillBrush = nullptr;
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.5f, 1.0f, 1.0f), &brush);
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.5f, 1.0f, 0.2f), &fillBrush);

    ID2D1SolidColorBrush* handleBrush = nullptr;
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &handleBrush);
    ID2D1SolidColorBrush* strokeBrush = nullptr;
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.5f, 1.0f, 1.0f), &strokeBrush);

    int debug_render_count = 0;
    for (const auto& obj : m_objects) {
        auto annotObj = std::dynamic_pointer_cast<AnnotationSelectableObject>(obj);
        if (annotObj) {
            auto annot = annotObj->GetAnnotation();
            if (annot) { // FORCE FALLBACK RENDER ALWAYS
                

                D2D1_RECT_F b = GetObjectBoundsInView(*obj);
                
                int r=255, g=0, bl=0, a=255;
                if (!annot->GetColor(r, g, bl, a) || a == 0) {
                    r = 0; g = 120; bl = 215; a = 255; // Default visible color
                }
                float bw = annot->GetBorderWidth();
                if (bw <= 0.0f) bw = 2.0f;

                Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> sBrush;
                renderTarget->CreateSolidColorBrush(D2D1::ColorF(r/255.f, g/255.f, bl/255.f, a/255.f), &sBrush);

                Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> fBrush;
                int fr=0, fg=0, fb=0, fa=0;
                if (annot->GetFillColor(fr, fg, fb, fa) && fa > 0) {
                    renderTarget->CreateSolidColorBrush(D2D1::ColorF(fr/255.f, fg/255.f, fb/255.f, fa/255.f), &fBrush);
                }

                debug_render_count++;
                auto type = annot->GetType();
                if (type == core::interfaces::dom::AnnotationType::Square) {
                    if (fBrush.Get()) renderTarget->FillRectangle(b, fBrush.Get());
                    if (sBrush.Get()) renderTarget->DrawRectangle(b, sBrush.Get(), bw / scale);
                } else if (type == core::interfaces::dom::AnnotationType::Circle) {
                    D2D1_ELLIPSE ellipse = D2D1::Ellipse(
                        D2D1::Point2F((b.left + b.right) * 0.5f, (b.top + b.bottom) * 0.5f),
                        (b.right - b.left) * 0.5f,
                        (b.bottom - b.top) * 0.5f
                    );
                    if (fBrush.Get()) renderTarget->FillEllipse(ellipse, fBrush.Get());
                    if (sBrush.Get()) renderTarget->DrawEllipse(ellipse, sBrush.Get(), bw / scale);
                } else if (type == core::interfaces::dom::AnnotationType::Line) {
                    core::interfaces::dom::LineGeometry geom;
                    if (annot->GetLineGeometry(geom) && pageToView) {
                        double sx, sy, ex, ey;
                        pageToView(geom.start.x, geom.start.y, obj->GetPageIndex(), sx, sy);
                        pageToView(geom.end.x, geom.end.y, obj->GetPageIndex(), ex, ey);
                        if (sBrush.Get()) renderTarget->DrawLine(
                            D2D1::Point2F(static_cast<float>(sx), static_cast<float>(sy)),
                            D2D1::Point2F(static_cast<float>(ex), static_cast<float>(ey)),
                            sBrush.Get(), bw / scale
                        );
                    }
                } else if (type == core::interfaces::dom::AnnotationType::Ink) {
                    if (sBrush.Get() && pageToView) {
                        for (const auto& stroke : annot->GetInkList()) {
                            for (size_t i = 1; i < stroke.size(); ++i) {
                                double sx, sy, ex, ey;
                                pageToView(stroke[i-1].x, stroke[i-1].y, obj->GetPageIndex(), sx, sy);
                                pageToView(stroke[i].x, stroke[i].y, obj->GetPageIndex(), ex, ey);
                                renderTarget->DrawLine(
                                    D2D1::Point2F(static_cast<float>(sx), static_cast<float>(sy)),
                                    D2D1::Point2F(static_cast<float>(ex), static_cast<float>(ey)),
                                    sBrush.Get(), bw / scale
                                );
                            }
                        }
                    }
                } else if (type == core::interfaces::dom::AnnotationType::Highlight) {
                    if (sBrush.Get() && pageToView) {
                        auto quads = annot->GetQuadPoints();
                        for (const auto& q : quads) {
                            double q1x, q1y, q2x, q2y, q3x, q3y, q4x, q4y;
                            pageToView(q.p1.x, q.p1.y, obj->GetPageIndex(), q1x, q1y);
                            pageToView(q.p2.x, q.p2.y, obj->GetPageIndex(), q2x, q2y);
                            pageToView(q.p3.x, q.p3.y, obj->GetPageIndex(), q3x, q3y);
                            pageToView(q.p4.x, q.p4.y, obj->GetPageIndex(), q4x, q4y);
                            float minX = static_cast<float>(std::min({q1x, q2x, q3x, q4x}));
                            float maxX = static_cast<float>(std::max({q1x, q2x, q3x, q4x}));
                            float minY = static_cast<float>(std::min({q1y, q2y, q3y, q4y}));
                            float maxY = static_cast<float>(std::max({q1y, q2y, q3y, q4y}));
                            D2D1_RECT_F qb = D2D1::RectF(minX, minY, maxX, maxY);
                            renderTarget->FillRectangle(qb, sBrush.Get());
                        }
                    }
                } else if (type == core::interfaces::dom::AnnotationType::Text || type == core::interfaces::dom::AnnotationType::FreeText) {
                    if (sBrush.Get()) {
                        renderTarget->DrawRectangle(b, sBrush.Get(), 1.0f);
                    }
                }

                
                
            }
        }
    }

    static int last_render_count = -1;
    if (debug_render_count != last_render_count) {
        FILE* f = nullptr; fopen_s(&f, "C:\\Users\\sayed\\Downloads\\PDF-Elite\\annot_render.txt", "w"); if (f) { fprintf(f, "InteractionManager rendered %d annotations.\n", debug_render_count); fclose(f); }
        last_render_count = debug_render_count;
    }
    for (const auto& sel : m_selection.GetSelected()) {
        auto b = GetObjectBoundsInView(*sel);
        renderTarget->DrawRectangle(b, strokeBrush, 1.0f);
        
        bool canRotate = false;
        // removed IsRotatable() since it's not a member!
        DrawHandles(renderTarget, b, scale, canRotate, true);
    }

    if (m_drag.active && m_drag.isMarquee) {
        renderTarget->FillRectangle(m_drag.marqueeRect, fillBrush);
        renderTarget->DrawRectangle(m_drag.marqueeRect, brush, 1.0f / scale);
    }

    DrawContextualToolbar(renderTarget);

    if (brush) brush->Release();
    if (fillBrush) fillBrush->Release();
    if (handleBrush) handleBrush->Release();
    if (strokeBrush) strokeBrush->Release();
}

void InteractionManager::DrawHandles(ID2D1RenderTarget* renderTarget, const D2D1_RECT_F& rect, float scale, bool rotatable, bool isActive) {
    (void)isActive;
    ID2D1SolidColorBrush* handleBrush = nullptr;
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &handleBrush);
    ID2D1SolidColorBrush* strokeBrush = nullptr;
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.5f, 1.0f, 1.0f), &strokeBrush);

    D2D1_RECT_F rects[9];
    GetHandleRects(rect, scale, rects);

    int count = rotatable ? 9 : 8;
    for (int i = 0; i < count; ++i) {
        renderTarget->FillRectangle(rects[i], handleBrush);
        renderTarget->DrawRectangle(rects[i], strokeBrush, 1.0f / scale);
    }

    if (rotatable) {
        // Draw line connecting rotation handle
        D2D1_POINT_2F p1 = { (rect.left + rect.right) / 2.0f, rect.top };
        D2D1_POINT_2F p2 = { (rect.left + rect.right) / 2.0f, rects[8].bottom };
        renderTarget->DrawLine(p1, p2, strokeBrush, 1.0f / scale);
    }

    if (handleBrush) handleBrush->Release();
    if (strokeBrush) strokeBrush->Release();
}

} // namespace interaction
} // namespace ui

namespace ui {
namespace interaction {
HCURSOR InteractionManager::GetCursor() const {
    if (m_isEditingText || m_isAddingText) {
        return LoadCursor(nullptr, IDC_IBEAM);
    }
    if (m_hoverHandle != HandleType::None || m_drag.active) {
        HandleType ht = m_drag.active ? m_drag.handle : m_hoverHandle;
        switch (ht) {
            case HandleType::TopLeft:
            case HandleType::BottomRight:
                return LoadCursor(nullptr, IDC_SIZENWSE);
            case HandleType::TopRight:
            case HandleType::BottomLeft:
                return LoadCursor(nullptr, IDC_SIZENESW);
            case HandleType::Top:
            case HandleType::Bottom:
                return LoadCursor(nullptr, IDC_SIZENS);
            case HandleType::Left:
            case HandleType::Right:
                return LoadCursor(nullptr, IDC_SIZEWE);
            case HandleType::Rotation:
                return LoadCursor(nullptr, IDC_SIZEALL);
            default: break;
        }
    }
    return nullptr;
}

void InteractionManager::BuildContextualToolbar(core::interfaces::dom::AnnotationType type) {
    m_currentToolbarType = type;
    m_toolbarButtons.clear();
    // Color
    m_toolbarButtons.push_back({ToolbarButton::Action::ColorPick, L"Color", {0,0,0,0}, false, false, static_cast<int>(::controls::IconType::None), 255, 255, 0, 255});
    if (type == core::interfaces::dom::AnnotationType::Highlight) {
        m_toolbarButtons.push_back({ToolbarButton::Action::OpacityUp, L"Opacity +", {0,0,0,0}, false, false, static_cast<int>(::controls::IconType::None), 0, 0, 0, 0});
        m_toolbarButtons.push_back({ToolbarButton::Action::OpacityDown, L"Opacity -", {0,0,0,0}, false, false, static_cast<int>(::controls::IconType::None), 0, 0, 0, 0});
    }
    if (type == core::interfaces::dom::AnnotationType::Ink || type == core::interfaces::dom::AnnotationType::Square || type == core::interfaces::dom::AnnotationType::Circle || type == core::interfaces::dom::AnnotationType::Line) {
        m_toolbarButtons.push_back({ToolbarButton::Action::WidthUp, L"Width +", {0,0,0,0}, false, false, static_cast<int>(::controls::IconType::None), 0, 0, 0, 0});
        m_toolbarButtons.push_back({ToolbarButton::Action::WidthDown, L"Width -", {0,0,0,0}, false, false, static_cast<int>(::controls::IconType::None), 0, 0, 0, 0});
    }
    // Delete
    m_toolbarButtons.push_back({ToolbarButton::Action::Delete, L"Delete", {0,0,0,0}, false, false, static_cast<int>(::controls::IconType::Delete), 0, 0, 0, 0});
    
    m_colorButtons.clear();
    m_colorButtons.push_back({ToolbarButton::Action::ColorPick, L"Yellow", {0,0,0,0}, false, false, static_cast<int>(::controls::IconType::None), 255, 255, 0, 255});
    m_colorButtons.push_back({ToolbarButton::Action::ColorPick, L"Green", {0,0,0,0}, false, false, static_cast<int>(::controls::IconType::None), 0, 255, 0, 255});
    m_colorButtons.push_back({ToolbarButton::Action::ColorPick, L"Blue", {0,0,0,0}, false, false, static_cast<int>(::controls::IconType::None), 0, 180, 255, 255});
    m_colorButtons.push_back({ToolbarButton::Action::ColorPick, L"Pink", {0,0,0,0}, false, false, static_cast<int>(::controls::IconType::None), 255, 105, 180, 255});
    m_colorButtons.push_back({ToolbarButton::Action::ColorPick, L"Red", {0,0,0,0}, false, false, static_cast<int>(::controls::IconType::None), 255, 0, 0, 255});
}

void InteractionManager::UpdateContextualToolbarLayout(const D2D1_RECT_F& objBounds, float scale) {
    (void)scale;
    if (m_toolbarButtons.empty() || (m_selection.GetSelected().size() > 0 && std::dynamic_pointer_cast<AnnotationSelectableObject>(m_selection.GetSelected().front()) && std::dynamic_pointer_cast<AnnotationSelectableObject>(m_selection.GetSelected().front())->GetAnnotation()->GetType() != m_currentToolbarType)) {
        if (!m_selection.GetSelected().empty()) {
            if (auto annotObj = std::dynamic_pointer_cast<AnnotationSelectableObject>(m_selection.GetSelected().front())) {
                BuildContextualToolbar(annotObj->GetAnnotation()->GetType());
            }
        }
    }
    
    float btnSize = 32.0f;
    float padding = 4.0f;
    float totalW = (btnSize + padding) * m_toolbarButtons.size() + padding;
    
    // Center above the object
    float centerX = (objBounds.left + objBounds.right) / 2.0f;
    float left = centerX - totalW / 2.0f;
    float top = objBounds.top - btnSize - 16.0f;
    
    if (top < 0) top = objBounds.bottom + 16.0f; // flip to bottom if too high
    
    m_toolbarBounds = { left, top, left + totalW, top + btnSize + padding * 2 };
    
    float currentX = left + padding;
    float currentY = top + padding;
    for (auto& btn : m_toolbarButtons) {
        btn.rect = { currentX, currentY, currentX + btnSize, currentY + btnSize };
        currentX += btnSize + padding;
    }
    
    if (m_colorPaletteOpen) {
        float palLeft = left;
        float palTop = m_toolbarBounds.bottom + 4.0f;
        float palX = palLeft + padding;
        for (auto& btn : m_colorButtons) {
            btn.rect = { palX, palTop, palX + btnSize, palTop + btnSize };
            palX += btnSize + padding;
        }
    }
}

void InteractionManager::HideContextualToolbar() {
    m_colorPaletteOpen = false;
    m_strokePaletteOpen = false;
}

bool InteractionManager::HandleContextualToolbarHit(double x, double y) {
    if (m_selection.GetSelected().empty()) return false;
    
    if (m_colorPaletteOpen) {
        ToolbarButton* btn = nullptr;
        for (auto& b : m_colorButtons) {
            if (x >= b.rect.left && x <= b.rect.right && y >= b.rect.top && y <= b.rect.bottom) { btn = &b; break; }
        }
        if (btn) {
            auto obj = m_selection.GetSelected().front();
            if (auto annotObj = std::dynamic_pointer_cast<AnnotationSelectableObject>(obj)) {
                if (onColorChangedRequested) {
                    onColorChangedRequested(annotObj, btn->r, btn->g, btn->b, btn->a);
                }
            }
            m_colorPaletteOpen = false;
            return true;
        }
    }
    
    ToolbarButton* mainBtn = nullptr;
    for (auto& b : m_toolbarButtons) {
        if (x >= b.rect.left && x <= b.rect.right && y >= b.rect.top && y <= b.rect.bottom) { mainBtn = &b; break; }
    }
    if (mainBtn) {
        auto* btn = mainBtn;
        if (btn->action == ToolbarButton::Action::Delete) {
            if (onDeleteRequested) {
                onDeleteRequested(m_selection.GetSelected());
            }
            m_selection.Clear();
            return true;
        } else if (btn->action == ToolbarButton::Action::WidthUp || btn->action == ToolbarButton::Action::WidthDown || btn->action == ToolbarButton::Action::OpacityUp || btn->action == ToolbarButton::Action::OpacityDown) {
            if (auto annotObj = std::dynamic_pointer_cast<AnnotationSelectableObject>(m_selection.GetSelected().front())) {
                if (btn->action == ToolbarButton::Action::WidthUp && onWidthChangedRequested) {
                    onWidthChangedRequested(annotObj, annotObj->GetAnnotation()->GetBorderWidth() + 1.0f);
                } else if (btn->action == ToolbarButton::Action::WidthDown && onWidthChangedRequested) {
                    onWidthChangedRequested(annotObj, std::max(1.0f, annotObj->GetAnnotation()->GetBorderWidth() - 1.0f));
                } else if (btn->action == ToolbarButton::Action::OpacityUp && onOpacityChangedRequested) {
                    onOpacityChangedRequested(annotObj, std::min(1.0f, annotObj->GetAnnotation()->GetOpacity() + 0.1f));
                } else if (btn->action == ToolbarButton::Action::OpacityDown && onOpacityChangedRequested) {
                    onOpacityChangedRequested(annotObj, std::max(0.1f, annotObj->GetAnnotation()->GetOpacity() - 0.1f));
                }
            }
            return true;
        } else if (btn->action == ToolbarButton::Action::ColorPick) {
            m_colorPaletteOpen = !m_colorPaletteOpen;
            return true;
        }
    }
    
    if (m_colorPaletteOpen) {
        m_colorPaletteOpen = false;
        if (x < m_toolbarBounds.left || x > m_toolbarBounds.right || y < m_toolbarBounds.top || y > m_toolbarBounds.bottom) {
            return false;
        }
        return true;
    }
    
    if (x >= m_toolbarBounds.left && x <= m_toolbarBounds.right && y >= m_toolbarBounds.top && y <= m_toolbarBounds.bottom) {
        return true;
    }
    return false;
}

void InteractionManager::DrawContextualToolbar(ID2D1RenderTarget* renderTarget) {
    if (m_selection.GetSelected().empty()) return;
    auto obj = m_selection.GetSelected().front();
    
    if (!std::dynamic_pointer_cast<AnnotationSelectableObject>(obj)) return;
    
    D2D1_RECT_F objBounds = GetObjectBoundsInView(*obj);
    UpdateContextualToolbarLayout(objBounds, 1.0f);
    
    ID2D1SolidColorBrush* bgBrush = nullptr;
    ID2D1SolidColorBrush* fgBrush = nullptr;
    
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.15f, 0.15f, 0.15f, 0.95f), &bgBrush);
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.9f, 0.9f, 0.9f, 1.0f), &fgBrush);
    
    if (bgBrush) {
        D2D1_ROUNDED_RECT rrect = D2D1::RoundedRect(m_toolbarBounds, 4.0f, 4.0f);
        renderTarget->FillRoundedRectangle(&rrect, bgBrush);
    }
    
    auto drawBtns = [&](const std::vector<ToolbarButton>& btns) {
        for (const auto& btn : btns) {
            if (btn.action == ToolbarButton::Action::ColorPick) {
                ID2D1SolidColorBrush* cBrush = nullptr;
                renderTarget->CreateSolidColorBrush(D2D1::ColorF(btn.r/255.f, btn.g/255.f, btn.b/255.f, 1.0f), &cBrush);
                if (cBrush) {
                    D2D1_ROUNDED_RECT brrect = D2D1::RoundedRect(btn.rect, 4.0f, 4.0f);
                    renderTarget->FillRoundedRectangle(&brrect, cBrush);
                    cBrush->Release();
                }
            } else if (btn.iconId != static_cast<int>(::controls::IconType::None)) {
                ::controls::IconRenderer::DrawIcon(Microsoft::WRL::ComPtr<ID2D1RenderTarget>(renderTarget), static_cast<::controls::IconType>(btn.iconId), btn.rect, D2D1::ColorF(0.9f, 0.9f, 0.9f, 1.0f));
            }
        }
    };
    
    drawBtns(m_toolbarButtons);
    
    if (m_colorPaletteOpen && !m_colorButtons.empty()) {
        float palTotalW = (32.0f + 4.0f) * m_colorButtons.size() + 4.0f;
        D2D1_RECT_F palBounds = {
            m_toolbarBounds.left,
            m_toolbarBounds.bottom + 4.0f,
            m_toolbarBounds.left + palTotalW,
            m_toolbarBounds.bottom + 4.0f + 32.0f + 8.0f
        };
        if (bgBrush) {
            D2D1_ROUNDED_RECT rrect = D2D1::RoundedRect(palBounds, 4.0f, 4.0f);
            renderTarget->FillRoundedRectangle(&rrect, bgBrush);
        }
        drawBtns(m_colorButtons);
    }
    
    if (bgBrush) bgBrush->Release();
    if (fgBrush) fgBrush->Release();
}






} // namespace interaction
} // namespace ui





























