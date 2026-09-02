#pragma once
#include "SelectionModel.h"
#include "TextSelectableObject.h"
#include "../controls/TextEditor.h"
#include "core/interfaces/dom/ICommand.h"
#include "core/interfaces/dom/IAnnotation.h"
#include <windows.h>
#include <d2d1.h>
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <algorithm>

namespace ui {
namespace controls { class TextEditor; }
namespace interaction {

class ISelectableObject;
class TextSelectableObject;
class AnnotationSelectableObject;

enum class HandleType {
    None, TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left, Rotation
};

struct DragState {
    bool active = false;
    HandleType handle = HandleType::None;
    std::string objectId;
    double startX, startY;
    Rect originalBounds;
    double originalRotation;
    Matrix3x2F originalTransform;
    
    // For Line annotation drag/resize
    bool hasOriginalLineGeometry = false;
    core::interfaces::dom::LineGeometry originalLineGeometry;
    
    bool isMarquee = false;
    D2D1_RECT_F marqueeRect;
};

enum class HitResult {
    None,
    Handle,
    Object
};

class InteractionManager {
public:
    bool HandleContextualToolbarHit(double x, double y);

    InteractionManager();
    ~InteractionManager();

    // Core state
    void SetObjects(const std::vector<std::shared_ptr<ISelectableObject>>& objects);
    void AddObject(std::shared_ptr<ISelectableObject> obj);
    void AddObjects(const std::vector<std::shared_ptr<ISelectableObject>>& objects);
    void RemoveObject(const std::string& id);
    void RemoveObjectsForPage(int pageIndex);
    void ClearObjects();
    std::shared_ptr<ISelectableObject> HitTestObjects(double px, double py, int pageIndex);
    SelectionModel& GetSelectionModel() { return m_selection; }
    std::vector<std::shared_ptr<ISelectableObject>> GetSelection() const;
    const std::vector<std::shared_ptr<ISelectableObject>>& GetObjects() const { return m_objects; }
    
    // Input handling
    HitResult OnLButtonDown(double x, double y, bool shiftPressed);
    void StartMarquee(double x, double y, bool shiftPressed);
    bool OnMouseMove(double x, double y);
    bool OnLButtonUp(double x, double y);
    bool OnKeyDown(WPARAM wParam, bool shiftPressed, bool ctrlPressed);
    bool OnChar(WPARAM wParam);

    void Render(ID2D1RenderTarget* renderTarget, float scale);
    
    // Text Editing
    void EnterTextEditMode(std::shared_ptr<TextSelectableObject> obj);
    void EnterNewTextMode(double vx, double vy);
    void EnterAnnotationEditMode(std::shared_ptr<AnnotationSelectableObject> obj);
    void CommitTextEdit();
    void CancelTextEdit();
    bool IsEditingText() const { return m_isEditingText || m_isEditingAnnotationText; }
    HCURSOR GetCursor() const;
    bool IsHoveringObject() const { return !m_hoverObject.empty() && m_hoverHandle == HandleType::None && !m_drag.active; }

    // Callbacks to convert view to page and back
    std::function<void(double vx, double vy, double& px, double& py, int& pageIndex)> viewToPage;
    std::function<void(double px, double py, int pageIndex, double& vx, double& vy)> pageToView;
    std::function<void()> invalidateView;
    std::function<void(std::shared_ptr<ISelectableObject> obj, const Rect& oldBounds, const Rect& newBounds, bool hasOldLineGeom, const core::interfaces::dom::LineGeometry& oldLineGeom)> onObjectCommitted;
    std::function<void(const std::vector<std::shared_ptr<ISelectableObject>>&)> onDeleteRequested;
    std::function<void(std::shared_ptr<AnnotationSelectableObject>, int, int, int, int)> onColorChangedRequested;
    std::function<void(std::shared_ptr<AnnotationSelectableObject>, float)> onOpacityChangedRequested;
    std::function<void(std::shared_ptr<AnnotationSelectableObject>, float)> onWidthChangedRequested;
    std::function<void(std::unique_ptr<core::interfaces::dom::ICommand>)> onCommandRequested;
    std::function<void()> onSelectionChanged;

private:
    SelectionModel m_selection;
    std::vector<std::shared_ptr<ISelectableObject>> m_objects;
    DragState m_drag;
    HandleType m_hoverHandle = HandleType::None;
    std::string m_hoverObject;
    
    // Text Editing
    ui::controls::TextEditor m_textEditor;
    std::shared_ptr<TextSelectableObject> m_editingTextObj;
    std::shared_ptr<AnnotationSelectableObject> m_editingAnnotObj;
    bool m_isEditingText = false;
    bool m_isAddingText = false;
    bool m_isEditingAnnotationText = false;
    int m_addingTextPageIndex = -1;
    double m_addingTextPx = 0;
    double m_addingTextPy = 0;
    std::vector<core::interfaces::dom::TextLineData> m_editingOriginalLines;

    // Contextual Toolbar
    struct ToolbarButton {
        enum class Action { ColorPick, StrokeWidth, Delete, OpacityUp, OpacityDown, WidthUp, WidthDown };
        Action action;
        std::wstring tooltip;
        D2D1_RECT_F rect;
        bool isHovered = false;
        bool isPressed = false;
        int iconId = 0;
        int r = 0, g = 0, b = 0, a = 255;
    };
    std::vector<ToolbarButton> m_toolbarButtons;
    core::interfaces::dom::AnnotationType m_currentToolbarType = core::interfaces::dom::AnnotationType::Unknown;
    std::vector<ToolbarButton> m_colorButtons;
    bool m_colorPaletteOpen = false;
    bool m_strokePaletteOpen = false;
    D2D1_RECT_F m_toolbarBounds = {0,0,0,0};
    
    void BuildContextualToolbar(core::interfaces::dom::AnnotationType type);
    void UpdateContextualToolbarLayout(const D2D1_RECT_F& objBounds, float scale);
    void DrawContextualToolbar(ID2D1RenderTarget* renderTarget);

    void HideContextualToolbar();

    // Helper functions
    HandleType HitTestHandles(double vx, double vy, float scale);
    D2D1_RECT_F GetObjectBoundsInView(const ISelectableObject& obj);
    void DrawHandles(ID2D1RenderTarget* renderTarget, const D2D1_RECT_F& rect, float scale, bool rotatable, bool isActive);
    void UpdateDrag(double x, double y);
    
    // Geometry helpers
    void GetHandleRects(const D2D1_RECT_F& bounds, float scale, D2D1_RECT_F rects[9]);
};

} // namespace interaction
} // namespace ui






