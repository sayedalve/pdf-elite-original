import re

with open('MainWindow.cpp', 'r', encoding='utf-8') as f:
    code = f.read()

# 1. Update Layout Rectangles in RenderViewer
# Find the start of RenderViewer
viewer_start = code.find('void MainWindow::RenderViewer')
if viewer_start != -1:
    layout_old = '''    D2D1_RECT_F topBar=D2D1::RectF(client.left, client.top, client.right, client.top+48);
    D2D1_RECT_F toolbar=D2D1::RectF(client.left+64, topBar.bottom, client.right-48, topBar.bottom+48);
    D2D1_RECT_F leftRail=D2D1::RectF(client.left, topBar.bottom, client.left+64, client.bottom-24);
    D2D1_RECT_F rightRail=D2D1::RectF(client.right-48, topBar.bottom, client.right, client.bottom-24);
    D2D1_RECT_F center=D2D1::RectF(leftRail.right, toolbar.bottom, rightRail.left, client.bottom-24);
    D2D1_RECT_F status=D2D1::RectF(client.left, client.bottom-24, client.right, client.bottom);'''
    
    layout_new = '''    D2D1_RECT_F leftRail=D2D1::RectF(client.left, client.top, client.left+68, client.bottom);
    D2D1_RECT_F topBar=D2D1::RectF(leftRail.right, client.top, client.right, client.top+48);
    D2D1_RECT_F rightRail=D2D1::RectF(client.right-48, topBar.bottom, client.right, client.bottom);
    D2D1_RECT_F toolbar=D2D1::RectF(leftRail.right, topBar.bottom, rightRail.left, topBar.bottom+48);
    D2D1_RECT_F status=D2D1::RectF(leftRail.right, client.bottom-24, rightRail.left, client.bottom);
    D2D1_RECT_F center=D2D1::RectF(leftRail.right, toolbar.bottom, rightRail.left, status.top);'''
    
    code = code.replace(layout_old, layout_new)

# 2. Fix the Toolbar lambda in RenderViewer
tool_lambda_old = '''        float x = toolbar.left + 16;
        auto drawToolItem = [&](const wchar_t* label, bool active=false) {
            float w = 0;
            if (m_dwFactory && m_theme.fmtBody) {
                Microsoft::WRL::ComPtr<IDWriteTextLayout> tl;
                m_dwFactory->CreateTextLayout(label, (UINT32)wcslen(label), m_theme.fmtBody, 200, 32, &tl);
                if (tl) { DWRITE_TEXT_METRICS tm; tl->GetMetrics(&tm); w = tm.width; }
            }
            if (w==0) w = wcslen(label)*8;
            D2D1_RECT_F tr = D2D1::RectF(x, toolbar.top+8, x+w+16, toolbar.bottom-8);
            if (active && m_theme.brushSurface) {
                rt->FillRoundedRectangle(D2D1::RoundedRect(tr, 4,4), m_theme.brushSurface);
            }
            if (m_theme.fmtBody) {
                rt->DrawText(label, (UINT32)wcslen(label), m_theme.fmtBody, D2D1::RectF(tr.left+8, tr.top+6, tr.right, tr.bottom), active ? m_theme.brushTextPrimary : m_theme.brushTextSecondary);
            }
            x += w + 4;
        };
        auto drawSep = [&]() {
            if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(x+8, toolbar.top+12), D2D1::Point2F(x+8, toolbar.bottom-12), m_theme.brushBorder, 1.0f);
            x += 20;
        };
        drawToolItem(L"Undo"); drawToolItem(L"Redo"); drawSep();
        drawToolItem(L"Zoom-"); drawToolItem(L"Zoom+"); drawSep();
        drawToolItem(L"Hand"); drawToolItem(L"Rect"); drawSep();
        drawToolItem(L"Edit All", true); drawToolItem(L"Add Text"); drawToolItem(L"OCR"); drawToolItem(L"Crop"); drawToolItem(L"Combine"); drawToolItem(L"Compress");'''

