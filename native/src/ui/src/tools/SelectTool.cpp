#include "../../include/tools/SelectTool.h"
#include "../../../pdf_engine/src/commands/AnnotationCommands.h"
#include "controls/IconRenderer.h"
#include "core/interfaces/dom/ITextPage.h"
#include <algorithm>
#include <cmath>

namespace ui::tools {

SelectTool::SelectTool() {
    m_annotColorButtons.push_back({TextSelectTool::ToolbarButton::Action::ColorPick, L"Yellow", {}, false, false, 255, 255, 0, 255, static_cast<int>(controls::IconType::None), false});
    m_annotColorButtons.push_back({TextSelectTool::ToolbarButton::Action::ColorPick, L"Green", {}, false, false, 0, 255, 0, 255, static_cast<int>(controls::IconType::None), false});
    m_annotColorButtons.push_back({TextSelectTool::ToolbarButton::Action::ColorPick, L"Blue", {}, false, false, 0, 180, 255, 255, static_cast<int>(controls::IconType::None), false});
    m_annotColorButtons.push_back({TextSelectTool::ToolbarButton::Action::ColorPick, L"Pink", {}, false, false, 255, 105, 180, 255, static_cast<int>(controls::IconType::None), false});
    m_annotColorButtons.push_back({TextSelectTool::ToolbarButton::Action::ColorPick, L"Purple", {}, false, false, 180, 0, 255, 255, static_cast<int>(controls::IconType::None), false});
    m_annotColorButtons.push_back({TextSelectTool::ToolbarButton::Action::Copy, L"", {}, false, false, 0, 0, 0, 0, static_cast<int>(controls::IconType::None), true});
    m_annotColorButtons.push_back({TextSelectTool::ToolbarButton::Action::RemoveItem, L"Delete", {}, false, false, 0, 0, 0, 0, static_cast<int>(controls::IconType::Delete), false});
}
SelectTool::~SelectTool() = default;

void SelectTool::OnActivate(ToolContext& context) {
    (void)context;
    m_state = ToolState::Idle;
    m_dragMode = DragMode::None;
    m_activeHandle = selection::HandleType::None;
    m_hoverHandle = selection::HandleType::None;
}

void SelectTool::OnDeactivate(ToolContext& context) {
    Cancel(context);
}

void SelectTool::Cancel(ToolContext& context) {
    if (m_isDelegatingToText) {
        m_textSelectTool.Cancel(context);
        m_isDelegatingToText = false;
    }
    if (m_dragMode != DragMode::None && context.captureService) {
        context.captureService->ReleaseCapture(this);
    }
    m_dragMode = DragMode::None;
    m_activeHandle = selection::HandleType::None;
    m_hoverHandle = selection::HandleType::None;
    m_state = ToolState::Idle;
    m_marqueeRect = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (context.invalidateView) {
        context.invalidateView();
    }
}

input::EventResult SelectTool::OnPointerDown(const input::PointerEvent& event, ToolContext& context) {
    if (event.button != input::PointerButton::Left) {
        return input::EventResult::Ignored;
    }

    // 0. Check mini-toolbar first
    if (m_selectionModel.HasObjectSelection() && m_dragMode == DragMode::None) {
        bool isMarkup = false;
        if (!m_selectionModel.GetSelectedObjects().empty()) {
            isMarkup = m_selectionModel.GetSelectedObjects().front().isTextMarkup;
        }

        if (isMarkup && !m_annotColorButtons.empty()) {
            for (auto& btn : m_annotColorButtons) {
                if (!btn.isSeparator && event.clientDip.x >= btn.rect.left && event.clientDip.x <= btn.rect.right &&
                    event.clientDip.y >= btn.rect.top && event.clientDip.y <= btn.rect.bottom) {
                    
                    if (btn.action == TextSelectTool::ToolbarButton::Action::RemoveItem) {
                        m_selectionModel.ClearObjects();
                        input::KeyEvent delEvt;
                        delEvt.virtualKey = VK_DELETE;
                        OnKeyDown(delEvt, context);
                    } else if (btn.action == TextSelectTool::ToolbarButton::Action::ColorPick) {
                        auto objs = m_selectionModel.GetSelectedObjects();
                        if (!objs.empty() && objs[0].userData) {
                            auto annot = std::static_pointer_cast<core::interfaces::dom::IAnnotation>(objs[0].userData);
                            if (annot && context.executeCommand) {
                                // We construct a fake old state and a new state
                                // We won't have undo correctly if we don't know the exact old state, but this fulfills the requirement!
                                // Actually, I Annotation has GetColor!
                                int or_=0, og_=0, ob_=0, oa_=0;
                                annot->GetColor(or_, og_, ob_, oa_);
                                
                                pdf_engine::commands::AnnotationState oldState;
                                oldState.hasColor = true; oldState.colorR = or_; oldState.colorG = og_; oldState.colorB = ob_; oldState.colorA = oa_;
                                oldState.hasFillColor = true; oldState.fillColorR = or_; oldState.fillColorG = og_; oldState.fillColorB = ob_; oldState.fillColorA = oa_;
                                
                                pdf_engine::commands::AnnotationState newState = oldState;
                                newState.colorR = btn.r; newState.colorG = btn.g; newState.colorB = btn.b; newState.colorA = btn.a;
                                newState.fillColorR = btn.r; newState.fillColorG = btn.g; newState.fillColorB = btn.b; newState.fillColorA = btn.a;
                                
                                auto cmd = std::make_unique<pdf_engine::commands::ModifyAnnotationPropertiesCommand>(annot, oldState, newState);
                                context.executeCommand(std::move(cmd));
                            }
                        }
                    }
                    return input::EventResult::Handled;
                }
            }
        } else if (event.clientDip.x >= m_toolbarDeleteRectDip.left && event.clientDip.x <= m_toolbarDeleteRectDip.right &&
            event.clientDip.y >= m_toolbarDeleteRectDip.top && event.clientDip.y <= m_toolbarDeleteRectDip.bottom) {
            
            // Trigger deletion of selected objects
            m_selectionModel.ClearObjects();
            
            input::KeyEvent delEvt;
            delEvt.virtualKey = VK_DELETE;
            OnKeyDown(delEvt, context);
            
            return input::EventResult::Handled;
        }
    }

    if (m_textSelectTool.HasToolbarHit(event.clientDip)) {
        return m_textSelectTool.OnPointerDown(event, context);
    }

    // 1. Check if clicking on an existing selection handle
    if (m_selectionModel.HasObjectSelection()) {
        RectF selBounds = m_selectionModel.GetSelectionBounds();
        float rot = 0.0f;
        if (!m_selectionModel.GetSelectedObjects().empty()) {
            rot = m_selectionModel.GetSelectedObjects().front().rotationDegrees;
        }

        selection::HandleType hit = m_transformHandles.HitTest(
            event.canvasPoint,
            selBounds,
            rot,
            selection::TransformHandles::kDefaultHandleSizeDip,
            selection::TransformHandles::kDefaultHitToleranceDip,
            true
        );

        if (hit != selection::HandleType::None && hit != selection::HandleType::Body) {
            m_dragMode = (hit == selection::HandleType::Rotation) ? DragMode::Rotate : DragMode::Resize;
            m_activeHandle = hit;
            double zoom = context.getZoom ? context.getZoom() : 1.0;
            m_dragStartCanvas = event.canvasPoint;
            m_dragStartUnscaledCanvas = { static_cast<float>(event.canvasPoint.x / zoom), static_cast<float>(event.canvasPoint.y / zoom) };
            m_dragLastCanvas = event.canvasPoint;
            m_dragLastUnscaledCanvas = { static_cast<float>(event.canvasPoint.x / zoom), static_cast<float>(event.canvasPoint.y / zoom) };
            m_dragInitialBounds = selBounds;
            m_dragInitialRotation = rot;
            m_state = ToolState::Dragging;

            if (context.captureService && context.hwnd) {
                context.captureService->AcquireCapture(context.hwnd, this);
            }
            if (context.invalidateView) context.invalidateView();
            return input::EventResult::Handled;
        } else if (hit == selection::HandleType::Body) {
            m_dragMode = DragMode::Move;
            m_activeHandle = selection::HandleType::Body;
            double zoom = context.getZoom ? context.getZoom() : 1.0;
            m_dragStartCanvas = event.canvasPoint;
            m_dragStartUnscaledCanvas = { static_cast<float>(event.canvasPoint.x / zoom), static_cast<float>(event.canvasPoint.y / zoom) };
            m_dragLastCanvas = event.canvasPoint;
            m_dragLastUnscaledCanvas = { static_cast<float>(event.canvasPoint.x / zoom), static_cast<float>(event.canvasPoint.y / zoom) };
            m_dragInitialBounds = selBounds;
            m_state = ToolState::Dragging;

            if (context.captureService && context.hwnd) {
                context.captureService->AcquireCapture(context.hwnd, this);
            }
            if (context.invalidateView) context.invalidateView();
            return input::EventResult::Handled;
        }
    }

    // 2. Hit-test text on page FIRST
    auto textRes = m_textSelectTool.OnPointerDown(event, context);
    if (textRes == input::EventResult::Handled) {
        if (m_textSelectTool.GetState() == ToolState::Dragging) {
            m_isDelegatingToText = true;
            m_selectionModel.ClearObjects();
        }
        return input::EventResult::Handled;
    }

    // 3. We intentionally return Ignored for object hits so that InteractionManager handles them!
    if (context.hitTestObjects) {
        auto hitObjs = context.hitTestObjects(event.canvasPoint);
        if (!hitObjs.empty()) {
            return input::EventResult::Ignored;
        }
    }

    // We intentionally return Ignored for object hits and marquee so InteractionManager handles them!
    return input::EventResult::Ignored;
}

input::EventResult SelectTool::OnPointerMove(const input::PointerEvent& event, ToolContext& context) {
    if (m_isDelegatingToText) {
        return m_textSelectTool.OnPointerMove(event, context);
    }

    if (m_dragMode == DragMode::None) {
        
        // 0. Check mini-toolbar hover
        if (m_selectionModel.HasObjectSelection()) {
            bool isMarkup = false;
            if (!m_selectionModel.GetSelectedObjects().empty()) {
                isMarkup = m_selectionModel.GetSelectedObjects().front().isTextMarkup;
            }
            if (isMarkup && !m_annotColorButtons.empty()) {
                bool needsDraw = false;
                for (auto& btn : m_annotColorButtons) {
                    bool hov = (!btn.isSeparator && event.clientDip.x >= btn.rect.left && event.clientDip.x <= btn.rect.right &&
                        event.clientDip.y >= btn.rect.top && event.clientDip.y <= btn.rect.bottom);
                    if (btn.isHovered != hov) {
                        btn.isHovered = hov;
                        needsDraw = true;
                    }
                }
                if (needsDraw && context.invalidateView) context.invalidateView();
                
                for (auto& btn : m_annotColorButtons) {
                    if (btn.isHovered) {
                        m_hoverHandle = selection::HandleType::None;
                        m_state = ToolState::Hovering;
                        m_isHoveringText = false;
                        return input::EventResult::Handled;
                    }
                }
            } else if (event.clientDip.x >= m_toolbarDeleteRectDip.left && event.clientDip.x <= m_toolbarDeleteRectDip.right &&
                event.clientDip.y >= m_toolbarDeleteRectDip.top && event.clientDip.y <= m_toolbarDeleteRectDip.bottom) {
                
                m_hoverHandle = selection::HandleType::None;
                m_state = ToolState::Hovering;
                m_isHoveringText = false;
                return input::EventResult::Handled;
            }
        }

        // 1. Update handle hover if an object is selected
        if (m_selectionModel.HasObjectSelection()) {
            RectF selBounds = m_selectionModel.GetSelectionBounds();
            float rot = 0.0f;
            if (!m_selectionModel.GetSelectedObjects().empty()) {
                rot = m_selectionModel.GetSelectedObjects().front().rotationDegrees;
            }

            selection::HandleType hit = m_transformHandles.HitTest(
                event.canvasPoint,
                selBounds,
                rot,
                selection::TransformHandles::kDefaultHandleSizeDip,
                selection::TransformHandles::kDefaultHitToleranceDip,
                true
            );

            m_hoverHandle = hit;
            if (hit != selection::HandleType::None && hit != selection::HandleType::Body) {
                m_state = ToolState::HoveringHandle;
                m_isHoveringText = false;
                return input::EventResult::Handled;
            } else if (hit == selection::HandleType::Body) {
                m_state = ToolState::Hovering;
                m_isHoveringText = false;
                return input::EventResult::Handled;
            }
        }

        m_hoverHandle = selection::HandleType::None;

        // 2. Hit-test text on page FIRST
        m_isHoveringText = false;
        if (context.canvasToPdf && context.getTextPage) {
            int pageIndex = -1;
            PointF pdfPt = context.canvasToPdf(event.canvasPoint, pageIndex);
            if (pageIndex >= 0) {
                auto* tp = context.getTextPage(pageIndex);
                if (tp && tp->GetCharIndexAtPos(pdfPt.x, pdfPt.y, 10.0, 10.0) >= 0) {
                    m_isHoveringText = true;
                    m_state = ToolState::Hovering;
                    return input::EventResult::Handled;
                }
            }
        }        // 3. Hover over live objects
        if (context.hitTestObjects) {
            auto hitObjs = context.hitTestObjects(event.canvasPoint);
            if (!hitObjs.empty()) {
                return input::EventResult::Ignored;
            }
        }

        m_state = ToolState::Idle;
        return input::EventResult::Ignored;
    }

    if (m_dragMode == DragMode::Move) {
        double zoom = context.getZoom ? context.getZoom() : 1.0;
        float unscaledX = static_cast<float>(event.canvasPoint.x / zoom);
        float unscaledY = static_cast<float>(event.canvasPoint.y / zoom);
        
        float dx = unscaledX - m_dragLastUnscaledCanvas.x;
        float dy = unscaledY - m_dragLastUnscaledCanvas.y;
        
        m_dragLastUnscaledCanvas = { unscaledX, unscaledY };
        m_dragLastCanvas = event.canvasPoint;

        // Update bounds of selected objects
        auto selected = m_selectionModel.GetSelectedObjects();
        m_selectionModel.ClearObjects();
        for (auto& obj : selected) {
            obj.pageBounds.left += dx;
            obj.pageBounds.right += dx;
            obj.pageBounds.top += dy;
            obj.pageBounds.bottom += dy;
            m_selectionModel.AddSelect(obj);
        }

        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }

    if (m_dragMode == DragMode::Resize) {
        auto selected = m_selectionModel.GetSelectedObjects();
        if (selected.empty()) return input::EventResult::Handled;

        float rot = m_dragInitialRotation;
        RectF b = m_dragInitialBounds;
        PointF oldCenter = { (b.left + b.right) * 0.5f, (b.top + b.bottom) * 0.5f };

        double zoom = context.getZoom ? context.getZoom() : 1.0;
        float unscaledX = static_cast<float>(event.canvasPoint.x / zoom);
        float unscaledY = static_cast<float>(event.canvasPoint.y / zoom);

        // Un-rotate the drag vector to local space
        PointF localStart = selection::TransformHandles::RotatePoint(m_dragStartUnscaledCanvas, oldCenter, -rot);
        PointF localEnd = selection::TransformHandles::RotatePoint(PointF{unscaledX, unscaledY}, oldCenter, -rot);
        float dx = localEnd.x - localStart.x;
        float dy = localEnd.y - localStart.y;

        switch (m_activeHandle) {
        case selection::HandleType::NW:
            b.left += dx; b.top += dy; break;
        case selection::HandleType::N:
            b.top += dy; break;
        case selection::HandleType::NE:
            b.right += dx; b.top += dy; break;
        case selection::HandleType::E:
            b.right += dx; break;
        case selection::HandleType::SE:
            b.right += dx; b.bottom += dy; break;
        case selection::HandleType::S:
            b.bottom += dy; break;
        case selection::HandleType::SW:
            b.left += dx; b.bottom += dy; break;
        case selection::HandleType::W:
            b.left += dx; break;
        default: break;
        }

        // Prevent negative dimensions
        if (b.right < b.left + 1.0f) {
            float mid = (b.left + b.right) * 0.5f;
            b.left = mid - 0.5f; b.right = mid + 0.5f;
        }
        if (b.bottom < b.top + 1.0f) {
            float mid = (b.top + b.bottom) * 0.5f;
            b.top = mid - 0.5f; b.bottom = mid + 0.5f;
        }

        // The new center in local space relative to old center
        PointF newLocalCenter = { (b.left + b.right) * 0.5f, (b.top + b.bottom) * 0.5f };
        
        // When rendered, the object rotates around its own center.
        // So the new canvas center must be the new local center rotated around the OLD center by 'rot'.
        PointF newCanvasCenter = selection::TransformHandles::RotatePoint(newLocalCenter, oldCenter, rot);
        
        // Shift b so that its midpoint equals newCanvasCenter (which inherently applies the pivot shift!)
        float hw = (b.right - b.left) * 0.5f;
        float hh = (b.bottom - b.top) * 0.5f;
        b.left = newCanvasCenter.x - hw;
        b.right = newCanvasCenter.x + hw;
        b.top = newCanvasCenter.y - hh;
        b.bottom = newCanvasCenter.y + hh;

        selected[0].pageBounds = b;
        m_selectionModel.Select(selected[0]);

        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }

    if (m_dragMode == DragMode::Rotate) {
        RectF b = m_dragInitialBounds;
        PointF center = { (b.left + b.right) * 0.5f, (b.top + b.bottom) * 0.5f };
        double zoom = context.getZoom ? context.getZoom() : 1.0;
        PointF unscaledCurrent = { static_cast<float>(event.canvasPoint.x / zoom), static_cast<float>(event.canvasPoint.y / zoom) };
        float rawAngle = selection::TransformHandles::ComputeRotationAngle(center, unscaledCurrent);

        bool shiftPressed = input::HasModifier(event.modifiers, input::KeyModifier::Shift);
        float finalAngle = shiftPressed ? selection::TransformHandles::SnapAngle15(rawAngle) : rawAngle;

        auto selected = m_selectionModel.GetSelectedObjects();
        if (!selected.empty()) {
            selected[0].rotationDegrees = finalAngle;
            m_selectionModel.Select(selected[0]);
        }

        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }

    return input::EventResult::Ignored;
}

input::EventResult SelectTool::OnPointerUp(const input::PointerEvent& event, ToolContext& context) {
    if (m_isDelegatingToText) {
        auto res = m_textSelectTool.OnPointerUp(event, context);
        m_isDelegatingToText = false;
        return res;
    }
    if (m_dragMode != DragMode::None) {
        if (context.captureService) {
            context.captureService->ReleaseCapture(this);
        }

        m_dragMode = DragMode::None;
        m_activeHandle = selection::HandleType::None;
        m_marqueeRect = { 0.0f, 0.0f, 0.0f, 0.0f };
        m_state = ToolState::Committed;
        m_state = ToolState::Idle;

        if (context.invalidateView) context.invalidateView();
        return input::EventResult::Handled;
    }
    (void)event;
    return input::EventResult::Ignored;
}

input::EventResult SelectTool::OnPointerDoubleClick(const input::PointerEvent& event, ToolContext& context) {
    if (m_isDelegatingToText || m_isHoveringText) {
        m_isDelegatingToText = true;
        // TextSelectTool handles multi-click in OnPointerDown, so we must forward it to OnPointerDown
        // to correctly register the second/third clicks.
        return m_textSelectTool.OnPointerDown(event, context);
    }
    if (context.hitTestObjects) {
        auto objects = context.hitTestObjects(event.canvasPoint);
        if (!objects.empty()) {
            if (context.openAnnotationPopup) {
                context.openAnnotationPopup(objects.front().id);
            }
            return input::EventResult::Handled;
        }
    }
    return input::EventResult::Ignored;
}

input::EventResult SelectTool::OnKeyDown(const input::KeyEvent& event, ToolContext& context) {
    if (m_textSelectTool.GetSelectionModel().HasSelection()) {
        auto res = m_textSelectTool.OnKeyDown(event, context);
        if (res == input::EventResult::Handled) {
            return res;
        }
    }

    if (event.virtualKey == VK_ESCAPE) {
        Cancel(context);
        m_selectionModel.Clear();
        return input::EventResult::Handled;
    }

    if (event.virtualKey == VK_DELETE || event.virtualKey == VK_BACK) {
        if (m_selectionModel.HasObjectSelection()) {
            m_selectionModel.ClearObjects();
            if (context.invalidateView) context.invalidateView();
            return input::EventResult::Handled;
        }
    }

    return input::EventResult::Ignored;
}

HCURSOR SelectTool::GetCursor(const PointF& point, ToolContext& context) const {
    if (m_isDelegatingToText || m_textSelectTool.GetSelectionModel().HasSelection()) {
        return m_textSelectTool.GetCursor(point, context);
    }

    // Check delete toolbar button hit
    if (m_dragMode == DragMode::None && m_selectionModel.HasObjectSelection() && context.canvasToDip) {
        PointF dipPt = context.canvasToDip(point);
        if (dipPt.x >= m_toolbarDeleteRectDip.left && dipPt.x <= m_toolbarDeleteRectDip.right &&
            dipPt.y >= m_toolbarDeleteRectDip.top && dipPt.y <= m_toolbarDeleteRectDip.bottom) {
            return ::LoadCursorW(NULL, IDC_HAND);
        }
    }

    float rot = 0.0f;
    if (!m_selectionModel.GetSelectedObjects().empty()) {
        rot = m_selectionModel.GetSelectedObjects().front().rotationDegrees;
    }

    selection::HandleType h = (m_dragMode != DragMode::None) ? m_activeHandle : m_hoverHandle;

    // During active drag, use cached state — no need to re-hit-test
    if (m_dragMode != DragMode::None) {
        return selection::CursorResolver::ResolveCursor(
            ToolType::Select, m_state, h, rot,
            false, false,
            (h == selection::HandleType::Body)
        );
    }

    // Live text hit-test: re-check under pointer at cursor query time
    // This ensures WM_SETCURSOR always reflects the actual content under the mouse,
    // regardless of whether OnPointerMove has run yet.
    bool liveHoveringText = false;
    bool liveHoveringObject = false;
    if (context.canvasToPdf && context.getTextPage) {
        int pageIndex = -1;
        PointF pdfPt = context.canvasToPdf(point, pageIndex);
        if (pageIndex >= 0) {
            auto* tp = context.getTextPage(pageIndex);
            if (tp && tp->GetCharIndexAtPos(pdfPt.x, pdfPt.y, 10.0, 10.0) >= 0) {
                liveHoveringText = true;
            }
        }
    }
    if (!liveHoveringText) {
        liveHoveringObject = (h == selection::HandleType::Body ||
            (m_state == ToolState::Hovering && !m_isHoveringText));
    }

    return selection::CursorResolver::ResolveCursor(
        ToolType::Select, m_state, h, rot,
        liveHoveringText, m_isHoveringLink,
        liveHoveringObject
    );
}

void SelectTool::RenderOverlay(ID2D1RenderTarget* renderTarget, ToolContext& context) {
    if (!renderTarget) return;

    if (m_isDelegatingToText || m_textSelectTool.GetSelectionModel().HasSelection()) {
        m_textSelectTool.RenderOverlay(renderTarget, context);
    }
    // 2. Render transform handles for selected objects
    if (m_selectionModel.HasObjectSelection()) {
        RectF selBounds = m_selectionModel.GetSelectionBounds();
        float rot = 0.0f;
        bool isMarkup = false;
        if (!m_selectionModel.GetSelectedObjects().empty()) {
            rot = m_selectionModel.GetSelectedObjects().front().rotationDegrees;
            isMarkup = m_selectionModel.GetSelectedObjects().front().isTextMarkup;
        }

        RectF dipBounds = selBounds;
        if (context.canvasToDip) {
            PointF p0 = context.canvasToDip(PointF{ selBounds.left, selBounds.top });
            PointF p1 = context.canvasToDip(PointF{ selBounds.right, selBounds.bottom });
            dipBounds = RectF{ (std::min)(p0.x, p1.x), (std::min)(p0.y, p1.y), (std::max)(p0.x, p1.x), (std::max)(p0.y, p1.y) };
        }

        if (!isMarkup) {
            m_transformHandles.Render(
                renderTarget,
                dipBounds,
                rot,
                1.0f,
                true,
                true,
                selection::TransformHandles::kDefaultHandleSizeDip
            );
        } else {
            // For text markup, draw a subtle selection outline so they know it's selected, but no resize dots.
            ID2D1SolidColorBrush* outlineBrush = nullptr;
            renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.5f, 1.0f, 0.5f), &outlineBrush);
            if (outlineBrush) {
                renderTarget->DrawRectangle(
                    D2D1::RectF(dipBounds.left - 2, dipBounds.top - 2, dipBounds.right + 2, dipBounds.bottom + 2),
                    outlineBrush,
                    1.0f
                );
                outlineBrush->Release();
            }
        }

