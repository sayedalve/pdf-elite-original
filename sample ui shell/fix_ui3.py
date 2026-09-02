import re

# 1. Update Theme.cpp to pure white text
with open('Theme.cpp', 'r', encoding='utf-8') as f:
    theme_code = f.read()

theme_code = re.sub(
    r'hr = rt->CreateSolidColorBrush\(FromHex\(Colors::TEXT_PRIMARY\), &brushTextPrimary\);',
    r'hr = rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &brushTextPrimary);',
    theme_code
)
theme_code = re.sub(
    r'hr = rt->CreateSolidColorBrush\(FromHex\(Colors::TEXT_SECONDARY\), &brushTextSecondary\);',
    r'hr = rt->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.85f, 0.85f, 1.0f), &brushTextSecondary);',
    theme_code
)
theme_code = re.sub(
    r'hr = rt->CreateSolidColorBrush\(FromHex\(Colors::TEXT_TERTIARY\), &brushTextTertiary\);',
    r'hr = rt->CreateSolidColorBrush(D2D1::ColorF(0.70f, 0.70f, 0.70f, 1.0f), &brushTextTertiary);',
    theme_code
)
with open('Theme.cpp', 'w', encoding='utf-8') as f:
    f.write(theme_code)


# 2. Update MainWindow.cpp
with open('MainWindow.cpp', 'r', encoding='utf-8') as f:
    mw_code = f.read()

# Add dwmapi include and DWMWA_USE_IMMERSIVE_DARK_MODE
if '<dwmapi.h>' not in mw_code:
    mw_code = '#include <dwmapi.h>\n#pragma comment(lib, "dwmapi.lib")\n' + mw_code

init_pattern = r'RegisterClassExW\(&wcex\);\s*if \(!m_d2dFactory\)'
init_new = '''RegisterClassExW(&wcex);
    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(m_hwnd, 20, &useDarkMode, sizeof(useDarkMode)); // DWMWA_USE_IMMERSIVE_DARK_MODE
    if (!m_d2dFactory)'''
mw_code = re.sub(init_pattern, init_new, mw_code)

# Fix Tab spacing
tab_pattern = r'float tx = topBar\.left \+ 80;'
tab_new = r'float tx = topBar.left + 8;'
mw_code = re.sub(tab_pattern, tab_new, mw_code)

# Fix Left Rail layout
leftrail_pattern = r'// LeftRail with icons\s*rt->FillRectangle\(leftRail.*?(?=// RightRail \(icons on right\))'
leftrail_new = '''        // LeftRail with icons
        rt->FillRectangle(leftRail, m_theme.brushSidebarBg);
        if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(leftRail.right, leftRail.top), D2D1::Point2F(leftRail.right, leftRail.bottom), m_theme.brushBorder, 1.0f);
        const wchar_t* railLabels[] = {L"Home", L"Comment", L"Edit", L"Convert", L"View", L"Organize", L"Tools", L"Form"};
        IconId railIcons[] = {IconId::Home, IconId::Comment, IconId::Edit, IconId::Convert, IconId::View, IconId::Organize, IconId::Tools, IconId::Form};
        
        float lr_width = leftRail.right - leftRail.left;
        auto drawLeftRailItem = [&](int idx, float y, bool active) {
            D2D1_RECT_F itemRect = D2D1::RectF(leftRail.left+4, y, leftRail.right-4, y+68);
            auto brush = active ? m_theme.brushAccent : (idx==0 ? m_theme.brushTextPrimary : m_theme.brushTextTertiary);
            if (active && m_theme.brushSurface && m_theme.brushAccent) {
                D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(itemRect, 8,8);
                rt->FillRoundedRectangle(rr, m_theme.brushSurface);
            }
            if (m_d2dFactory) {
                float iconSize = 24.0f;
                float ix = leftRail.left + (lr_width - iconSize) / 2.0f;
                D2D1_RECT_F iconRect = D2D1::RectF(ix, itemRect.top+12, ix+iconSize, itemRect.top+12+iconSize);
                IconSystem::DrawIcon(rt, iconRect, railIcons[idx], brush, 1.5f);
            }
            if (m_theme.fmtSmall) {
                float labelW = wcslen(railLabels[idx])*6.0f;
                float tx = leftRail.left + (lr_width - labelW) / 2.0f;
                D2D1_RECT_F labelRect = D2D1::RectF(tx, itemRect.top+42, leftRail.right, itemRect.bottom);
                rt->DrawText(railLabels[idx], (UINT32)wcslen(railLabels[idx]), m_theme.fmtSmall, labelRect, brush);
            }
        };

        drawLeftRailItem(0, client.top + 8, false);
        if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(leftRail.left+12, topBar.bottom), D2D1::Point2F(leftRail.right-12, topBar.bottom), m_theme.brushBorder, 1.0f);

        float ly = topBar.bottom + 16;
        for (int i=1;i<8;++i) { // Skip Home
            bool active = (i==1 && m_viewerMode==ViewerMode::Comment) || (i==2 && m_viewerMode==ViewerMode::Edit) || (i==4 && m_viewerMode==ViewerMode::View) || (i==5 && m_viewerMode==ViewerMode::Organize);
            drawLeftRailItem(i, ly, active);
            ly += 76; // Uniform spacing
        }
        
'''
mw_code = re.sub(leftrail_pattern, leftrail_new, mw_code, flags=re.DOTALL)

