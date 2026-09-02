#include "IconSystem.h"
// AppShell.cpp - Drastically improved to match Wondershare target exactly
#include "AppShell.h"
#include <d2d1helper.h>

namespace PdfElite {

HRESULT AppShell::Initialize(ID2D1RenderTarget* rt, IDWriteFactory* dwFactory, Theme* theme, CommandManager* cmd) {
    m_theme = theme; m_cmd = cmd; return S_OK;
}
void AppShell::Release() {}

void AppShell::Render(ID2D1RenderTarget* rt, const D2D1_RECT_F& topBar, const D2D1_RECT_F& toolbar, const D2D1_RECT_F& leftRail, const D2D1_RECT_F& rightRail, const D2D1_RECT_F& status, ViewerMode mode, const std::vector<TabInfo>& tabs, int curPage, int totalPages, float zoom) {
    m_mode = mode;
    RenderTopBar(rt, topBar, tabs);
    RenderToolbar(rt, toolbar);
    RenderLeftRail(rt, leftRail);
    RenderRightRail(rt, rightRail, curPage, totalPages, zoom);
    RenderStatus(rt, status, curPage, totalPages, zoom);
}

void AppShell::RenderTopBar(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect, const std::vector<TabInfo>& tabs) {
    // Exact target: #1c1c2e with bottom border
    rt->FillRectangle(rect, m_theme->brushTopBar);
    if (m_theme->brushBorder) rt->DrawLine(D2D1::Point2F(rect.left, rect.bottom), D2D1::Point2F(rect.right, rect.bottom), m_theme->brushBorder, 1.0f);

    // Hamburger + Home icon (left)
    D2D1_RECT_F ham = D2D1::RectF(rect.left+12, rect.top+12, rect.left+36, rect.bottom-12);
    IconSystem::DrawIcon(rt, ham, IconId::Menu, m_theme->brushTextSecondary);
    D2D1_RECT_F homeIcon = D2D1::RectF(rect.left+40, rect.top+12, rect.left+64, rect.bottom-12);
    IconSystem::DrawIcon(rt, homeIcon, IconId::Home, m_theme->brushTextSecondary);

    // Tabs - exact from image: rounded 8px bg #28283e, active, X close, + new
    float x = rect.left + 72;
    for (size_t i=0;i<tabs.size() && i<2;++i) {
        bool active = tabs[i].active;
        D2D1_RECT_F tabRect = D2D1::RectF(x, rect.top+8, x+260, rect.bottom-8);
        D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(tabRect, 8,8);
        auto bg = active ? m_theme->brushSurface : m_theme->brushElevated;
        rt->FillRoundedRectangle(rr, bg);
        if (m_theme->brushBorder) rt->DrawRoundedRectangle(rr, m_theme->brushBorder, 1.0f);
        D2D1_RECT_F textRect = D2D1::RectF(tabRect.left+12, tabRect.top+4, tabRect.right-28, tabRect.bottom-4);
        rt->DrawText(tabs[i].title.c_str(), (UINT32)min(tabs[i].title.length(), (size_t)28), m_theme->fmtBody, textRect, m_theme->brushTextPrimary);
        // dirty * and X
        D2D1_RECT_F closeRect = D2D1::RectF(tabRect.right-24, tabRect.top+6, tabRect.right-8, tabRect.bottom-6);
        IconSystem::DrawIcon(rt, closeRect, IconId::Close, m_theme->brushTextTertiary);
        x += 268;
    }
    // + new tab
    D2D1_RECT_F plusRect = D2D1::RectF(x+8, rect.top+12, x+32, rect.bottom-12);
    IconSystem::DrawIcon(rt, plusRect, IconId::Plus, m_theme->brushTextSecondary);

    // Right: avatar blue circle, bell, gear, min max close - exact target
    float rx = rect.right - 32;
    IconSystem::DrawIcon(rt, D2D1::RectF(rx, rect.top+12, rx+24, rect.bottom-12), IconId::Undo, m_theme->brushTextSecondary);
    rx -= 36;
    IconSystem::DrawIcon(rt, D2D1::RectF(rx, rect.top+12, rx+24, rect.bottom-12), IconId::Undo, m_theme->brushTextSecondary);
    rx -= 36;
    IconSystem::DrawIcon(rt, D2D1::RectF(rx, rect.top+12, rx+24, rect.bottom-12), IconId::Undo, m_theme->brushTextSecondary);
    rx -= 48;
    IconSystem::DrawIcon(rt, D2D1::RectF(rx, rect.top+12, rx+24, rect.bottom-12), IconId::Undo, m_theme->brushTextSecondary);
    rx -= 36;
    IconSystem::DrawIcon(rt, D2D1::RectF(rx, rect.top+12, rx+24, rect.bottom-12), IconId::Undo, m_theme->brushTextSecondary);
    rx -= 40;
    // Blue avatar
    D2D1_ELLIPSE avatar = D2D1::Ellipse(D2D1::Point2F(rx+12, rect.top+24), 14,14);
    rt->FillEllipse(avatar, m_theme->brushAccent);
    D2D1_RECT_F avatarText = D2D1::RectF(rx, rect.top+12, rx+24, rect.bottom-12);
    IconSystem::DrawIcon(rt, avatarText, IconId::Avatar, m_theme->brushWhite);
}

void AppShell::RenderToolbar(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect) {
    rt->FillRectangle(rect, m_theme->brushToolbar);
    if (m_theme->brushBorder) rt->DrawLine(D2D1::Point2F(rect.left, rect.bottom), D2D1::Point2F(rect.right, rect.bottom), m_theme->brushBorder, 1.0f);

    switch(m_mode) {
        case ViewerMode::View: RenderViewToolbar(rt, rect); break;
        case ViewerMode::Comment: RenderCommentToolbar(rt, rect); break;
        case ViewerMode::Edit: RenderEditToolbar(rt, rect); break;
        case ViewerMode::Organize: RenderOrganizeToolbar(rt, rect); break;
        default: RenderViewToolbar(rt, rect); break;
    }

    // Right side: Search Tools, save, print, cloud, upload - exact target
    float rx = rect.right - 20;
    // Upload
    D2D1_RECT_F upload = D2D1::RectF(rx-28, rect.top+12, rx, rect.bottom-12);
    IconSystem::DrawIcon(rt, upload, IconId::Upload, m_theme->brushTextSecondary);
    rx -= 36;
    D2D1_RECT_F cloud = D2D1::RectF(rx-28, rect.top+12, rx, rect.bottom-12);
    IconSystem::DrawIcon(rt, cloud, IconId::Cloud, m_theme->brushTextSecondary);
    rx -= 36;
    D2D1_RECT_F print = D2D1::RectF(rx-28, rect.top+12, rx, rect.bottom-12);
    IconSystem::DrawIcon(rt, print, IconId::Print, m_theme->brushTextSecondary);
    rx -= 36;
    D2D1_RECT_F save = D2D1::RectF(rx-28, rect.top+12, rx, rect.bottom-12);
    IconSystem::DrawIcon(rt, save, IconId::Save, m_theme->brushTextSecondary);
    rx -= 48;
    // Search Tools pill
    D2D1_ROUNDED_RECT searchRR = D2D1::RoundedRect(D2D1::RectF(rx-140, rect.top+8, rx-8, rect.bottom-8), 8,8);
    rt->FillRoundedRectangle(searchRR, m_theme->brushSurface);
    if (m_theme->brushBorder) rt->DrawRoundedRectangle(searchRR, m_theme->brushBorder, 1.0f);
    D2D1_RECT_F searchText = D2D1::RectF(rx-132, rect.top+12, rx-16, rect.bottom-12);
    rt->DrawText(L"Search Tools", 12, m_theme->fmtSmall, searchText, m_theme->brushTextTertiary);
}

void AppShell::DrawToolbarItem(ID2D1RenderTarget* rt, float& x, float y, float h, const wchar_t* label, bool active, bool hasDropdown) {
    float w = 0;
    if (wcscmp(label, L"|")==0) {
        if (m_theme->brushBorder) rt->DrawLine(D2D1::Point2F(x+8, y+8), D2D1::Point2F(x+8, y+h-8), m_theme->brushBorder, 1.0f);
        x+=24; return;
    }
    w = (wcslen(label)*7 + 24);
    if (hasDropdown) w+=12;
    if (w<32) w=32;
    D2D1_RECT_F tr = D2D1::RectF(x, y+6, x+w, y+h-6);
    if (active) {
        D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(tr, 6,6);
        rt->FillRoundedRectangle(rr, m_theme->brushElevated);
    }
    auto brush = active ? m_theme->brushTextPrimary : m_theme->brushTextSecondary;
    rt->DrawText(label, (UINT32)wcslen(label), m_theme->fmtBody, D2D1::RectF(tr.left+8, tr.top+2, tr.right-8, tr.bottom-2), brush);
    if (hasDropdown) {
        IconSystem::DrawIcon(rt, D2D1::RectF(tr.right-14, tr.top+4, tr.right-2, tr.bottom-2), IconId::Down, brush);
    }
    x+=w+4;
}

void AppShell::RenderViewToolbar(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect) {
    float x = rect.left + 12;
    float y = rect.top;
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"↩", false);
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"↪", false);
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"|");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"🔍−");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"🔍+");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"|");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"✋");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"▭");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"|");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"Edit All", true, true);
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"Add Text");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"OCR");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"Crop");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"Combine");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"Compress");
}

