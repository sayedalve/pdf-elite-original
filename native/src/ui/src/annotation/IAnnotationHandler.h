#pragma once
#include <windows.h>
#include <d2d1.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "core/Geometry.h"
#include "core/interfaces/dom/IAnnotation.h"
#include "core/interfaces/dom/ICommand.h"
#include "core/interfaces/dom/IDocument.h"
#include "core/interfaces/dom/ITextPage.h"
#include "PdfViewer.h"

namespace ui::annotation {

enum class InteractionState {
    Idle,
    Creating,
    Selected,
    Moving,
    Resizing
};

struct MouseEvent {
    float viewX = 0.0f;
    float viewY = 0.0f;
    double pdfX = 0.0;
    double pdfY = 0.0;
    int pageIndex = -1;
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
    bool leftButton = false;
    bool rightButton = false;
};

struct AnnotationHandlerContext {
    HWND hwnd = nullptr;
    std::function<void(double vx, double vy, double& px, double& py, int& pageIndex)> viewToPage;
    std::function<void(double px, double py, int pageIndex, double& vx, double& vy)> pageToView;
    std::function<void()> invalidateView;
    std::function<bool(std::unique_ptr<core::interfaces::dom::ICommand>)> executeCommand;
    std::function<core::interfaces::dom::IDocument*()> getDocument;
    std::function<core::interfaces::dom::ITextPage*(int pageIndex)> getTextPage;
    std::function<void(ToolMode mode)> setToolMode;
    std::function<void()> reloadInteractables;
    std::function<void()> onMutationCommitted;
};

class IAnnotationHandler {
public:
    virtual ~IAnnotationHandler() = default;

    virtual void Initialize(const AnnotationHandlerContext& context) = 0;
    virtual bool OnMouseDown(const MouseEvent& event) = 0;
    virtual bool OnMouseMove(const MouseEvent& event) = 0;
    virtual bool OnMouseUp(const MouseEvent& event) = 0;
    virtual bool OnKeyDown(int keyCode, bool shift, bool ctrl, bool alt) = 0;
    virtual bool OnKeyUp(int keyCode, bool shift, bool ctrl, bool alt) = 0;
    virtual HCURSOR OnSetCursor(const PointF& viewPoint) = 0;
    virtual void RenderPreview(ID2D1RenderTarget* target, float zoom, const PointF& scrollOffset) = 0;
    virtual void Cancel() = 0;
    virtual InteractionState GetState() const = 0;
    virtual ToolMode GetToolType() const = 0;
};

} // namespace ui::annotation
