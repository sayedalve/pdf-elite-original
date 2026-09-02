# Subsystem Migration Report: Unified SelectionModel & Multi-Click Classification

## 1. Subsystem Overview
- **Module:** `ui::selection::SelectionModel`
- **Location:** `native/src/ui/include/selection/SelectionModel.h`, `native/src/ui/src/selection/SelectionModel.cpp`
- **Scope:** Unified state container managing both discrete object selections (`SelectedObject`) and continuous text selections (`TextSelectionRange`) with multi-click heuristics.

## 2. Reference Project & Clean-Room License Audit
- **Inspiration Source:** Okular (`TextSelection`, `SelectionModel`, multi-click boundary detection)
- **Reference License:** GPL-2.0+
- **License Decision:** STRICT CLEAN-ROOM REIMPLEMENTATION. Implemented from first principles in native C++20 using standard library Unicode character inspection (`std::iswalnum`, whitespace, newlines) and PDFium text page APIs.

## 3. Architecture & Adaptations Made
- **Dual Selection Modes:** Supports `SelectionMode::Objects` and `SelectionMode::Text` with clear mutual exclusivity rules (selecting text clears objects, selecting objects clears text).
- **Multi-Click Classification:**
  - Single click: Character-level placement and drag expansion.
  - Double click: Word boundary expansion respecting alphanumeric/underscore delimiters.
  - Triple click: Line/paragraph boundary expansion respecting `\r`, `\n` linebreaks.
- **Multi-Object Union:** Aggregates selected bounding boxes across multiple items into an overall bounding rect for transformation.
- **Observer Notification:** `onSelectionChanged` callback informs UI controls (PropertiesPanel, Toolbars) immediately.

## 4. Old Code Removed / Superseded
- Removed duplicate `ui::interaction::SelectionModel.h` and legacy `ISelectableObject` vector management.
