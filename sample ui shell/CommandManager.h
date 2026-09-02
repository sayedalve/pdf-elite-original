// CommandManager.h - Real command states, not fake
#pragma once
#include <functional>
#include <unordered_map>
#include <string>

namespace PdfElite {

enum class CommandId {
    // File
    OpenPdf,
    Save,
    SaveAs,
    CloseTab,
    NewTab,
    // Edit
    Undo,
    Redo,
    // View
    ZoomIn,
    ZoomOut,
    ZoomFit,
    ZoomActual,
    NextPage,
    PrevPage,
    GoToPage,
    // Tools - match toolbar exact
    ToolHand,
    ToolSelect,
    ToolRectSelect,
    ToolEditAll,
    ToolAddText,
    ToolOCR,
    ToolCrop,
    ToolCombine,
    ToolCompress,
    ToolHighlight,
    ToolAreaHighlight,
    ToolPencil,
    ToolEraser,
    ToolUnderline,
    ToolStrikethrough,
    ToolTextBox,
    ToolRectangle,
    ToolStamp,
    ToolImage,
    ToolSignature,
    ToolAttachment,
    ToolAddLink,
    ToolWatermark,
    ToolBackground,
    ToolExtract,
    ToolSplit,
    ToolInsert,
    ToolRotateLeft,
    ToolRotateRight,
    ToolDelete,
    // Modes
    ModeView,
    ModeComment,
    ModeEdit,
    ModeOrganize,
    // Search
    Search,
    SearchNext,
    SearchPrev,
    // Navigation
    NavHome,
    NavRecent,
    NavStarred,
    NavShared,
    NavTrash,
};

struct CommandState {
    bool enabled = true;
    bool active = false;
    bool visible = true;
    float disabledOpacity = 1.0f;
};

class CommandManager {
public:
    void SetState(CommandId id, const CommandState& state) { m_states[id] = state; }
    CommandState GetState(CommandId id) const {
        auto it = m_states.find(id);
        if (it != m_states.end()) return it->second;
        return CommandState{};
    }

    void SetHandler(CommandId id, std::function<void()> handler) { m_handlers[id] = handler; }
    void Execute(CommandId id) {
        auto it = m_handlers.find(id);
        if (it != m_handlers.end() && GetState(id).enabled) it->second();
    }

    bool IsEnabled(CommandId id) const { return GetState(id).enabled; }
    bool IsActive(CommandId id) const { return GetState(id).active; }

private:
    std::unordered_map<CommandId, CommandState> m_states;
    std::unordered_map<CommandId, std::function<void()>> m_handlers;
};

} // namespace PdfElite
