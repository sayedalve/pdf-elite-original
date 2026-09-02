#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>

namespace ui {
namespace commands {

class IActionObserver {
public:
    virtual ~IActionObserver() = default;
    virtual void OnActionStateChanged(const std::wstring& actionId, bool isEnabled, bool isChecked) = 0;
};

class Action {
public:
    Action(std::wstring id, std::function<void()> execute, std::function<bool()> isEnabled = nullptr, std::function<bool()> isChecked = nullptr)
        : m_id(std::move(id)), m_execute(std::move(execute)), m_isEnabled(std::move(isEnabled)), m_isChecked(std::move(isChecked)) {}

    void Execute() const { if (m_execute && IsEnabled()) m_execute(); }
    bool IsEnabled() const { return m_isEnabled ? m_isEnabled() : true; }
    bool IsChecked() const { return m_isChecked ? m_isChecked() : false; }
    const std::wstring& GetId() const { return m_id; }

private:
    std::wstring m_id;
    std::function<void()> m_execute;
    std::function<bool()> m_isEnabled;
    std::function<bool()> m_isChecked;
};

class CommandManager {
public:
    static CommandManager& Instance() {
        static CommandManager instance;
        return instance;
    }

    void RegisterAction(std::shared_ptr<Action> action) {
        m_actions[action->GetId()] = action;
        NotifyObservers(action->GetId());
    }

    void ExecuteAction(const std::wstring& actionId) {
        auto it = m_actions.find(actionId);
        if (it != m_actions.end()) {
            it->second->Execute();
        }
    }

    void AddObserver(IActionObserver* observer) {
        m_observers.push_back(observer);
    }

    void RemoveObserver(IActionObserver* observer) {
        auto it = std::find(m_observers.begin(), m_observers.end(), observer);
        if (it != m_observers.end()) {
            m_observers.erase(it);
        }
    }

    void UpdateAllStates() {
        for (const auto& pair : m_actions) {
            NotifyObservers(pair.first);
        }
    }

private:
    CommandManager() = default;

    void NotifyObservers(const std::wstring& actionId) {
        auto it = m_actions.find(actionId);
        if (it != m_actions.end()) {
            bool enabled = it->second->IsEnabled();
            bool checked = it->second->IsChecked();
            for (auto* obs : m_observers) {
                obs->OnActionStateChanged(actionId, enabled, checked);
            }
        }
    }

    std::unordered_map<std::wstring, std::shared_ptr<Action>> m_actions;
    std::vector<IActionObserver*> m_observers;
};

} // namespace commands
} // namespace ui