void AppShell::RenderCommentToolbar(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect) {
    float x = rect.left + 12;
    float y = rect.top;
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"↩");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"↪");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"|");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"🖍 Highlight");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"▭ Highlight");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"✎");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"⌫");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"|");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"U", false, true);
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"T");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"T Box");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"☐", false, true);
}

void AppShell::RenderEditToolbar(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect) {
    float x = rect.left + 12;
    float y = rect.top;
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"↩");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"↪");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"|");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"Edit All", true, true);
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"T Add Text");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"🔗 Add Link");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"🖼 Image", false, true);
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"|");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"Watermark", false, true);
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"Background", false, true);
}

void AppShell::RenderOrganizeToolbar(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect) {
    float x = rect.left + 12;
    float y = rect.top;
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"↩");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"↪");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"|");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"🔍−");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"🔍+");
    D2D1_ROUNDED_RECT pageRR = D2D1::RoundedRect(D2D1::RectF(x, y+10, x+80, y+38), 6,6);
    rt->FillRoundedRectangle(pageRR, m_theme->brushSurface);
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"1", false, true);
    x+=20;
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"↻");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"↺");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"🗑");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"|");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"Extract");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"Split", false, true);
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"Insert", false, true);
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"|");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"Crop");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"Rotate");
    DrawToolbarItem(rt, x, y, rect.bottom-rect.top, L"Size");
}

