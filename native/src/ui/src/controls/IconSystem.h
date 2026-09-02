#pragma once
#include <d2d1_3.h>
#include <wrl/client.h>

namespace controls {

enum class IconId {
    Home, Comment, Edit, Convert, View, Organize, Tools, Form,
    RecentFiles, StarredFiles, RecentFolders, Spaces, Cloud, Agreement, Receipt,
    OpenFolder, CreateDoc, Plus, Close, Search, List, Grid, Filter, Refresh,
    Undo, Redo, ZoomOut, ZoomIn, Hand, RectSelect, EditAll, AddText, AddLink, Image, Watermark, Background,
    Ocr, Crop, Combine, Compress, Extract, Split, Insert, RotateLeft, RotateRight, Trash,
    Highlight, HighlightArea, Pencil, Eraser, Underline, Strikethrough, Text, TextBox, Rectangle, Stamp, Signature, Attachment, Line, Arrow, Ellipse, TextCallout, Note,
    Save, Print, CloudUpload, Upload,
    Thumbnails, Bookmark, CommentBubble, Fields, More, Up, Down, Fit, ZoomPercent,
    ToolEdit, ToolConvert, ToolOcr, ToolComment, ToolTranslate, ToolCombine, ToolCompress, ToolBatch,
    Menu, Avatar, DarkMode,
    None
};

class IconSystem {
public:
    static void DrawIcon(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect, IconId id, ID2D1SolidColorBrush* brush, float stroke = 1.5f);
    static void ClearCache();
};

} // namespace controls
