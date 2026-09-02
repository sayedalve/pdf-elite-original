#pragma once

#include <string>
#include <memory>
#include <functional>
#include <windows.h>
#include <d2d1.h>
#include "ToolType.h"
#include "../input/PointerEvent.h"
#include "../input/EventResult.h"
#include "../input/PointerCaptureService.h"
#include "../selection/SelectionModel.h"
#include "../../../core/Geometry.h"

#include "../../../core/interfaces/dom/ICommand.h"

namespace core::interfaces::dom {
class IDocument;
class ITextPage;
class IAnnotation;
}

namespace ui::tools {

enum class ToolState {
    Idle,           // Tool is inactive or hovering without button press
    Hovering,       // Hovering over an interactive element or canvas
    HoveringHandle, // Over an object resize/rotation handle
    Dragging,       // Active dragging (pan, marquee, shape creation, move, resize)
    InPlaceEditing, // In-place text editing
    Committed,      // Gesture committed, command dispatched
    Suspended       // Temporarily suspended
};

struct ToolContext {
    core::interfaces::dom::IDocument* document = nullptr;
    input::IPointerCaptureService* captureService = nullptr;
    HWND hwnd = nullptr;

    std::function<void()> invalidateView;
    std::function<void(ToolType newTool)> requestToolSwitch;
    std::function<void(const std::wstring& statusMsg)> updateStatusText;
    std::function<void(float deltaX, float deltaY)> scrollViewport;
    std::function<void(double factor, double mouseX, double mouseY)> zoomViewport;
    std::function<double()> getZoom;
    std::function<PointF(const PointF& dipPt)> dipToCanvas;
    std::function<PointF(const PointF& canvasPt)> canvasToDip;
    std::function<PointF(const PointF& canvasPt, int& outPageIndex)> canvasToPdf;
    std::function<PointF(int pageIndex, const PointF& pdfPt)> pdfToCanvas;
    std::function<core::interfaces::dom::ITextPage*(int pageIndex)> getTextPage;
    std::function<std::vector<ui::selection::SelectedObject>(const PointF& canvasPt)> hitTestObjects;
    std::function<bool(std::unique_ptr<core::interfaces::dom::ICommand> cmd)> executeCommand;
    std::function<void(const std::string& annotId)> openAnnotationPopup;
    std::function<void(std::shared_ptr<core::interfaces::dom::IAnnotation> annot)> enterAnnotationEditMode;
};

class ITool {
public:
    virtual ~ITool() = default;

    virtual ToolType GetType() const = 0;
    virtual std::wstring GetName() const = 0;
    virtual ToolState GetState() const = 0;

    // Lifecycle
    virtual void OnActivate(ToolContext& context) = 0;
    virtual void OnDeactivate(ToolContext& context) = 0;
    virtual void Cancel(ToolContext& context) = 0;

    // Input handlers
    virtual input::EventResult OnPointerDown(const input::PointerEvent& event, ToolContext& context) = 0;
    virtual input::EventResult OnPointerMove(const input::PointerEvent& event, ToolContext& context) = 0;
    virtual input::EventResult OnPointerUp(const input::PointerEvent& event, ToolContext& context) = 0;
    virtual input::EventResult OnPointerDoubleClick(const input::PointerEvent& event, ToolContext& context) {
        (void)event; (void)context; return input::EventResult::Ignored;
    }

    virtual input::EventResult OnKeyDown(const input::KeyEvent& event, ToolContext& context) {
        (void)event; (void)context; return input::EventResult::Ignored;
    }
    virtual input::EventResult OnKeyUp(const input::KeyEvent& event, ToolContext& context) {
        (void)event; (void)context; return input::EventResult::Ignored;
    }
    virtual input::EventResult OnChar(const input::KeyEvent& event, ToolContext& context) {
        (void)event; (void)context; return input::EventResult::Ignored;
    }
    virtual input::EventResult OnMouseWheel(const input::ScrollEvent& event, ToolContext& context) {
        (void)event; (void)context; return input::EventResult::Ignored;
    }

    // Visuals & Cursors
    virtual HCURSOR GetCursor(const PointF& point, ToolContext& context) const = 0;
    virtual void RenderOverlay(ID2D1RenderTarget* renderTarget, ToolContext& context) {
        (void)renderTarget; (void)context;
    }
};

} // namespace ui::tools
