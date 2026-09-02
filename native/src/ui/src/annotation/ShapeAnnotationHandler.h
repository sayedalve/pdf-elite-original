#pragma once
#include "IAnnotationHandler.h"

namespace ui::annotation {

class ShapeAnnotationHandler : public IAnnotationHandler {
public:
    explicit ShapeAnnotationHandler(ToolMode toolMode = ToolMode::Rectangle);
    ~ShapeAnnotationHandler() override = default;

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
    void SetStrokeColor(int r, int g, int b, int a) { m_strokeR = r; m_strokeG = g; m_strokeB = b; m_strokeA = a; }
    void SetFillColor(int r, int g, int b, int a) { m_fillR = r; m_fillG = g; m_fillB = b; m_fillA = a; m_hasFill = true; }
    void SetBorderWidth(float width) { m_borderWidth = width; }

private:
    AnnotationHandlerContext m_context;
    InteractionState m_state = InteractionState::Idle;
    ToolMode m_toolMode = ToolMode::Rectangle;

    int m_pageIndex = -1;
    PointF m_startPdf = {0.0f, 0.0f};
    PointF m_currentPdf = {0.0f, 0.0f};
    bool m_shiftPressed = false;

    int m_strokeR = 255;
    int m_strokeG = 0;
    int m_strokeB = 0;
    int m_strokeA = 255;
    
    int m_fillR = 255;
    int m_fillG = 0;
    int m_fillB = 0;
    int m_fillA = 40;
    bool m_hasFill = false;

    float m_borderWidth = 2.0f;
};

} // namespace ui::annotation
