#pragma once
#include "IAnnotationHandler.h"
#include <vector>

namespace ui::annotation {

class InkAnnotationHandler : public IAnnotationHandler {
public:
    InkAnnotationHandler();
    ~InkAnnotationHandler() override = default;

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
    ToolMode GetToolType() const override { return ToolMode::Ink; }

    void SetStrokeColor(int r, int g, int b, int a) { m_r = r; m_g = g; m_b = b; m_a = a; }
    void SetStrokeWidth(float width) { m_strokeWidth = width; }
    const std::vector<PointF>& GetCurrentStroke() const { return m_currentStroke; }

private:
    AnnotationHandlerContext m_context;
    InteractionState m_state = InteractionState::Idle;

    int m_pageIndex = -1;
    std::vector<PointF> m_currentStroke; // Accumulated PDF coordinates

    int m_r = 0;
    int m_g = 0;
    int m_b = 0;
    int m_a = 255;
    float m_strokeWidth = 2.0f;
};

} // namespace ui::annotation