void AppShell::RenderLeftRail(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect) {
    rt->FillRectangle(rect, m_theme->brushSidebarBg);
    if (m_theme->brushBorder) rt->DrawLine(D2D1::Point2F(rect.right, rect.top), D2D1::Point2F(rect.right, rect.bottom), m_theme->brushBorder, 1.0f);

    struct Item { PdfElite::IconId id; const wchar_t* label; };
    Item items[] = {
        {IconId::Home, L"Home"}, {IconId::Comment, L"Comment"}, {IconId::Edit, L"Edit"}, {IconId::Convert, L"Convert"}, {IconId::View, L"View"}, {IconId::Organize, L"Organize"}, {IconId::Tools, L"Tools"}, {IconId::Form, L"Form"}
    };
    float y = rect.top + 8;
    for (int i=0;i<8;++i) {
        bool active = false;
        if (i==1 && m_mode==ViewerMode::Comment) active=true;
        if (i==2 && m_mode==ViewerMode::Edit) active=true;
        if (i==4 && m_mode==ViewerMode::View) active=true;
        if (i==5 && m_mode==ViewerMode::Organize) active=true;

        D2D1_RECT_F itemRect = D2D1::RectF(rect.left+6, y, rect.right-6, y+56);
        if (active) {
            D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(itemRect, 10,10);
            rt->FillRoundedRectangle(rr, m_theme->brushSurface);
            if (m_theme->brushAccent) rt->DrawRoundedRectangle(rr, m_theme->brushAccent, 1.2f);
        }
        // Icon
        D2D1_RECT_F iconRect = D2D1::RectF(itemRect.left, itemRect.top+6, itemRect.right, itemRect.top+26);
        auto brush = active ? m_theme->brushAccent : m_theme->brushTextTertiary;
        IconSystem::DrawIcon(rt, D2D1::RectF(iconRect.left+22, iconRect.top+2, iconRect.right-22, iconRect.bottom-2), items[i].id, brush);
        // Label
        D2D1_RECT_F labelRect = D2D1::RectF(itemRect.left, itemRect.top+28, itemRect.right, itemRect.bottom);
        rt->DrawText(items[i].label, (UINT32)wcslen(items[i].label), m_theme->fmtSmall, labelRect, active ? m_theme->brushTextPrimary : m_theme->brushTextTertiary);
        y+=60;
    }
}

