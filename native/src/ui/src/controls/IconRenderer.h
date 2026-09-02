#pragma once
#include <d2d1_1.h>
#include <wrl/client.h>
#include "../GraphicsDevice.h"
#include <algorithm>

using Microsoft::WRL::ComPtr;

#include "IconSystem.h"

namespace controls {

enum class IconType { Extract, Split, Delete, RotateCW, RotateCCW, Insert,
    None, Home, Recent, Star, Edit, Comment, Combine, Open, Menu, Settings, Save, Undo, Redo, Search, ZoomIn, ZoomOut, AddText, Highlight, Rectangle, Close, Maximize, Minimize, Convert, OCR, Translate, Compress, Batch, PDFDocument, Folder, More,
    View, Organize, Tools, Form,
    Link, Image, Watermark, Background,
    Print, Cloud, Share,
    Thumbnails, Bookmark, ArrowUp, ArrowDown,
    Chat, Fields, Fit, Avatar,
    Underline, Strikethrough, Copy, Squiggly, DarkMode,
    AreaHighlight, Pencil, Eraser, Note, Line, Arrow, Ellipse, TextBox, TextCallout
};

class IconRenderer {
public:
    static void DrawIcon(ComPtr<ID2D1RenderTarget> target, IconType type, const D2D1_RECT_F& bounds, D2D1_COLOR_F color) {
        ComPtr<ID2D1SolidColorBrush> brush;
        target->CreateSolidColorBrush(color, &brush);
        
        controls::IconId mapped = controls::IconId::None;
        switch (type) {
            case IconType::Home: mapped = controls::IconId::Home; break;
            case IconType::Recent: mapped = controls::IconId::RecentFiles; break;
            case IconType::Star: mapped = controls::IconId::StarredFiles; break;
            case IconType::Edit: mapped = controls::IconId::Edit; break;
            case IconType::Comment: mapped = controls::IconId::Comment; break;
            case IconType::Combine: mapped = controls::IconId::Combine; break;
            case IconType::Open: mapped = controls::IconId::OpenFolder; break;
            case IconType::Menu: mapped = controls::IconId::Menu; break;
            case IconType::Settings: mapped = controls::IconId::Tools; break;
            case IconType::Save: mapped = controls::IconId::Save; break;
            case IconType::Undo: mapped = controls::IconId::Undo; break;
            case IconType::Redo: mapped = controls::IconId::Redo; break;
            case IconType::Search: mapped = controls::IconId::Search; break;
            case IconType::ZoomIn: mapped = controls::IconId::ZoomIn; break;
            case IconType::ZoomOut: mapped = controls::IconId::ZoomOut; break;
            case IconType::AddText: mapped = controls::IconId::AddText; break;
            case IconType::Highlight: mapped = controls::IconId::Highlight; break;
            case IconType::Rectangle: mapped = controls::IconId::Rectangle; break;
            case IconType::Close: mapped = controls::IconId::Close; break;
            case IconType::Convert: mapped = controls::IconId::ToolConvert; break;
            case IconType::OCR: mapped = controls::IconId::ToolOcr; break;
            case IconType::Translate: mapped = controls::IconId::ToolTranslate; break;
            case IconType::Compress: mapped = controls::IconId::ToolCompress; break;
            case IconType::Batch: mapped = controls::IconId::ToolBatch; break;
            case IconType::PDFDocument: mapped = controls::IconId::Form; break;
            case IconType::Folder: mapped = controls::IconId::OpenFolder; break;
            case IconType::More: mapped = controls::IconId::More; break;
            case IconType::View: mapped = controls::IconId::View; break;
            case IconType::Organize: mapped = controls::IconId::Organize; break;
            case IconType::Tools: mapped = controls::IconId::Tools; break;
            case IconType::Form: mapped = controls::IconId::Form; break;
            case IconType::Link: mapped = controls::IconId::AddLink; break;
            case IconType::Image: mapped = controls::IconId::Image; break;
            case IconType::Watermark: mapped = controls::IconId::Watermark; break;
            case IconType::Background: mapped = controls::IconId::Background; break;
            case IconType::Print: mapped = controls::IconId::Print; break;
            case IconType::Cloud: mapped = controls::IconId::CloudUpload; break;
            case IconType::Share: mapped = controls::IconId::Upload; break; // Use upload for share
            case IconType::Thumbnails: mapped = controls::IconId::Thumbnails; break;
            case IconType::DarkMode: mapped = controls::IconId::DarkMode; break;
            case IconType::Bookmark: mapped = controls::IconId::Bookmark; break;
            case IconType::ArrowUp: mapped = controls::IconId::Up; break;
            case IconType::ArrowDown: mapped = controls::IconId::Down; break;
            case IconType::Chat: mapped = controls::IconId::CommentBubble; break;
            case IconType::Fields: mapped = controls::IconId::Fields; break;
            case IconType::Fit: mapped = controls::IconId::Fit; break;
            case IconType::Avatar: mapped = controls::IconId::Avatar; break;
            case IconType::Extract: mapped = controls::IconId::Extract; break; case IconType::Split: mapped = controls::IconId::Split; break; case IconType::Delete: mapped = controls::IconId::Trash; break; case IconType::RotateCW: mapped = controls::IconId::RotateRight; break; case IconType::RotateCCW: mapped = controls::IconId::RotateLeft; break; case IconType::Insert: mapped = controls::IconId::Insert; break; 
            case IconType::Underline: mapped = controls::IconId::Underline; break;
            case IconType::Strikethrough: mapped = controls::IconId::Strikethrough; break;
            case IconType::Copy: mapped = controls::IconId::EditAll; break; // Use EditAll as a proxy for Copy for now
            case IconType::Squiggly: mapped = controls::IconId::Underline; break; // proxy for squiggly
            case IconType::AreaHighlight: mapped = controls::IconId::HighlightArea; break;
            case IconType::Pencil: mapped = controls::IconId::Pencil; break;
            case IconType::Eraser: mapped = controls::IconId::Eraser; break;
            case IconType::Note: mapped = controls::IconId::Note; break;
            case IconType::Line: mapped = controls::IconId::Line; break;
            case IconType::Arrow: mapped = controls::IconId::Arrow; break;
            case IconType::Ellipse: mapped = controls::IconId::Ellipse; break;
            case IconType::TextBox: mapped = controls::IconId::TextBox; break;
            case IconType::TextCallout: mapped = controls::IconId::TextCallout; break;
            default: mapped = controls::IconId::None; break;
        }

        if (mapped != controls::IconId::None && brush) {
            controls::IconSystem::DrawIcon(target.Get(), bounds, mapped, brush.Get());
        }
    }
};
} // namespace controls

