#include "CommandStack.h"
#include <algorithm>

namespace core {

// ============================================================================
// MacroCommand Implementation
// ============================================================================

MacroCommand::MacroCommand(std::string description)
    : m_description(std::move(description)) {
}

void MacroCommand::AddCommand(std::unique_ptr<ICommand> cmd) {
    if (cmd) {
        m_commands.push_back(std::move(cmd));
    }
}

size_t MacroCommand::GetCommandCount() const {
    return m_commands.size();
}

const ICommand* MacroCommand::GetCommand(size_t index) const {
    if (index < m_commands.size()) {
        return m_commands[index].get();
    }
    return nullptr;
}

bool MacroCommand::Execute() {
    size_t executedCount = 0;
    for (size_t i = 0; i < m_commands.size(); ++i) {
        if (m_commands[i] && m_commands[i]->Execute()) {
            executedCount++;
        } else {
            // Atomic Rollback: Undo all previously executed commands in reverse order
            for (size_t j = executedCount; j > 0; --j) {
                if (m_commands[j - 1]) {
                    m_commands[j - 1]->Undo();
                }
            }
            return false;
        }
    }
    return true;
}

bool MacroCommand::Undo() {
    bool allSuccess = true;
    for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it) {
        if (*it) {
            if (!(*it)->Undo()) {
                allSuccess = false;
            }
        }
    }
    return allSuccess;
}

bool MacroCommand::CanMergeWith(const ICommand* other) const {
    if (!other) return false;
    const auto* otherMacro = dynamic_cast<const MacroCommand*>(other);
    return otherMacro != nullptr;
}

bool MacroCommand::MergeWith(std::unique_ptr<ICommand>& other) {
    if (!other) return false;
    // Macro commands can merge if other provides additional commands
    // In typical usage, the caller creates a new MacroCommand with merged operations
    return false;
}

size_t MacroCommand::GetMemorySize() const {
    size_t total = sizeof(*this) + m_description.capacity();
    total += m_commands.capacity() * sizeof(std::unique_ptr<ICommand>);
    for (const auto& cmd : m_commands) {
        if (cmd) {
            total += cmd->GetMemorySize();
        }
    }
    return total;
}

std::string MacroCommand::GetDescription() const {
    return m_description;
}


// ============================================================================
// CommandGroup Implementation
// ============================================================================

CommandGroup::CommandGroup(std::string description, uint64_t timestampMs, std::unique_ptr<ICommand> initialCmd)
    : m_description(std::move(description)), m_lastTimestampMs(timestampMs) {
    if (initialCmd) {
        m_commands.push_back(std::move(initialCmd));
    }
}