tool_lambda_new = '''        float x = toolbar.left + 12;
        auto drawToolItem = [&](IconId icon, const wchar_t* label, bool active=false) {
            float w = 0;
            if (m_dwFactory && m_theme.fmtBody) {
                Microsoft::WRL::ComPtr<IDWriteTextLayout> tl;
                m_dwFactory->CreateTextLayout(label, (UINT32)wcslen(label), m_theme.fmtBody, 200, 32, &tl);
                if (tl) { DWRITE_TEXT_METRICS tm; tl->GetMetrics(&tm); w = tm.width; }
            }
            if (w==0) w = wcslen(label)*8;
            float iconW = (icon != IconId::None) ? 20.0f : 0.0f;
            D2D1_RECT_F tr = D2D1::RectF(x, toolbar.top+8, x+w+iconW+24, toolbar.bottom-8);
            if (active && m_theme.brushSurface) {
                rt->FillRoundedRectangle(D2D1::RoundedRect(tr, 4,4), m_theme.brushSurface);
            }
            if (icon != IconId::None && m_d2dFactory) {
                D2D1_RECT_F ir = D2D1::RectF(tr.left+8, tr.top+6, tr.left+28, tr.top+26);
                IconSystem::DrawIcon(rt, ir, icon, active ? m_theme.brushTextPrimary : m_theme.brushTextSecondary, 1.5f);
            }
            if (m_theme.fmtBody) {
                rt->DrawText(label, (UINT32)wcslen(label), m_theme.fmtBody, D2D1::RectF(tr.left+8+iconW+(iconW>0?4:0), tr.top+6, tr.right, tr.bottom), active ? m_theme.brushTextPrimary : m_theme.brushTextSecondary);
            }
            x += w + iconW + 28;
        };
        auto drawSep = [&]() {
            if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(x+2, toolbar.top+12), D2D1::Point2F(x+2, toolbar.bottom-12), m_theme.brushBorder, 1.0f);
            x += 12;
        };
        
        if (m_viewerMode == ViewerMode::View) {
            drawToolItem(IconId::Undo, L""); drawToolItem(IconId::Redo, L""); drawSep();
            drawToolItem(IconId::ZoomOut, L""); drawToolItem(IconId::ZoomIn, L""); drawSep();
            drawToolItem(IconId::Hand, L"Hand", true); drawToolItem(IconId::RectSelect, L"Select"); drawSep();
            drawToolItem(IconId::AddText, L"Add Text"); drawToolItem(IconId::Ocr, L"OCR"); drawToolItem(IconId::Crop, L"Crop"); 
            drawToolItem(IconId::Combine, L"Combine"); drawToolItem(IconId::Compress, L"Compress");
        } else if (m_viewerMode == ViewerMode::Comment) {
            drawToolItem(IconId::Undo, L""); drawToolItem(IconId::Redo, L""); drawSep();
            drawToolItem(IconId::Highlight, L"Highlight"); drawToolItem(IconId::Underline, L"Underline"); drawToolItem(IconId::Strikethrough, L"Strikethrough"); drawSep();
            drawToolItem(IconId::Pencil, L"Pencil"); drawToolItem(IconId::Eraser, L"Eraser"); drawSep();
            drawToolItem(IconId::Text, L"Text"); drawToolItem(IconId::TextBox, L"Text Box"); drawToolItem(IconId::Rectangle, L"Rectangle");
        } else if (m_viewerMode == ViewerMode::Edit) {
            drawToolItem(IconId::Undo, L""); drawToolItem(IconId::Redo, L""); drawSep();
            drawToolItem(IconId::EditAll, L"Edit All", true); drawToolItem(IconId::AddText, L"Add Text"); drawToolItem(IconId::AddLink, L"Add Link"); drawSep();
            drawToolItem(IconId::Image, L"Image"); drawToolItem(IconId::Watermark, L"Watermark"); drawToolItem(IconId::Background, L"Background");
        } else {
            drawToolItem(IconId::Undo, L""); drawToolItem(IconId::Redo, L""); drawSep();
            drawToolItem(IconId::ZoomOut, L""); drawToolItem(IconId::ZoomIn, L""); drawSep();
            drawToolItem(IconId::Hand, L"Hand"); drawToolItem(IconId::RectSelect, L"Select"); drawSep();
            drawToolItem(IconId::EditAll, L"Edit All", true); drawToolItem(IconId::AddText, L"Add Text"); drawToolItem(IconId::Ocr, L"OCR");
        }'''

code = code.replace(tool_lambda_old, tool_lambda_new)

