import re

with open('MainWindow.cpp', 'r', encoding='utf-8') as f:
    code = f.read()

# 1. Inject DwmSetWindowAttribute
init_pattern = r'm_hwnd=CreateWindowW.*?;\s*if\(!m_hwnd\) return E_FAIL;'
init_new = '''m_hwnd=CreateWindowW(L"PDFEliteMain", L"PDF Elite", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1440, 900, nullptr, nullptr, hInst, this);
    if(!m_hwnd) return E_FAIL;
    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(m_hwnd, 20, &useDarkMode, sizeof(useDarkMode));
    DwmSetWindowAttribute(m_hwnd, 19, &useDarkMode, sizeof(useDarkMode));'''
code = re.sub(init_pattern, init_new, code, flags=re.DOTALL)

# 2. Fix Right Rail Width from 48 to 68
code = code.replace(
    'D2D1_RECT_F rightRail=D2D1::RectF(client.right-48, topBar.bottom, client.right, client.bottom);',
    'D2D1_RECT_F rightRail=D2D1::RectF(client.right-68, topBar.bottom, client.right, client.bottom);'
)

# 3. Fix Toolbar spacing and icon rendering
toolbar_pattern = r'float x = toolbar\.left \+ 16;\s*auto drawToolItem = \[\&\]\(IconId icon, const wchar_t\* label, bool active=false\) \{.*?;'
toolbar_new = '''float x = toolbar.left + 16;
        auto drawToolItem = [&](IconId icon, const wchar_t* label, bool active=false) {
            float labelW = wcslen(label) > 0 ? (wcslen(label)*6.0f + 8.0f) : 0.0f;
            float iconW = (icon != IconId::None) ? 24.0f : 0.0f;
            float w = labelW + iconW + 16.0f;
            D2D1_RECT_F tr = D2D1::RectF(x, toolbar.top+8, x+w, toolbar.bottom-8);
            if (active && m_theme.brushElevated) {
                rt->FillRoundedRectangle(D2D1::RoundedRect(tr, 6,6), m_theme.brushElevated);
            }
            if (icon != IconId::None && m_d2dFactory) {
                IconSystem::DrawIcon(rt, D2D1::RectF(tr.left+8, tr.top+4, tr.left+32, tr.top+28), icon, active ? m_theme.brushTextPrimary : m_theme.brushWhite, 1.5f);
            }
            if (labelW > 0 && m_theme.fmtBody && m_theme.brushTextSecondary) {
                rt->DrawText(label, (UINT32)wcslen(label), m_theme.fmtBody, D2D1::RectF(tr.left+8+iconW+4, tr.top+6, tr.right, tr.bottom), active ? m_theme.brushTextPrimary : m_theme.brushTextSecondary);
            }
            x += w + 4; // Tighter, uniform spacing
        };
        auto drawSep = [&]() {
            x += 4;
            if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(x+2, toolbar.top+12), D2D1::Point2F(x+2, toolbar.bottom-12), m_theme.brushBorder, 1.0f);
            x += 12;
        };'''
code = re.sub(toolbar_pattern, toolbar_new, code, flags=re.DOTALL)

# 4. Fix Right Rail icons and spacing
# First let's find the current right rail lambda
rr_pattern = r'float ry = rightRail\.top \+ 16;\s*auto drawRIcon = \[\&\]\(IconId id, bool active=false\) \{.*?drawRIcon\(IconId::ZoomOut\);\s*\}'
rr_new = '''float ry = rightRail.top + 16;
            auto drawRIcon = [&](IconId id, bool active=false) {
                D2D1_RECT_F ir = D2D1::RectF(rightRail.left+14, ry, rightRail.left+54, ry+40);
                if (active && m_theme.brushSurface) rt->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(rightRail.left+10, ry-4, rightRail.right-10, ry+44), 6,6), m_theme.brushSurface);
                IconSystem::DrawIcon(rt, D2D1::RectF(rightRail.left+20, ry+6, rightRail.left+48, ry+34), id, active ? m_theme.brushAccent : m_theme.brushWhite, 1.8f);
                ry+=56; // Much larger uniform spacing
            };
            drawRIcon(IconId::Thumbnails, true);
            drawRIcon(IconId::Bookmark);
            drawRIcon(IconId::CommentBubble);
            drawRIcon(IconId::Fields);
            drawRIcon(IconId::More);
            
            // Separator
            ry += 4;
            if (m_theme.brushBorder) rt->DrawLine(D2D1::Point2F(rightRail.left+16, ry), D2D1::Point2F(rightRail.right-16, ry), m_theme.brushBorder, 1.0f);
            ry += 20;

            // Bottom navigation icons
            auto drawRLabel = [&](const wchar_t* lbl) {
                if (m_theme.fmtSmall) {
                    float wl = wcslen(lbl)*6.0f;
                    rt->DrawText(lbl, (UINT32)wcslen(lbl), m_theme.fmtSmall, D2D1::RectF(rightRail.left + (68-wl)/2, ry, rightRail.right, ry+20), m_theme.brushTextSecondary);
                }
                ry+=32; // Larger text spacing
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
code = re.sub(rr_pattern, rr_new, code, flags=re.DOTALL)


with open('MainWindow.cpp', 'w', encoding='utf-8') as f:
    f.write(code)

