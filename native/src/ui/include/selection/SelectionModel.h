#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include "../../../core/Geometry.h"

namespace core::interfaces::dom {
class ITextPage;
}

namespace ui::selection {

// ----------------------------------------------------------------------------
// Selected Object Descriptor
// ----------------------------------------------------------------------------
struct SelectedObject {
    std::string id;
    int pageIndex = -1;
    RectF pageBounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    float rotationDegrees = 0.0f;
    std::shared_ptr<void> userData = nullptr;
    bool isTextMarkup = false;

    bool operator==(const SelectedObject& other) const {
        return id == other.id;
    }
};

// ----------------------------------------------------------------------------
// Text Selection Range
// ----------------------------------------------------------------------------
struct TextSelectionRange {
    int pageIndex = -1;
    int startCharIndex = -1;
    int endCharIndex = -1; // Inclusive
    std::wstring text;
    std::vector<RectF> rects; // Bounding boxes in page coordinates

    bool IsValid() const {
        return pageIndex >= 0 && startCharIndex >= 0 && endCharIndex >= startCharIndex;
    }

    int Length() const {
        return IsValid() ? (endCharIndex - startCharIndex + 1) : 0;
    }

    void Clear() {
        pageIndex = -1;
        startCharIndex = -1;
        endCharIndex = -1;
        text.clear();
        rects.clear();
    }
};

// ----------------------------------------------------------------------------
// Selection Mode & Click Classification
// ----------------------------------------------------------------------------
enum class SelectionMode {
    None = 0,
    Objects,
    Text
};

enum class TextClickType {
    Single = 1,   // Character / cursor positioning
    Double = 2,   // Word selection
    Triple = 3    // Line / paragraph selection
};

// ----------------------------------------------------------------------------
// SelectionModel
// ----------------------------------------------------------------------------
class SelectionModel {
public:
    SelectionModel();
    ~SelectionModel();

    // Callback notification
    std::function<void()> onSelectionChanged;

    // --- Object Selection ---
    void Select(const SelectedObject& obj);
    void Select(const std::string& id, int pageIndex, const RectF& bounds, float rotation = 0.0f);
    void AddSelect(const SelectedObject& obj);
    void AddSelect(const std::string& id, int pageIndex, const RectF& bounds, float rotation = 0.0f);
    void ToggleSelect(const SelectedObject& obj);
    void ToggleSelect(const std::string& id, int pageIndex, const RectF& bounds, float rotation = 0.0f);
    void Deselect(const std::string& id);
    void ClearObjects();

    bool IsSelected(const std::string& id) const;
    const std::vector<SelectedObject>& GetSelectedObjects() const { return m_selectedObjects; }
    size_t GetSelectedCount() const { return m_selectedObjects.size(); }
    bool HasObjectSelection() const { return !m_selectedObjects.empty(); }
    RectF GetSelectionBounds() const;
    int GetSelectionPageIndex() const;

    // --- Text Range Selection ---
    void SetTextSelection(const TextSelectionRange& range);
    void SetTextSelection(int pageIndex, int startChar, int endChar, core::interfaces::dom::ITextPage* textPage);
    void ClearTextSelection();
    const TextSelectionRange& GetTextSelection() const { return m_textSelection; }
    bool HasTextSelection() const { return m_textSelection.IsValid(); }
    std::wstring GetSelectedText() const { return m_textSelection.text; }

    // Multi-click text selection support
    void SelectCharacterAt(int pageIndex, int charIndex, core::interfaces::dom::ITextPage* textPage);
    void SelectWordAt(int pageIndex, int charIndex, core::interfaces::dom::ITextPage* textPage);
    void SelectLineAt(int pageIndex, int charIndex, core::interfaces::dom::ITextPage* textPage);
    void ExpandSelectionTo(int pageIndex, int targetCharIndex, core::interfaces::dom::ITextPage* textPage, TextClickType clickType = TextClickType::Single);

    // --- Boundary Finding Utilities ---
    static std::pair<int, int> FindWordBoundaries(const std::wstring& text, int charIndex);
    static std::pair<int, int> FindLineBoundaries(const std::wstring& text, int charIndex);

    // --- Unified API ---
    void Clear();
    SelectionMode GetSelectionMode() const;
    bool HasSelection() const;

private:
    void NotifyChanged();

    std::vector<SelectedObject> m_selectedObjects;
    TextSelectionRange m_textSelection;
    int m_anchorCharIndex = -1;
    int m_anchorPageIndex = -1;
};

} // namespace ui::selection
