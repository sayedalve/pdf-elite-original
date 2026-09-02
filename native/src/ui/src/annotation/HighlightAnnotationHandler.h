#pragma once
#include "IAnnotationHandler.h"
#include <vector>

namespace ui::annotation {

class HighlightAnnotationHandler : public IAnnotationHandler {
public:
    explicit HighlightAnnotationHandler(ToolMode toolMode = ToolMode::Highlight);
    ~HighlightAnnotationHandler() override = default;

    void Initialize(const AnnotationHandlerContext& context) override;
    bool OnMouseDown(const MouseEvent& event) override;
    bool OnMouseMove(const MouseEvent& event) override;
    bool OnMouseUp(const MouseEvent& event) override;
    bool OnKeyDown(int keyCode, bool shift, bool ctrl, bool alt) override;
    bool OnKeyUp(int keyCode, bool shift, bool ctrl, bool alt) override;
    HCURSOR OnSetCursor(const PointF& viewPoint) override;
    void RenderPreview(ID2D1RenderTarget* target, float zoom, const PointF& scrollOffset) override;
    void Cancel() override;
    InteractionState GetState() const override { return m_state; }
    ToolMode GetToolType() const override { return m_toolMode; }

    void SetToolMode(ToolMode mode) { m_toolMode = mode; }
    void SetHighlightColor(int r, int g, int b, int a) { m_r = r; m_g = g; m_b = b; m_a = a; }

private:
    AnnotationHandlerContext m_context;
    InteractionState m_state = InteractionState::Idle;
    ToolMode m_toolMode = ToolMode::Highlight;

    int m_startPage = -1;
    int m_startChar = -1;
    int m_endPage = -1;
    int m_endChar = -1;

    int m_r = 255;
    int m_g = 255;
    int m_b = 0;
    int m_a = 128; // Translucent yellow
};

} // namespace ui::annotation
