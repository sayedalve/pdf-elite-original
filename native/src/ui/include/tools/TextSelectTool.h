#pragma once

#include "ITool.h"
#include "../selection/SelectionModel.h"
#include "../selection/CursorResolver.h"
#include "../../../core/interfaces/dom/IAnnotation.h"
namespace ui::tools {

class TextSelectTool : public ITool {
public:
    enum class AutoAction { None, Highlight, Underline, StrikeOut };
    TextSelectTool(AutoAction autoAction = AutoAction::None);
    ~TextSelectTool() override;

    ToolType GetType() const override { 
        switch (m_autoAction) {
            case AutoAction::Highlight: return ToolType::Highlight;
            case AutoAction::Underline: return ToolType::Underline;
            case AutoAction::StrikeOut: return ToolType::Strikeout;
            
            default: return ToolType::TextSelect;
        }
    }
    std::wstring GetName() const override { return L"TextSelect"; }
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

    bool HasToolbarHit(const PointF& dipPt) const;

    int GetClickCount() const { return m_clickCount; }

private:
    ToolState m_state = ToolState::Idle;
    bool m_isSelecting = false;

    selection::SelectionModel m_selectionModel;

    // Multi-click tracking
    int m_clickCount = 1;
    uint64_t m_lastClickTimeMs = 0;
    PointF m_lastClickDip = { 0.0f, 0.0f };

    int m_anchorPageIndex = -1;
    int m_anchorCharIndex = -1;
    selection::TextClickType m_activeClickType = selection::TextClickType::Single;

    // Mini-toolbar UI
public:
    struct ToolbarButton {
        enum class Action { Copy, Highlight, Underline, StrikeOut, ColorPick, Comment, RemoveItem };
        Action action;
        std::wstring tooltip;
        RectF rect;
        bool isHovered = false;
        bool isPressed = false;
        int r = 0, g = 0, b = 0, a = 255;
        int iconId = 0; // use int instead of enum to avoid include issues in header
        bool isSeparator = false;
    };

    std::vector<ToolbarButton> m_toolbarButtons;
    std::vector<ToolbarButton> m_colorButtons;
    bool m_colorPaletteOpen = false;
    PointF m_toolbarPosDip = {0,0};
    int m_currentColor[4] = {255, 255, 0, 255}; // Default yellow
    AutoAction m_autoAction = AutoAction::None;
    
    void BuildToolbar();
    void UpdateToolbarLayout(float minX, float minY, float maxX, float maxY, ToolContext& context);
    void DrawToolbar(ID2D1RenderTarget* renderTarget, ToolContext& context);
    void CreateAnnotation(core::interfaces::dom::AnnotationType type, ToolContext& context);
};

} // namespace ui::tools

