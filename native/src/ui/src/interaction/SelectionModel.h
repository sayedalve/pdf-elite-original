#pragma once
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <functional>
#include "ISelectableObject.h"

namespace ui {
namespace interaction {

class SelectionModel {
public:
    std::function<void()> onSelectionChanged;

    void Select(std::shared_ptr<ISelectableObject> obj) {
        Clear();
        if (obj) m_selected.push_back(obj);
        if (onSelectionChanged) onSelectionChanged();
    }

    void ToggleSelect(std::shared_ptr<ISelectableObject> obj) {
        if (!obj) return;
        auto it = std::find_if(m_selected.begin(), m_selected.end(), [&](const auto& o) { return o->GetId() == obj->GetId(); });
        if (it != m_selected.end()) m_selected.erase(it);
        else m_selected.push_back(obj);
        if (onSelectionChanged) onSelectionChanged();
    }

    void AddSelect(std::shared_ptr<ISelectableObject> obj) {
        if (!obj) return;
        auto it = std::find_if(m_selected.begin(), m_selected.end(), [&](const auto& o) { return o->GetId() == obj->GetId(); });
        if (it == m_selected.end()) {
            m_selected.push_back(obj);
            if (onSelectionChanged) onSelectionChanged();
        }
    }

    void Clear() { 
        if (!m_selected.empty()) {
            m_selected.clear(); 
            if (onSelectionChanged) onSelectionChanged();
        }
    }
    
    void Deselect(const std::string& id) {
        auto it = std::remove_if(m_selected.begin(), m_selected.end(), [&](const auto& o) { return o->GetId() == id; });
        if (it != m_selected.end()) {
            m_selected.erase(it, m_selected.end());
            if (onSelectionChanged) onSelectionChanged();
        }
    }
    
    const std::vector<std::shared_ptr<ISelectableObject>>& GetSelected() const { return m_selected; }
    
    bool IsSelected(const std::string& id) const {
        return std::any_of(m_selected.begin(), m_selected.end(), [&](const auto& o) { return o->GetId() == id; });
    }

private:
    std::vector<std::shared_ptr<ISelectableObject>> m_selected;
};

} // namespace interaction
} // namespace ui