# 3. Update LeftRail rendering in RenderViewer
leftrail_old = '''        // LeftRail with icons
        rt->FillRectangle(leftRail, m_theme.brushSidebarBg);
        if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(leftRail.right, leftRail.top), D2D1::Point2F(leftRail.right, leftRail.bottom), m_theme.brushBorder, 1.0f);
        const wchar_t* railLabels[] = {L"Home", L"Comment", L"Edit", L"Convert", L"View", L"Organize", L"Tools", L"Form"};
        IconId railIcons[] = {IconId::Home, IconId::Comment, IconId::Edit, IconId::Convert, IconId::View, IconId::Organize, IconId::Tools, IconId::Form};
        float y = leftRail.top + 8;
        for (int i=0;i<8;++i) {
            bool active = (i==1 && m_viewerMode==ViewerMode::Comment) || (i==2 && m_viewerMode==ViewerMode::Edit) || (i==4 && m_viewerMode==ViewerMode::View) || (i==5 && m_viewerMode==ViewerMode::Organize);
            D2D1_RECT_F itemRect = D2D1::RectF(leftRail.left+6, y, leftRail.right-6, y+56);
            if (active && m_theme.brushSurface && m_theme.brushAccent) {
                D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(itemRect, 10,10);
                rt->FillRoundedRectangle(rr, m_theme.brushSurface);
                rt->DrawRoundedRectangle(rr, m_theme.brushAccent, 1.2f);
            }
            D2D1_RECT_F iconRect = D2D1::RectF(itemRect.left+12, itemRect.top+8, itemRect.left+32, itemRect.top+28);
            if (m_d2dFactory) {
                auto brush = active ? m_theme.brushAccent : m_theme.brushTextTertiary;
                if (brush) IconSystem::DrawIcon(rt, iconRect, railIcons[i], brush, 1.2f);
            }
            if (m_theme.fmtSmall) {
                D2D1_RECT_F labelRect = D2D1::RectF(itemRect.left, itemRect.top+30, itemRect.right, itemRect.bottom);
                rt->DrawText(railLabels[i], (UINT32)wcslen(railLabels[i]), m_theme.fmtSmall, labelRect, active ? m_theme.brushTextPrimary : m_theme.brushTextTertiary);
            }
            y+=60;
        }'''

leftrail_new = '''        // LeftRail with icons
        rt->FillRectangle(leftRail, m_theme.brushSidebarBg);
        if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(leftRail.right, leftRail.top), D2D1::Point2F(leftRail.right, leftRail.bottom), m_theme.brushBorder, 1.0f);
        const wchar_t* railLabels[] = {L"Home", L"Comment", L"Edit", L"Convert", L"View", L"Organize", L"Tools", L"Form"};
        IconId railIcons[] = {IconId::Home, IconId::Comment, IconId::Edit, IconId::Convert, IconId::View, IconId::Organize, IconId::Tools, IconId::Form};
        
        // Render Home button at the very top (in line with topBar)
        if (m_d2dFactory) {
            D2D1_RECT_F homeRect = D2D1::RectF(leftRail.left+4, client.top+4, leftRail.right-4, client.top+44);
            IconSystem::DrawIcon(rt, D2D1::RectF(homeRect.left+16, homeRect.top+4, homeRect.left+36, homeRect.top+24), IconId::Home, m_theme.brushAccent, 1.5f);
            if (m_theme.fmtSmall) rt->DrawText(L"Home", 4, m_theme.fmtSmall, D2D1::RectF(homeRect.left, homeRect.top+26, homeRect.right, homeRect.bottom), m_theme.brushAccent);
            if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(leftRail.left+12, topBar.bottom), D2D1::Point2F(leftRail.right-12, topBar.bottom), m_theme.brushBorder, 1.0f);
        }

        float y = topBar.bottom + 16;
        for (int i=1;i<8;++i) { // Skip Home
            bool active = (i==1 && m_viewerMode==ViewerMode::Comment) || (i==2 && m_viewerMode==ViewerMode::Edit) || (i==4 && m_viewerMode==ViewerMode::View) || (i==5 && m_viewerMode==ViewerMode::Organize);
            D2D1_RECT_F itemRect = D2D1::RectF(leftRail.left+4, y, leftRail.right-4, y+60);
            if (active && m_theme.brushSurface && m_theme.brushAccent) {
                D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(itemRect, 8,8);
                rt->FillRoundedRectangle(rr, m_theme.brushSurface);
            }
            D2D1_RECT_F iconRect = D2D1::RectF(itemRect.left+20, itemRect.top+8, itemRect.left+40, itemRect.top+28);
            if (m_d2dFactory) {
                auto brush = active ? m_theme.brushAccent : m_theme.brushTextTertiary;
                if (brush) IconSystem::DrawIcon(rt, iconRect, railIcons[i], brush, 1.5f);
            }
            if (m_theme.fmtSmall) {
                // center text (DrawText centers if we use IDWriteTextFormat with DWRITE_TEXT_ALIGNMENT_CENTER, but we assume default is left, so offset x manually)
                D2D1_RECT_F labelRect = D2D1::RectF(itemRect.left, itemRect.top+34, itemRect.right, itemRect.bottom);
                float labelW = wcslen(railLabels[i])*6.0f; // approx width
                labelRect.left = itemRect.left + (60 - labelW)/2 - 2; 
                if (labelRect.left < itemRect.left) labelRect.left = itemRect.left;
                rt->DrawText(railLabels[i], (UINT32)wcslen(railLabels[i]), m_theme.fmtSmall, labelRect, active ? m_theme.brushTextPrimary : m_theme.brushTextTertiary);
            }
            y+=68;
        }'''