# Fix Toolbar removing OCR and uniform spacing
toolbar_pattern = r'float x = toolbar\.left \+ 12;\s*auto drawToolItem = \[\&\]\(IconId icon.*?drawToolItem\(IconId::Ocr, L"OCR"\);'
toolbar_new = '''        float x = toolbar.left + 16;
        auto drawToolItem = [&](IconId icon, const wchar_t* label, bool active=false) {
            float labelW = wcslen(label)*7.0f;
            float iconW = (icon != IconId::None) ? 24.0f : 0.0f; // Bigger icons
            float w = labelW + iconW + (iconW>0?8:0) + 16;
            D2D1_RECT_F tr = D2D1::RectF(x, toolbar.top+8, x+w, toolbar.bottom-8);
            if (active && m_theme.brushElevated) {
                rt->FillRoundedRectangle(D2D1::RoundedRect(tr, 6,6), m_theme.brushElevated);
            }
            if (icon != IconId::None && m_d2dFactory) {
                IconSystem::DrawIcon(rt, D2D1::RectF(tr.left+8, tr.top+4, tr.left+32, tr.top+28), icon, active ? m_theme.brushTextPrimary : m_theme.brushTextSecondary, 1.5f);
            }
            if (m_theme.fmtBody && m_theme.brushTextSecondary) {
                rt->DrawText(label, (UINT32)wcslen(label), m_theme.fmtBody, D2D1::RectF(tr.left+8+iconW+(iconW>0?8:0), tr.top+6, tr.right, tr.bottom), active ? m_theme.brushTextPrimary : m_theme.brushTextSecondary);
            }
            x += w + 8; // Uniform distance
        };
        auto drawSep = [&]() {
            if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(x+4, toolbar.top+12), D2D1::Point2F(x+4, toolbar.bottom-12), m_theme.brushBorder, 1.0f);
            x += 16;
        };
        
        if (m_viewerMode == ViewerMode::View) {
            drawToolItem(IconId::Undo, L""); drawToolItem(IconId::Redo, L""); drawSep();
            drawToolItem(IconId::ZoomOut, L""); drawToolItem(IconId::ZoomIn, L""); drawSep();
            drawToolItem(IconId::Hand, L"Hand", true); drawToolItem(IconId::RectSelect, L"Select"); drawSep();
            drawToolItem(IconId::AddText, L"Add Text"); drawToolItem(IconId::Crop, L"Crop"); 
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
            drawToolItem(IconId::EditAll, L"Edit All", true); drawToolItem(IconId::AddText, L"Add Text");
        }'''
mw_code = re.sub(toolbar_pattern, toolbar_new, mw_code, flags=re.DOTALL)

# Fix Right Rail icons being bigger
rightrail_pattern = r'float ry = rightRail\.top \+ 12;\s*auto drawRIcon = \[\&\]\(IconId id, bool active=false\) \{.*?;'
rightrail_new = '''float ry = rightRail.top + 16;
            auto drawRIcon = [&](IconId id, bool active=false) {
                D2D1_RECT_F ir = D2D1::RectF(rightRail.left+12, ry, rightRail.left+36, ry+24);
                if (active && m_theme.brushSurface) rt->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(rightRail.left+4, ry-4, rightRail.right-4, ry+28), 4,4), m_theme.brushSurface);
                IconSystem::DrawIcon(rt, D2D1::RectF(rightRail.left+10, ry-2, rightRail.left+38, ry+26), id, active ? m_theme.brushAccent : m_theme.brushTextSecondary, 1.5f);
                ry+=40;
            };'''
mw_code = re.sub(rightrail_pattern, rightrail_new, mw_code, flags=re.DOTALL)

with open('MainWindow.cpp', 'w', encoding='utf-8') as f:
    f.write(mw_code)
