#pragma once
#include <string>
#include <memory>
#include <cstddef>

namespace core {

class ICommand {
public:
    virtual ~ICommand() = default;

    // Executes the command (also used for Redo)
    virtual bool Execute() = 0;

    // Reverts the command
    virtual bool Undo() = 0;

    // Alternative name for Undo for PROJECT.md contract compliance
    virtual bool Unexecute() { return Undo(); }

    // Command Coalescing / Merging support (e.g. for drag / slider / text typing gestures)
    virtual bool CanMergeWith(const ICommand* /*other*/) const { return false; }
    virtual bool MergeWith(std::unique_ptr<ICommand>& /*other*/) { return false; }

    // Memory estimation for bounding limits
    virtual size_t GetMemorySize() const { return GetMemoryFootprint(); }
    virtual size_t GetMemoryFootprint() const { return sizeof(*this); }

    // Descriptions
    virtual std::string GetDescription() const { return ""; }
    virtual std::wstring GetName() const {
        std::string desc = GetDescription();
        return std::wstring(desc.begin(), desc.end());
    }
};

// Backwards compatibility alias for core::Command
using Command = ICommand;

} // namespace core

namespace core {
namespace interfaces {
namespace dom {

// Alias core::ICommand into core::interfaces::dom for legacy code
using ICommand = ::core::ICommand;

} // namespace dom
} // namespace interfaces
} // namespace core
