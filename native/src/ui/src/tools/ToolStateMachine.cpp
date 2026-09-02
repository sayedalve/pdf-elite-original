#include "../../include/tools/ToolStateMachine.h"

namespace ui::tools {

ToolStateMachine::ToolStateMachine(ToolContext context)
    : m_context(std::move(context)), m_activeTool(nullptr), m_activeType(ToolType::None) {
}

ToolStateMachine::~ToolStateMachine() {
    if (m_activeTool) {
        m_activeTool->Cancel(m_context);
        m_activeTool->OnDeactivate(m_context);
        m_activeTool = nullptr;
    }
}

void ToolStateMachine::RegisterTool(std::unique_ptr<ITool> tool) {
    if (!tool) return;
    ToolType type = tool->GetType();
    m_tools[type] = std::move(tool);
}

bool ToolStateMachine::SetActiveTool(ToolType type) {
    if (m_activeType == type && m_activeTool != nullptr && m_toolStack.empty()) {
        return true;
    }

    if (m_activeTool) {
        m_activeTool->Cancel(m_context);
        m_activeTool->OnDeactivate(m_context);
        m_activeTool = nullptr;
        m_activeType = ToolType::None;
    }
    
    m_toolStack.clear();

    auto it = m_tools.find(type);
    if (it != m_tools.end() && it->second) {
        m_activeTool = it->second.get();
        m_activeType = type;
        m_activeTool->OnActivate(m_context);
        if (m_context.invalidateView) {
            m_context.invalidateView();
        }
        return true;
    }

    return false;
}

bool ToolStateMachine::PushTool(ToolType type) {
    if (m_activeType != ToolType::None) {
        m_toolStack.push_back(m_activeType);
        if (m_activeTool) {
            m_activeTool->Cancel(m_context);
            m_activeTool->OnDeactivate(m_context);
        }
    }
    
    m_activeTool = nullptr;
    m_activeType = ToolType::None;
    
    auto it = m_tools.find(type);
    if (it != m_tools.end() && it->second) {
        m_activeTool = it->second.get();
        m_activeType = type;
        m_activeTool->OnActivate(m_context);
        if (m_context.invalidateView) {
            m_context.invalidateView();
        }
        return true;
    }
    return false;
}

bool ToolStateMachine::PopTool() {
    if (m_toolStack.empty()) {
        return false;
    }
    
    if (m_activeTool) {
        m_activeTool->Cancel(m_context);
        m_activeTool->OnDeactivate(m_context);
    }
    
    ToolType previousType = m_toolStack.back();
    m_toolStack.pop_back();
    
    m_activeTool = nullptr;
    m_activeType = ToolType::None;
    
    auto it = m_tools.find(previousType);
    if (it != m_tools.end() && it->second) {
        m_activeTool = it->second.get();
        m_activeType = previousType;
        m_activeTool->OnActivate(m_context);
        if (m_context.invalidateView) {
            m_context.invalidateView();
        }
        return true;
    }
    return false;
}

ToolState ToolStateMachine::GetActiveToolState() const {
    if (m_activeTool) {
        return m_activeTool->GetState();
    }
    return ToolState::Idle;
}

ITool* ToolStateMachine::GetTool(ToolType type) const {
    auto it = m_tools.find(type);
    if (it != m_tools.end()) {
        return it->second.get();
    }
    return nullptr;
}

void ToolStateMachine::CancelActiveInteractions() {
    if (m_activeTool) {
        m_activeTool->Cancel(m_context);
    }
    if (m_context.captureService) {
        m_context.captureService->ForceRelease();
    }
}

input::EventResult ToolStateMachine::RoutePointerDown(const input::PointerEvent& event) {
    if (m_activeTool) {
        return m_activeTool->OnPointerDown(event, m_context);
    }
    return input::EventResult::Ignored;
}

input::EventResult ToolStateMachine::RoutePointerMove(const input::PointerEvent& event) {
    if (m_activeTool) {
        return m_activeTool->OnPointerMove(event, m_context);
    }
    return input::EventResult::Ignored;
}

input::EventResult ToolStateMachine::RoutePointerUp(const input::PointerEvent& event) {
    if (m_activeTool) {
        return m_activeTool->OnPointerUp(event, m_context);
    }
    return input::EventResult::Ignored;
}

input::EventResult ToolStateMachine::RoutePointerDoubleClick(const input::PointerEvent& event) {
    if (m_activeTool) {
        return m_activeTool->OnPointerDoubleClick(event, m_context);
    }
    return input::EventResult::Ignored;
}

input::EventResult ToolStateMachine::RouteKeyDown(const input::KeyEvent& event) {
    if (event.virtualKey == VK_ESCAPE) {
        if (m_activeTool) {
            m_activeTool->Cancel(m_context);
        }
        if (m_toolStack.size() > 1) {
            PopTool();
            return input::EventResult::Handled;
        }
        m_activeTool = nullptr;
        m_activeType = ToolType::None;
        m_toolStack.clear();
        return input::EventResult::Ignored;
    }

    if (m_activeTool) {
        return m_activeTool->OnKeyDown(event, m_context);
    }
    return input::EventResult::Ignored;
}

input::EventResult ToolStateMachine::RouteKeyUp(const input::KeyEvent& event) {
    if (m_activeTool) {
        return m_activeTool->OnKeyUp(event, m_context);
    }
    return input::EventResult::Ignored;
}

input::EventResult ToolStateMachine::RouteChar(const input::KeyEvent& event) {
    if (m_activeTool) {
        return m_activeTool->OnChar(event, m_context);
    }
    return input::EventResult::Ignored;
}

input::EventResult ToolStateMachine::RouteMouseWheel(const input::ScrollEvent& event) {
    if (m_activeTool) {
        return m_activeTool->OnMouseWheel(event, m_context);
    }
    return input::EventResult::Ignored;
}

HCURSOR ToolStateMachine::GetCursor(const PointF& point) const {
    if (m_activeTool) {
        return m_activeTool->GetCursor(point, const_cast<ToolContext&>(m_context));
    }
    return ::LoadCursor(nullptr, IDC_ARROW);
}

void ToolStateMachine::RenderOverlay(ID2D1RenderTarget* renderTarget) {
    if (m_activeTool) {
        m_activeTool->RenderOverlay(renderTarget, m_context);
    }
}

} // namespace ui::tools


