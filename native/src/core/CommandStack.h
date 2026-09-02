#pragma once
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <cstdint>
#include "core/interfaces/dom/ICommand.h"

namespace core {

class MacroCommand : public ICommand {
public:
    explicit MacroCommand(std::string description = "Composite Action");
    ~MacroCommand() override = default;

    void AddCommand(std::unique_ptr<ICommand> cmd);
    size_t GetCommandCount() const;
    const ICommand* GetCommand(size_t index) const;

    bool Execute() override;
    bool Undo() override;

    bool CanMergeWith(const ICommand* other) const override;
    bool MergeWith(std::unique_ptr<ICommand>& other) override;

    size_t GetMemorySize() const override;
    std::string GetDescription() const override;

private:
    std::string m_description;
    std::vector<std::unique_ptr<ICommand>> m_commands;
};

class CommandGroup : public ICommand {
public:
    explicit CommandGroup(std::string description, uint64_t timestampMs, std::unique_ptr<ICommand> initialCmd);
    ~CommandGroup() override = default;

    bool Execute() override;
    bool Undo() override;

    bool CanMergeWith(const ICommand* other) const override;
    bool MergeWith(std::unique_ptr<ICommand>& other) override;

    size_t GetMemorySize() const override;
    std::string GetDescription() const override;

private:
    std::string m_description;
    uint64_t m_lastTimestampMs;
    std::vector<std::unique_ptr<ICommand>> m_commands;
};

class CommandStack {
public:
    // Default 100 commands max depth and 50 MB max memory limit
    explicit CommandStack(size_t maxDepth = 100, size_t maxMemoryBytes = 50 * 1024 * 1024);
    ~CommandStack();

    CommandStack(const CommandStack&) = delete;
    CommandStack& operator=(const CommandStack&) = delete;
    CommandStack(CommandStack&&) noexcept;
    CommandStack& operator=(CommandStack&&) noexcept;

    // Executes a new command and pushes it to undo stack.
    // Clears redo stack. Supports command coalescing / merging.
    bool ExecuteCommand(std::unique_ptr<ICommand> cmd);
    void PushAndExecute(std::unique_ptr<ICommand> cmd);

    bool Undo();
    bool Redo();

    bool CanUndo() const;
    bool CanRedo() const;

    void Clear();

    // Save tracking / dirty state
    void MarkSaved();
    bool IsDirty() const;
    int GetCurrentPosition() const;
    int GetSavePosition() const;

    // Generation tracking for tile invalidation
    uint64_t GetGeneration() const;

    // Memory & capacity metrics
    size_t GetMaxDepth() const;
    void SetMaxDepth(size_t depth);

    size_t GetMaxMemoryBytes() const;
    void SetMaxMemoryBytes(size_t bytes);

    size_t GetCurrentMemoryUsage() const;
    size_t GetUndoCount() const;
    size_t GetRedoCount() const;

    // Optional callback for UI / dirty state notifications
    void SetStateChangedCallback(std::function<void()> callback);

private:
    void PruneBounds();
    void NotifyStateChanged();

    std::vector<std::unique_ptr<ICommand>> m_undoStack;
    std::vector<std::unique_ptr<ICommand>> m_redoStack;

    size_t m_maxDepth;
    size_t m_maxMemoryBytes;
    size_t m_currentMemoryUsage = 0;

    int m_savePosition = 0;
    int m_currentPosition = 0;
    uint64_t m_generation = 0;

    std::function<void()> m_onStateChanged;
};

} // namespace core

namespace core {
namespace interfaces {
namespace dom {

// Alias core::CommandStack into core::interfaces::dom for legacy code
using CommandStack = ::core::CommandStack;

} // namespace dom
} // namespace interfaces
} // namespace core