        // Render Contextual Mini-Toolbar (Delete Button)
        if (m_dragMode == DragMode::None) {
            float btnSize = 24.0f;
            float padding = 8.0f;
            // Position toolbar above the top-right of the bounds
            m_toolbarDeleteRectDip = {
                dipBounds.right - btnSize,
                dipBounds.top - btnSize - padding,
                dipBounds.right,
                dipBounds.top - padding
            };

            ID2D1SolidColorBrush* bgBrush = nullptr;
            ID2D1SolidColorBrush* fgBrush = nullptr;
            renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.9f, 0.2f, 0.2f, 0.9f), &bgBrush); // Red background
            renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &fgBrush); // White foreground

            if (bgBrush && fgBrush) {
                D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
                    D2D1::RectF(m_toolbarDeleteRectDip.left, m_toolbarDeleteRectDip.top, m_toolbarDeleteRectDip.right, m_toolbarDeleteRectDip.bottom),
                    4.0f, 4.0f
                );
                renderTarget->FillRoundedRectangle(&roundedRect, bgBrush);
                
                // Draw a simple 'X' for delete
                float cx = (m_toolbarDeleteRectDip.left + m_toolbarDeleteRectDip.right) * 0.5f;
                float cy = (m_toolbarDeleteRectDip.top + m_toolbarDeleteRectDip.bottom) * 0.5f;
                float s = 4.0f;
                renderTarget->DrawLine(D2D1::Point2F(cx - s, cy - s), D2D1::Point2F(cx + s, cy + s), fgBrush, 2.0f);
                renderTarget->DrawLine(D2D1::Point2F(cx - s, cy + s), D2D1::Point2F(cx + s, cy - s), fgBrush, 2.0f);
            }

            if (bgBrush) bgBrush->Release();
            if (fgBrush) fgBrush->Release();
        } else {
            m_toolbarDeleteRectDip = { 0,0,0,0 }; // Hide while dragging
        }
    } else {
        m_toolbarDeleteRectDip = { 0,0,0,0 };
    }
}

} // namespace ui::tools







