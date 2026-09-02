#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include "../input/IInputInterceptor.h"
#include "ITool.h"
#include "ToolType.h"

namespace ui::tools {

class ToolStateMachine : public input::IInputInterceptor {
public:
    explicit ToolStateMachine(ToolContext context = {});
    ~ToolStateMachine() override;

    void SetContext(const ToolContext& context) { m_context = context; }
    ToolContext& GetContext() { return m_context; }
    const ToolContext& GetContext() const { return m_context; }

    void RegisterTool(std::unique_ptr<ITool> tool);
    bool SetActiveTool(ToolType type);
    bool PushTool(ToolType type);
    bool PopTool();

    ITool* GetActiveTool() const { return m_activeTool; }
    ToolType GetActiveToolType() const { return m_activeType; }
    ToolState GetActiveToolState() const;
    ITool* GetTool(ToolType type) const;

    void CancelActiveInteractions();

    input::EventResult RoutePointerDown(const input::PointerEvent& event);
    input::EventResult RoutePointerMove(const input::PointerEvent& event);
    input::EventResult RoutePointerUp(const input::PointerEvent& event);
    input::EventResult RoutePointerDoubleClick(const input::PointerEvent& event);

    input::EventResult RouteKeyDown(const input::KeyEvent& event);
    input::EventResult RouteKeyUp(const input::KeyEvent& event);
    input::EventResult RouteChar(const input::KeyEvent& event);
    input::EventResult RouteMouseWheel(const input::ScrollEvent& event);

    HCURSOR GetCursor(const PointF& point) const;
    void RenderOverlay(ID2D1RenderTarget* renderTarget);

private:
    ToolContext m_context;
    std::unordered_map<ToolType, std::unique_ptr<ITool>> m_tools;
    ITool* m_activeTool = nullptr;
    ToolType m_activeType = ToolType::None;
    std::vector<ToolType> m_toolStack;
};

} // namespace ui::tools
