#include "../../include/selection/SelectionModel.h"
#include "../../../core/interfaces/dom/ITextPage.h"
#include <cwctype>
#include <cmath>

namespace ui::selection {

SelectionModel::SelectionModel() = default;
SelectionModel::~SelectionModel() = default;

void SelectionModel::NotifyChanged() {
    if (onSelectionChanged) {
        onSelectionChanged();
    }
}

// ----------------------------------------------------------------------------
// Object Selection Implementation
// ----------------------------------------------------------------------------

void SelectionModel::Select(const SelectedObject& obj) {
    m_selectedObjects.clear();
    m_textSelection.Clear();
    m_selectedObjects.push_back(obj);
    NotifyChanged();
}

void SelectionModel::Select(const std::string& id, int pageIndex, const RectF& bounds, float rotation) {
    SelectedObject obj;
    obj.id = id;
    obj.pageIndex = pageIndex;
    obj.pageBounds = bounds;
    obj.rotationDegrees = rotation;
    Select(obj);
}

void SelectionModel::AddSelect(const SelectedObject& obj) {
    m_textSelection.Clear();
    auto it = std::find_if(m_selectedObjects.begin(), m_selectedObjects.end(),
        [&](const SelectedObject& o) { return o.id == obj.id; });
    if (it == m_selectedObjects.end()) {
        m_selectedObjects.push_back(obj);
        NotifyChanged();
    }
}

void SelectionModel::AddSelect(const std::string& id, int pageIndex, const RectF& bounds, float rotation) {
    SelectedObject obj;
    obj.id = id;
    obj.pageIndex = pageIndex;
    obj.pageBounds = bounds;
    obj.rotationDegrees = rotation;
    AddSelect(obj);
}

void SelectionModel::ToggleSelect(const SelectedObject& obj) {
    m_textSelection.Clear();
    auto it = std::find_if(m_selectedObjects.begin(), m_selectedObjects.end(),
        [&](const SelectedObject& o) { return o.id == obj.id; });
    if (it != m_selectedObjects.end()) {
        m_selectedObjects.erase(it);
    } else {
        m_selectedObjects.push_back(obj);
    }
    NotifyChanged();
}

void SelectionModel::ToggleSelect(const std::string& id, int pageIndex, const RectF& bounds, float rotation) {
    SelectedObject obj;
    obj.id = id;
    obj.pageIndex = pageIndex;
    obj.pageBounds = bounds;
    obj.rotationDegrees = rotation;
    ToggleSelect(obj);
}

void SelectionModel::Deselect(const std::string& id) {
    auto it = std::remove_if(m_selectedObjects.begin(), m_selectedObjects.end(),
        [&](const SelectedObject& o) { return o.id == id; });
    if (it != m_selectedObjects.end()) {
        m_selectedObjects.erase(it, m_selectedObjects.end());
        NotifyChanged();
    }
}

void SelectionModel::ClearObjects() {
    if (!m_selectedObjects.empty()) {
        m_selectedObjects.clear();
        NotifyChanged();
    }
}

bool SelectionModel::IsSelected(const std::string& id) const {
    return std::any_of(m_selectedObjects.begin(), m_selectedObjects.end(),
        [&](const SelectedObject& o) { return o.id == id; });
}

RectF SelectionModel::GetSelectionBounds() const {
    if (m_selectedObjects.empty()) {
        return { 0.0f, 0.0f, 0.0f, 0.0f };
    }

    float l = m_selectedObjects[0].pageBounds.left;
    float t = m_selectedObjects[0].pageBounds.top;
    float r = m_selectedObjects[0].pageBounds.right;
    float b = m_selectedObjects[0].pageBounds.bottom;

    for (size_t i = 1; i < m_selectedObjects.size(); ++i) {
        const auto& ob = m_selectedObjects[i].pageBounds;
        l = (std::min)(l, ob.left);
        t = (std::min)(t, ob.top);
        r = (std::max)(r, ob.right);
        b = (std::max)(b, ob.bottom);
    }

    return { l, t, r, b };
}

int SelectionModel::GetSelectionPageIndex() const {
    if (m_selectedObjects.empty()) {
        return -1;
    }
    return m_selectedObjects.front().pageIndex;
}

// ----------------------------------------------------------------------------
// Text Range Selection Implementation
// ----------------------------------------------------------------------------

void SelectionModel::SetTextSelection(const TextSelectionRange& range) {
    m_selectedObjects.clear();
    m_textSelection = range;
    m_anchorPageIndex = range.pageIndex;
    m_anchorCharIndex = range.startCharIndex;
    NotifyChanged();
}

void SelectionModel::SetTextSelection(int pageIndex, int startChar, int endChar, core::interfaces::dom::ITextPage* textPage) {
    if (pageIndex < 0 || !textPage) {
        ClearTextSelection();
        return;
    }

    int totalChars = textPage->GetCharCount();
    if (totalChars <= 0) {
        ClearTextSelection();
        return;
    }

    int s = (std::clamp)(startChar, 0, totalChars - 1);
    int e = (std::clamp)(endChar, 0, totalChars - 1);
    if (s > e) std::swap(s, e);

    m_selectedObjects.clear();
    m_textSelection.pageIndex = pageIndex;
    m_textSelection.startCharIndex = s;
    m_textSelection.endCharIndex = e;

    int count = e - s + 1;
    m_textSelection.text = textPage->GetText(s, count);
    m_textSelection.rects = textPage->GetRects(s, count);

    NotifyChanged();
}

void SelectionModel::ClearTextSelection() {
    if (m_textSelection.IsValid()) {
        m_textSelection.Clear();
        m_anchorPageIndex = -1;
        m_anchorCharIndex = -1;
        NotifyChanged();
    }
}

// ----------------------------------------------------------------------------
// Multi-Click Boundary Finding Utilities
// ----------------------------------------------------------------------------

static inline bool IsWordCharacter(wchar_t ch) {
    return std::iswalnum(ch) != 0 || ch == L'_';
}

std::pair<int, int> SelectionModel::FindWordBoundaries(const std::wstring& text, int charIndex) {
    int length = static_cast<int>(text.length());
    if (length == 0 || charIndex < 0 || charIndex >= length) {
        return { 0, 0 };
    }

    wchar_t target = text[charIndex];
    int start = charIndex;
    int end = charIndex;

    if (IsWordCharacter(target)) {
        while (start > 0 && IsWordCharacter(text[start - 1])) {
            --start;
        }
        while (end + 1 < length && IsWordCharacter(text[end + 1])) {
            ++end;
        }
    } else if (std::iswspace(target)) {
        while (start > 0 && std::iswspace(text[start - 1]) && text[start - 1] != L'\n' && text[start - 1] != L'\r') {
            --start;
        }
        while (end + 1 < length && std::iswspace(text[end + 1]) && text[end + 1] != L'\n' && text[end + 1] != L'\r') {
            ++end;
        }
    } else {
        // Punctuation / symbol: single character or matched pair
        start = charIndex;
        end = charIndex;
    }

    return { start, end };
}

std::pair<int, int> SelectionModel::FindLineBoundaries(const std::wstring& text, int charIndex) {
    int length = static_cast<int>(text.length());
    if (length == 0 || charIndex < 0 || charIndex >= length) {
        return { 0, 0 };
    }

    int start = charIndex;
    while (start > 0 && text[start - 1] != L'\n' && text[start - 1] != L'\r') {
        --start;
    }

    int end = charIndex;
    while (end + 1 < length && text[end + 1] != L'\n' && text[end + 1] != L'\r') {
        ++end;
    }

    return { start, end };
}

void SelectionModel::SelectCharacterAt(int pageIndex, int charIndex, core::interfaces::dom::ITextPage* textPage) {
    if (!textPage || pageIndex < 0 || charIndex < 0) {
        ClearTextSelection();
        return;
    }
    m_anchorPageIndex = pageIndex;
    m_anchorCharIndex = charIndex;
    SetTextSelection(pageIndex, charIndex, charIndex, textPage);
}

void SelectionModel::SelectWordAt(int pageIndex, int charIndex, core::interfaces::dom::ITextPage* textPage) {
    if (!textPage || pageIndex < 0 || charIndex < 0) {
        ClearTextSelection();
        return;
    }

    int totalChars = textPage->GetCharCount();
    if (totalChars <= 0 || charIndex >= totalChars) {
        ClearTextSelection();
        return;
    }

    std::wstring fullPageText = textPage->GetText(0, totalChars);
    auto [start, end] = FindWordBoundaries(fullPageText, charIndex);

    m_anchorPageIndex = pageIndex;
    m_anchorCharIndex = start;
    SetTextSelection(pageIndex, start, end, textPage);
}

void SelectionModel::SelectLineAt(int pageIndex, int charIndex, core::interfaces::dom::ITextPage* textPage) {
    if (!textPage || pageIndex < 0 || charIndex < 0) {
        ClearTextSelection();
        return;
    }

    int totalChars = textPage->GetCharCount();
    if (totalChars <= 0 || charIndex >= totalChars) {
        ClearTextSelection();
        return;
    }

    std::wstring fullPageText = textPage->GetText(0, totalChars);
    auto [start, end] = FindLineBoundaries(fullPageText, charIndex);

    m_anchorPageIndex = pageIndex;
    m_anchorCharIndex = start;
    SetTextSelection(pageIndex, start, end, textPage);
}

void SelectionModel::ExpandSelectionTo(int pageIndex, int targetCharIndex, core::interfaces::dom::ITextPage* textPage, TextClickType clickType) {
    if (!textPage || pageIndex < 0 || targetCharIndex < 0) {
        return;
    }

    int totalChars = textPage->GetCharCount();
    if (totalChars <= 0) return;

    if (m_anchorPageIndex != pageIndex || m_anchorCharIndex < 0) {
        m_anchorPageIndex = pageIndex;
        m_anchorCharIndex = targetCharIndex;
    }

    int start = (std::min)(m_anchorCharIndex, targetCharIndex);
    int end = (std::max)(m_anchorCharIndex, targetCharIndex);

    if (clickType == TextClickType::Double) {
        std::wstring fullText = textPage->GetText(0, totalChars);
        auto [anchorStart, anchorEnd] = FindWordBoundaries(fullText, m_anchorCharIndex);
        auto [targetStart, targetEnd] = FindWordBoundaries(fullText, targetCharIndex);
        start = (std::min)(anchorStart, targetStart);
        end = (std::max)(anchorEnd, targetEnd);
    } else if (clickType == TextClickType::Triple) {
        std::wstring fullText = textPage->GetText(0, totalChars);
        auto [anchorStart, anchorEnd] = FindLineBoundaries(fullText, m_anchorCharIndex);
        auto [targetStart, targetEnd] = FindLineBoundaries(fullText, targetCharIndex);
        start = (std::min)(anchorStart, targetStart);
        end = (std::max)(anchorEnd, targetEnd);
    }

    SetTextSelection(pageIndex, start, end, textPage);
}

// ----------------------------------------------------------------------------
// Unified API
// ----------------------------------------------------------------------------

void SelectionModel::Clear() {
    bool hadSelection = HasSelection();
    m_selectedObjects.clear();
    m_textSelection.Clear();
    m_anchorPageIndex = -1;
    m_anchorCharIndex = -1;
    if (hadSelection) {
        NotifyChanged();
    }
}

SelectionMode SelectionModel::GetSelectionMode() const {
    if (!m_selectedObjects.empty()) {
        return m_selectedObjects.size() == 1 ? SelectionMode::Objects : SelectionMode::Objects;
    }
    if (m_textSelection.IsValid()) {
        return SelectionMode::Text;
    }
    return SelectionMode::None;
}

bool SelectionModel::HasSelection() const {
    return !m_selectedObjects.empty() || m_textSelection.IsValid();
}

} // namespace ui::selection