bool CommandGroup::Execute() {
    bool allSuccess = true;
    for (auto& cmd : m_commands) {
        if (!cmd->Execute()) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool CommandGroup::Undo() {
    bool allSuccess = true;
    for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it) {
        if (!(*it)->Undo()) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool CommandGroup::CanMergeWith(const ICommand* other) const {
    if (!other) return false;
    // Note: CanMergeWith only checks type or time, doesn't modify
    return true; // We always allow MergeWith to try
}

bool CommandGroup::MergeWith(std::unique_ptr<ICommand>& other) {
    if (!other) return false;
    // Assume other is a standard command we want to append, or another group
    auto otherGroup = dynamic_cast<CommandGroup*>(other.get());
    if (otherGroup) {
        // Time threshold for grouping rapid operations (e.g. 500ms)
        const uint64_t GROUPING_THRESHOLD_MS = 500;
        if ((otherGroup->m_lastTimestampMs - m_lastTimestampMs) <= GROUPING_THRESHOLD_MS) {
            m_lastTimestampMs = otherGroup->m_lastTimestampMs;
            for (auto& childCmd : otherGroup->m_commands) {
                m_commands.push_back(std::move(childCmd));
            }
            return true;
        }
    }
    return false;
}

size_t CommandGroup::GetMemorySize() const {
    size_t total = sizeof(*this) + m_description.capacity();
    total += m_commands.capacity() * sizeof(std::unique_ptr<ICommand>);
    for (const auto& cmd : m_commands) {
        if (cmd) {
            total += cmd->GetMemorySize();
        }
    }
    return total;
}

std::string CommandGroup::GetDescription() const {
    return m_description;
}

// ============================================================================
// CommandStack Implementation
// ============================================================================

CommandStack::CommandStack(size_t maxDepth, size_t maxMemoryBytes)
    : m_maxDepth(maxDepth), m_maxMemoryBytes(maxMemoryBytes), m_currentMemoryUsage(0),
      m_savePosition(0), m_currentPosition(0), m_generation(0) {
}

CommandStack::~CommandStack() {
    Clear();
}

CommandStack::CommandStack(CommandStack&& other) noexcept
    : m_undoStack(std::move(other.m_undoStack)),
      m_redoStack(std::move(other.m_redoStack)),
      m_maxDepth(other.m_maxDepth),
      m_maxMemoryBytes(other.m_maxMemoryBytes),
      m_currentMemoryUsage(other.m_currentMemoryUsage),
      m_savePosition(other.m_savePosition),
      m_currentPosition(other.m_currentPosition),
      m_generation(other.m_generation),
      m_onStateChanged(std::move(other.m_onStateChanged)) {
    other.m_currentMemoryUsage = 0;
    other.m_currentPosition = 0;
    other.m_savePosition = 0;
    other.m_generation = 0;
}

CommandStack& CommandStack::operator=(CommandStack&& other) noexcept {
    if (this != &other) {
        Clear();
        m_undoStack = std::move(other.m_undoStack);
        m_redoStack = std::move(other.m_redoStack);
        m_maxDepth = other.m_maxDepth;
        m_maxMemoryBytes = other.m_maxMemoryBytes;
        m_currentMemoryUsage = other.m_currentMemoryUsage;
        m_savePosition = other.m_savePosition;
        m_currentPosition = other.m_currentPosition;
        m_generation = other.m_generation;
        m_onStateChanged = std::move(other.m_onStateChanged);

        other.m_currentMemoryUsage = 0;
        other.m_currentPosition = 0;
        other.m_savePosition = 0;
        other.m_generation = 0;
    }
    return *this;
}

bool CommandStack::ExecuteCommand(std::unique_ptr<ICommand> cmd) {
    if (!cmd) return false;

    bool alreadyExecuted = false;

    // Check for Command Coalescing / Merging
    if (!m_undoStack.empty() && m_undoStack.back()->CanMergeWith(cmd.get())) {
        size_t oldMem = m_undoStack.back()->GetMemorySize();
        if (cmd->Execute()) {
            alreadyExecuted = true;
            if (m_undoStack.back()->MergeWith(cmd)) {
                size_t newMem = m_undoStack.back()->GetMemorySize();
                if (newMem > oldMem) {
                    m_currentMemoryUsage += (newMem - oldMem);
                } else if (oldMem > newMem) {
                    size_t diff = oldMem - newMem;
                    m_currentMemoryUsage = (m_currentMemoryUsage >= diff) ? (m_currentMemoryUsage - diff) : 0;
                }

                m_redoStack.clear();
                m_generation++;
                PruneBounds();
                NotifyStateChanged();
                return true;
            }
        }
    }

    // Standard execution
    if (cmd && (alreadyExecuted || cmd->Execute())) {
        m_currentMemoryUsage += cmd->GetMemorySize();
        m_undoStack.push_back(std::move(cmd));
        m_redoStack.clear();
        m_currentPosition++;
        m_generation++;

        PruneBounds();
        NotifyStateChanged();
        return true;
    }

    return false;
}

void CommandStack::PushAndExecute(std::unique_ptr<ICommand> cmd) {
    ExecuteCommand(std::move(cmd));
}

bool CommandStack::Undo() {
    if (!CanUndo()) return false;

    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    if (cmd->Undo()) {
        m_redoStack.push_back(std::move(cmd));
        m_currentPosition--;
        m_generation++;
        NotifyStateChanged();
        return true;
    } else {
        // If undo fails, discard the corrupted command
        size_t mem = cmd->GetMemorySize();
        if (m_currentMemoryUsage >= mem) {
            m_currentMemoryUsage -= mem;
        } else {
            m_currentMemoryUsage = 0;
        }
        m_generation++;
        NotifyStateChanged();
        return false;
    }
}

bool CommandStack::Redo() {
    if (!CanRedo()) return false;

    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    if (cmd->Execute()) {
        m_undoStack.push_back(std::move(cmd));
        m_currentPosition++;
        m_generation++;
        NotifyStateChanged();
        return true;
    } else {
        // Redo failed: discard corrupted command
        size_t mem = cmd->GetMemorySize();
        if (m_currentMemoryUsage >= mem) {
            m_currentMemoryUsage -= mem;
        } else {
            m_currentMemoryUsage = 0;
        }
        m_generation++;
        NotifyStateChanged();
        return false;
    }
}

bool CommandStack::CanUndo() const {
    return !m_undoStack.empty();
}

bool CommandStack::CanRedo() const {
    return !m_redoStack.empty();
}

void CommandStack::Clear() {
    m_undoStack.clear();
    m_redoStack.clear();
    m_currentMemoryUsage = 0;
    m_currentPosition = 0;
    m_savePosition = 0;
    m_generation++;
    NotifyStateChanged();
}

void CommandStack::MarkSaved() {
    m_savePosition = m_currentPosition;
    NotifyStateChanged();
}

bool CommandStack::IsDirty() const {
    return m_currentPosition != m_savePosition;
}

int CommandStack::GetCurrentPosition() const {
    return m_currentPosition;
}

int CommandStack::GetSavePosition() const {
    return m_savePosition;
}

uint64_t CommandStack::GetGeneration() const {
    return m_generation;
}

size_t CommandStack::GetMaxDepth() const {
    return m_maxDepth;
}

void CommandStack::SetMaxDepth(size_t depth) {
    m_maxDepth = depth;
    PruneBounds();
}

size_t CommandStack::GetMaxMemoryBytes() const {
    return m_maxMemoryBytes;
}

void CommandStack::SetMaxMemoryBytes(size_t bytes) {
    m_maxMemoryBytes = bytes;
    PruneBounds();
}

size_t CommandStack::GetCurrentMemoryUsage() const {
    return m_currentMemoryUsage;
}

size_t CommandStack::GetUndoCount() const {
    return m_undoStack.size();
}

size_t CommandStack::GetRedoCount() const {
    return m_redoStack.size();
}

void CommandStack::SetStateChangedCallback(std::function<void()> callback) {
    m_onStateChanged = std::move(callback);
}

void CommandStack::PruneBounds() {
    while (m_undoStack.size() > 1 &&
          (m_undoStack.size() > m_maxDepth || m_currentMemoryUsage > m_maxMemoryBytes)) {
        size_t mem = m_undoStack.front()->GetMemorySize();
        if (m_currentMemoryUsage >= mem) {
            m_currentMemoryUsage -= mem;
        } else {
            m_currentMemoryUsage = 0;
        }

        m_undoStack.erase(m_undoStack.begin());

        if (m_savePosition > 0) {
            m_savePosition--;
        } else if (m_savePosition == 0) {
            // Save state was evicted: document cannot return to initial saved state
            m_savePosition = -1;
        }

        if (m_currentPosition > 0) {
            m_currentPosition--;
        }
    }
}

void CommandStack::NotifyStateChanged() {
    if (m_onStateChanged) {
        m_onStateChanged();
    }
}

} // namespace core