code = code.replace(leftrail_old, leftrail_new)

# 4. Update RightRail rendering in RenderViewer
rightrail_old = '''        // RightRail
        rt->FillRectangle(rightRail, m_theme.brushSidebarBg);
        if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(rightRail.left, rightRail.top), D2D1::Point2F(rightRail.left, rightRail.bottom), m_theme.brushBorder, 1.0f);
        if (m_theme.fmtSmall && m_theme.brushSurface) {
            float ry = rightRail.top + 12;
            D2D1_ROUNDED_RECT pageRR = D2D1::RoundedRect(D2D1::RectF(rightRail.left+6, ry, rightRail.right-6, ry+28), 8,8);
            rt->FillRoundedRectangle(pageRR, m_theme.brushSurface);
            std::wstring pg = std::to_wstring(m_curPage);
            D2D1_RECT_F pgRect = D2D1::RectF(rightRail.left, ry, rightRail.right, ry+28);
            rt->DrawText(pg.c_str(), (UINT32)pg.length(), m_theme.fmtSmall, pgRect, m_theme.brushTextPrimary);
        }'''

rightrail_new = '''        // RightRail (icons on right)
        rt->FillRectangle(rightRail, m_theme.brushSidebarBg);
        if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(rightRail.left, rightRail.top), D2D1::Point2F(rightRail.left, rightRail.bottom), m_theme.brushBorder, 1.0f);
        if (m_d2dFactory) {
            float ry = rightRail.top + 12;
            auto drawRIcon = [&](IconId id, bool active=false) {
                D2D1_RECT_F ir = D2D1::RectF(rightRail.left+12, ry, rightRail.left+36, ry+24);
                if (active && m_theme.brushSurface) rt->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(rightRail.left+6, ry-4, rightRail.right-6, ry+28), 4,4), m_theme.brushSurface);
                IconSystem::DrawIcon(rt, ir, id, active ? m_theme.brushAccent : m_theme.brushTextSecondary, 1.5f);
                ry+=36;
            };
            drawRIcon(IconId::Thumbnails, true);
            drawRIcon(IconId::Bookmark);
            drawRIcon(IconId::CommentBubble);
            drawRIcon(IconId::Fields);
            drawRIcon(IconId::More);
            
            // Bottom navigation icons
            ry = client.bottom - 240;
            auto drawRLabel = [&](const wchar_t* lbl) {
                if (m_theme.fmtSmall) {
                    float w = wcslen(lbl)*6.0f;
                    rt->DrawText(lbl, (UINT32)wcslen(lbl), m_theme.fmtSmall, D2D1::RectF(rightRail.left + (48-w)/2, ry, rightRail.right, ry+20), m_theme.brushTextSecondary);
                }
                ry+=24;
            };
            drawRIcon(IconId::Up);
            drawRLabel(L"1");
            drawRLabel(L"564");
            drawRIcon(IconId::Down);
            drawRIcon(IconId::Fit);
            drawRLabel(L"100%");
            drawRIcon(IconId::ZoomIn);
            drawRIcon(IconId::ZoomOut);
        }'''

code = code.replace(rightrail_old, rightrail_new)


# 5. Fix Recent Files Icon in RenderHomeMain
recent_icon_old = '''        if (m_theme.brushAccent) {
            D2D1_ROUNDED_RECT fileIcon=D2D1::RoundedRect(D2D1::RectF(row.left+8, row.top+12, row.left+32, row.top+36), 4,4);
            rt->FillRoundedRectangle(fileIcon, m_theme.brushAccent);
        }'''
        
recent_icon_new = '''        if (m_theme.brushAccent) {
            D2D1_ROUNDED_RECT fileIcon=D2D1::RoundedRect(D2D1::RectF(row.left+8, row.top+12, row.left+32, row.top+36), 4,4);
            rt->FillRoundedRectangle(fileIcon, m_theme.brushAccent);
            if (m_theme.brushWhite) {
                IconSystem::DrawIcon(rt, D2D1::RectF(row.left+12, row.top+16, row.left+28, row.top+32), IconId::CreateDoc, m_theme.brushWhite, 1.8f);
            }
        }'''

code = code.replace(recent_icon_old, recent_icon_new)

with open('MainWindow.cpp', 'w', encoding='utf-8') as f:
    f.write(code)

