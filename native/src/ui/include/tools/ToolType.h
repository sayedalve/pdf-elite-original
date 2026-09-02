#pragma once

#include <string>

namespace ui::tools {

enum class ToolType {
    None = 0,
    Select,
    Pan,
    TextSelect,
    AddText,
    EditText,
    Highlight,
    Underline,
    Strikeout,
    AreaHighlight,
    Shapes,
    Rectangle,
    Ellipse,
    Line,
    Arrow,
    Ink,
    FreeText,
    TypeWriter,
    TextBox,
    TextCallout,
    StickyNote,
    Stamp,
    Eraser,
    InsertImage
};

inline const wchar_t* ToolTypeToString(ToolType type) {
    switch (type) {
    case ToolType::Select: return L"Select";
    case ToolType::Pan: return L"Pan";
    case ToolType::TextSelect: return L"TextSelect";
    case ToolType::AddText: return L"AddText";
    case ToolType::EditText: return L"EditText";
    case ToolType::Highlight: return L"Highlight";
    case ToolType::Underline: return L"Underline";
    case ToolType::Strikeout: return L"Strikeout";
    case ToolType::AreaHighlight: return L"AreaHighlight";
    case ToolType::Shapes: return L"Shapes";
    case ToolType::Rectangle: return L"Rectangle";
    case ToolType::Ellipse: return L"Ellipse";
    case ToolType::Line: return L"Line";
    case ToolType::Arrow: return L"Arrow";
    case ToolType::Ink: return L"Ink";
    case ToolType::FreeText: return L"FreeText";
    case ToolType::TypeWriter: return L"TypeWriter";
    case ToolType::TextBox: return L"TextBox";
    case ToolType::TextCallout: return L"TextCallout";
    case ToolType::StickyNote: return L"StickyNote";
    case ToolType::Stamp: return L"Stamp";
    case ToolType::Eraser: return L"Eraser";
    case ToolType::InsertImage: return L"InsertImage";
    default: return L"None";
    }
}

} // namespace ui::tools
