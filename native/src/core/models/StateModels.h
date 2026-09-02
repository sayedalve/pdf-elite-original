#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace core {
namespace state {

struct ZoomState {
    float level = 1.0f;
    enum class Mode { Fixed, FitWidth, FitPage } mode = Mode::Fixed;
};

struct SelectionState {
    bool hasSelection = false;
    // ...
};

struct SearchState {
    std::wstring currentQuery;
    bool isActive = false;
    int currentResultIndex = -1;
    int totalResults = 0;
};

struct ViewState {
    int currentPage = 0;
    int totalPages = 0;
    float scrollY = 0.0f;
    float scrollX = 0.0f;
    ZoomState zoom;
    SelectionState selection;
    SearchState search;
};

struct DocumentState {
    std::wstring documentId;
    std::wstring filePath;
    std::wstring title;
    bool isModified = false;
    uint64_t generation = 0;
    ViewState view;
};

struct TabState {
    std::vector<std::shared_ptr<DocumentState>> documents;
    std::wstring activeDocumentId;
};

// Global App State
struct AppState {
    TabState tabs;
    
    // Actions/Reducers can be managed by the UI controller or DocumentController
};

} // namespace state
} // namespace core
