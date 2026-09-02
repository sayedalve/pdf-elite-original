#pragma once
#include "IAnnotationHandler.h"
#include <string>

namespace ui::annotation {

class FreeTextAnnotationHandler : public IAnnotationHandler {
public:
    FreeTextAnnotationHandler();
    ~FreeTextAnnotationHandler() override = default;

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
    ToolMode GetToolType() const override { return ToolMode::FreeText; }

    void SetDefaultText(const std::string& text) { m_defaultText = text; }
    void SetFontSize(float fontSize) { m_fontSize = fontSize; }
    void SetTextColor(int r, int g, int b, int a) { m_textR = r; m_textG = g; m_textB = b; m_textA = a; }

private:
    AnnotationHandlerContext m_context;
    InteractionState m_state = InteractionState::Idle;

    int m_pageIndex = -1;
    PointF m_startPdf = {0.0f, 0.0f};
    PointF m_currentPdf = {0.0f, 0.0f};

    std::string m_defaultText = "Typewriter Text";
    float m_fontSize = 12.0f;
    int m_textR = 0;
    int m_textG = 0;
    int m_textB = 0;
    int m_textA = 255;
    float m_defaultWidth = 120.0f;
    float m_defaultHeight = 30.0f;
};

} // namespace ui::annotation
