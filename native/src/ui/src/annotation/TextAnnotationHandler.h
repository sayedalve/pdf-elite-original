#pragma once
#include "IAnnotationHandler.h"
#include <string>

namespace ui::annotation {

class TextAnnotationHandler : public IAnnotationHandler {
public:
    TextAnnotationHandler();
    ~TextAnnotationHandler() override = default;

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
    ToolMode GetToolType() const override { return ToolMode::StickyNote; }

    void SetDefaultAuthor(const std::string& author) { m_defaultAuthor = author; }
    void SetDefaultContents(const std::string& contents) { m_defaultContents = contents; }
    void SetIconName(const std::string& iconName) { m_iconName = iconName; }

private:
    AnnotationHandlerContext m_context;
    InteractionState m_state = InteractionState::Idle;
    
    int m_pendingPageIndex = -1;
    PointF m_pendingPdfPos = {0.0f, 0.0f};
    PointF m_currentHoverPdfPos = {0.0f, 0.0f};
    int m_hoverPageIndex = -1;
    bool m_hasHover = false;

    std::string m_defaultAuthor = "PDF Elite User";
    std::string m_defaultContents = "Sticky Note";
    std::string m_iconName = "Note";
    float m_noteWidth = 24.0f;
    float m_noteHeight = 24.0f;
};

} // namespace ui::annotation