void AppShell::RenderRightRail(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect, int curPage, int totalPages, float zoom) {
    rt->FillRectangle(rect, m_theme->brushSidebarBg);
    if (m_theme->brushBorder) rt->DrawLine(D2D1::Point2F(rect.left, rect.top), D2D1::Point2F(rect.left, rect.bottom), m_theme->brushBorder, 1.0f);

    float y = rect.top + 12;
    auto drawIcon = [&](IconId id, bool active=false) {
        D2D1_RECT_F ir = D2D1::RectF(rect.left+6, y, rect.right-6, y+32);
        if (active) {
            D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(ir, 8,8);
            rt->FillRoundedRectangle(rr, m_theme->brushSurface);
        }
        IconSystem::DrawIcon(rt, D2D1::RectF(ir.left+4, ir.top+4, ir.right-4, ir.bottom-4), id, active ? m_theme->brushTextPrimary : m_theme->brushTextTertiary);
        y+=36;
    };
    drawIcon(IconId::Bookmark);
    drawIcon(IconId::Bookmark);
    drawIcon(IconId::Bookmark);
    drawIcon(IconId::Bookmark);
    drawIcon(IconId::Bookmark);
    y+=12;
    // Page indicator
    D2D1_ROUNDED_RECT pageRR = D2D1::RoundedRect(D2D1::RectF(rect.left+6, y, rect.right-6, y+28), 8,8);
    rt->FillRoundedRectangle(pageRR, m_theme->brushSurface);
    std::wstring pg = std::to_wstring(curPage);
    D2D1_RECT_F pgRect = D2D1::RectF(rect.left, y, rect.right, y+28);
    rt->DrawText(pg.c_str(), (UINT32)pg.length(), m_theme->fmtSmall, pgRect, m_theme->brushTextPrimary);
    y+=36;
    D2D1_RECT_F totalRect = D2D1::RectF(rect.left, y, rect.right, y+20);
    std::wstring tot = std::to_wstring(totalPages);
    rt->DrawText(tot.c_str(), (UINT32)tot.length(), m_theme->fmtSmall, totalRect, m_theme->brushTextTertiary);
    y+=32;
    drawIcon(IconId::Bookmark);
    drawIcon(IconId::Bookmark);
    drawIcon(IconId::Bookmark);
    drawIcon(IconId::Bookmark);
    // Zoom
    D2D1_RECT_F zoomRect = D2D1::RectF(rect.left, y, rect.right, y+20);
    std::wstring zm = std::to_wstring(int(zoom*100)) + L"%";
    rt->DrawText(zm.c_str(), (UINT32)zm.length(), m_theme->fmtSmall, zoomRect, m_theme->brushTextTertiary);
    y+=24;
    drawIcon(IconId::Bookmark);
    drawIcon(IconId::Bookmark);
}

void AppShell::RenderStatus(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect, int curPage, int totalPages, float zoom) {
    rt->FillRectangle(rect, m_theme->brushSidebarBg);
    if (m_theme->fmtSmall) {
        std::wstring status = std::to_wstring(curPage) + L" / " + std::to_wstring(totalPages) + L"  " + std::to_wstring(int(zoom*100)) + L"%";
        rt->DrawText(status.c_str(), (UINT32)status.length(), m_theme->fmtSmall, D2D1::RectF(rect.left+12, rect.top+4, rect.right-12, rect.bottom), m_theme->brushTextTertiary);
    }
}

bool AppShell::HitTest(int x, int y, CommandId& out) {
    if (x>=0 && x<64 && y>48) {
        float ry = y-48-8;
        int idx = int(ry/60);
        if (idx==1){ out=CommandId::ModeComment; return true; }
        if (idx==2){ out=CommandId::ModeEdit; return true; }
        if (idx==4){ out=CommandId::ModeView; return true; }
        if (idx==5){ out=CommandId::ModeOrganize; return true; }
    }
    return false;
}

} // namespace PdfElite
