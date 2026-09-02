#pragma once

#include "ITool.h"
#include "../selection/SelectionModel.h"
#include "../selection/TransformHandles.h"
#include "../selection/CursorResolver.h"
#include "ToolStateMachine.h"
#include "TextSelectTool.h"

namespace ui::tools {

class SelectTool : public ITool {
public:
    SelectTool();
    ~SelectTool() override;

    ToolType GetType() const override { return ToolType::Select; }
    std::wstring GetName() const override { return L"Select"; }
    ToolState GetState() const override { return m_state; }

    void OnActivate(ToolContext& context) override;
    void OnDeactivate(ToolContext& context) override;
    void Cancel(ToolContext& context) override;

    input::EventResult OnPointerDown(const input::PointerEvent& event, ToolContext& context) override;
    input::EventResult OnPointerMove(const input::PointerEvent& event, ToolContext& context) override;
    input::EventResult OnPointerUp(const input::PointerEvent& event, ToolContext& context) override;
    input::EventResult OnPointerDoubleClick(const input::PointerEvent& event, ToolContext& context) override;

    input::EventResult OnKeyDown(const input::KeyEvent& event, ToolContext& context) override;

    HCURSOR GetCursor(const PointF& point, ToolContext& context) const override;
    void RenderOverlay(ID2D1RenderTarget* renderTarget, ToolContext& context) override;

    // Selection accessors
    selection::SelectionModel& GetSelectionModel() { return m_selectionModel; }
    const selection::SelectionModel& GetSelectionModel() const { return m_selectionModel; }

    selection::TransformHandles& GetTransformHandles() { return m_transformHandles; }
    const selection::TransformHandles& GetTransformHandles() const { return m_transformHandles; }

private:
    // Mini toolbar for annotations
    std::vector<TextSelectTool::ToolbarButton> m_annotColorButtons;
    bool m_annotToolbarOpen = false;
    D2D1_POINT_2F m_annotToolbarPos = {0,0};

    enum class DragMode {
        None = 0,
        Marquee,
        Move,
        Resize,
        Rotate
    };

    ToolState m_state = ToolState::Idle;
    DragMode m_dragMode = DragMode::None;
    selection::HandleType m_activeHandle = selection::HandleType::None;
    selection::HandleType m_hoverHandle = selection::HandleType::None;

    selection::SelectionModel m_selectionModel;
    selection::TransformHandles m_transformHandles;

    PointF m_dragStartCanvas = { 0.0f, 0.0f };
    PointF m_dragStartUnscaledCanvas = { 0.0f, 0.0f };
    PointF m_dragLastCanvas = { 0.0f, 0.0f };
    PointF m_dragLastUnscaledCanvas = { 0.0f, 0.0f };
    RectF m_dragInitialBounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    float m_dragInitialRotation = 0.0f;
    RectF m_marqueeRect = { 0.0f, 0.0f, 0.0f, 0.0f };

    bool m_isHoveringText = false;
    bool m_isHoveringLink = false;
    bool m_isDelegatingToText = false;

    TextSelectTool m_textSelectTool;

    // Mini-toolbar UI
    RectF m_toolbarDeleteRectDip = { 0.0f, 0.0f, 0.0f, 0.0f };
};

} // namespace ui::tools
