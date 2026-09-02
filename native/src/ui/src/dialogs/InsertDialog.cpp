#include "ui/dialogs/InsertDialog.h"
#include <string>

namespace ui::dialogs {

bool InsertDialog::Show(HWND parentHwnd, InsertParams& params) {
    InsertDialog dlg(parentHwnd, params);
    return dlg.DoModal();
}

InsertDialog::InsertDialog(HWND parent, InsertParams& params)
    : ModernDialog(parent, L"Insert Blank Page", 420, 480), m_params(params) {}

InsertDialog::~InsertDialog() {}

LRESULT CALLBACK InsertDialog::EditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto pThis = reinterpret_cast<InsertDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (msg == WM_CHAR && wp == VK_RETURN) return 0;
    return CallWindowProc(pThis->m_oldEditProc, hwnd, msg, wp, lp);
}

void InsertDialog::OnCreate() {
    auto hInst = GetModuleHandleW(nullptr);
    DWORD es = WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER;
    m_editPage = CreateWindowExW(0, L"EDIT", std::to_wstring(m_params.pageNum).c_str(), es, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    m_editCopies = CreateWindowExW(0, L"EDIT", std::to_wstring(m_params.copies).c_str(), es | ES_NUMBER | ES_CENTER, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    SetPlaceholder(m_editPage, L"Page #");
    SetPlaceholder(m_editCopies, L"1");
    HFONT hFont = GetEditFont();
    SendMessage(m_editPage, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_editCopies, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    m_oldEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(m_editPage, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc)));
    SetWindowLongPtr(m_editCopies, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc));
    SetWindowLongPtr(m_editPage, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetWindowLongPtr(m_editCopies, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

void InsertDialog::OnLayout(float w, float h) {
    m_rectCancel = {w - 110, h - 50, w - 20, h - 20};
    m_rectInsert = {w - 210, h - 50, w - 120, h - 20};
    
    m_rectLocFirst = {30, 80, 200, 104};
    m_rectLocLast = {30, 115, 200, 139};
    m_rectLocPage = {30, 150, 110, 174};
    
    m_rectAfter = {30, 230, 250, 254};
    m_rectBefore = {30, 265, 250, 289};
    
    if (m_editPage) SetChildPos(m_editPage, 120, 149, 70, 22);
    if (m_editCopies) SetChildPos(m_editCopies, 30, 362, 100, 22);
}

void InsertDialog::OnRender() {
    m_target->DrawTextW(L"Place at", 8, m_formatHeader.Get(), {30, 50, 200, 70}, m_textBrush.Get());
    DrawRadio(m_rectLocFirst, L"First page", 10, m_params.placeAt == 0);
    DrawRadio(m_rectLocLast, L"Last page", 9, m_params.placeAt == 1);
    DrawRadio(m_rectLocPage, L"Page", 4, m_params.placeAt == 2);
    
    DrawEditBorder({116, 145, 194, 175});
    std::wstring totalStr = L" / " + std::to_wstring(m_params.maxPages);
    m_target->DrawTextW(totalStr.c_str(), (UINT32)totalStr.length(), m_formatLabel.Get(), {205, 150, 280, 170}, m_textBrush.Get());
    
    m_target->DrawTextW(L"Location", 8, m_formatHeader.Get(), {30, 200, 200, 220}, m_textBrush.Get());
    DrawRadio(m_rectAfter, L"Insert after the page", 21, m_params.location == 0);
    DrawRadio(m_rectBefore, L"Insert before the page", 22, m_params.location == 1);
    
    m_target->DrawTextW(L"Copies", 6, m_formatHeader.Get(), {30, 330, 200, 350}, m_textBrush.Get());
    DrawEditBorder({26, 358, 134, 388});
    
    DrawPrimaryButton(m_rectInsert, L"OK", m_hoverButton == 1, m_downButton == 1);
    DrawSecondaryButton(m_rectCancel, L"Cancel", m_hoverButton == 0, m_downButton == 0);
}

void InsertDialog::OnMouseMove(float x, float y) {
    int hb = -1;
    if (PtInR(x, y, m_rectCancel)) hb = 0;
    else if (PtInR(x, y, m_rectInsert)) hb = 1;
    else if (PtInR(x, y, m_rectLocFirst) || PtInR(x, y, m_rectLocLast) || PtInR(x, y, m_rectLocPage) ||
             PtInR(x, y, m_rectAfter) || PtInR(x, y, m_rectBefore)) {
        SetCursor(LoadCursor(nullptr, IDC_HAND));
    }
    
    if (hb != m_hoverButton) {
        m_hoverButton = hb;
        Invalidate();
    }
}

void InsertDialog::OnMouseDown(float x, float y) {
    m_downButton = m_hoverButton;
    if (m_downButton != -1) Invalidate();
    
    if (PtInR(x, y, m_rectLocFirst)) { m_params.placeAt = 0; Invalidate(); }
    else if (PtInR(x, y, m_rectLocLast)) { m_params.placeAt = 1; Invalidate(); }
    else if (PtInR(x, y, m_rectLocPage)) { m_params.placeAt = 2; Invalidate(); }
    else if (PtInR(x, y, m_rectAfter)) { m_params.location = 0; Invalidate(); }
    else if (PtInR(x, y, m_rectBefore)) { m_params.location = 1; Invalidate(); }
    
    SetFocus(m_hwnd);
}

void InsertDialog::OnMouseUp(float x, float y) {
    (void)x; (void)y;
    if (m_downButton != -1) {
        int db = m_downButton;
        m_downButton = -1;
        Invalidate();
        
        if (db == m_hoverButton) {
            if (db == 0) {
                m_resultOk = false;
                m_running = false;
            } else if (db == 1) {
                wchar_t buf[256];
                GetWindowTextW(m_editPage, buf, 256);
                m_params.pageNum = _wtoi(buf);
                GetWindowTextW(m_editCopies, buf, 256);
                m_params.copies = _wtoi(buf);
                
                if (m_params.pageNum < 1) m_params.pageNum = 1;
                if (m_params.pageNum > m_params.maxPages) m_params.pageNum = m_params.maxPages;
                if (m_params.copies < 1) m_params.copies = 1;
                if (m_params.copies > 100) m_params.copies = 100;
                
                m_resultOk = true;
                m_running = false;
            }
        }
    }
}

} // namespace ui::dialogs

