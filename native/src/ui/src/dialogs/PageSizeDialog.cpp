#include "ui/dialogs/PageSizeDialog.h"
#include <iomanip>
#include <sstream>
#include <windowsx.h>

namespace ui::dialogs {

static std::wstring FormatFloat(float val) {
    std::wostringstream ss;
    ss << std::fixed << std::setprecision(2) << val;
    std::wstring s = ss.str();
    if (s.find(L'.') != std::wstring::npos) {
        while (s.back() == L'0') s.pop_back();
        if (s.back() == L'.') s.pop_back();
    }
    return s;
}

bool PageSizeDialog::Show(HWND parentHwnd, PageSizeParams& params) {
    PageSizeDialog dlg(parentHwnd, params);
    return dlg.DoModal();
}

PageSizeDialog::PageSizeDialog(HWND parent, PageSizeParams& params)
    : ModernDialog(parent, L"Page Size", 450, 360), m_params(params) {}
PageSizeDialog::~PageSizeDialog() {}

LRESULT CALLBACK PageSizeDialog::EditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto pThis = reinterpret_cast<PageSizeDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (msg == WM_CHAR && wp == VK_RETURN) return 0;
    return CallWindowProc(pThis->m_oldEditProc, hwnd, msg, wp, lp);
}

void PageSizeDialog::OnCreate() {
    auto hInst = GetModuleHandleW(nullptr);
    DWORD es = WS_CHILD | WS_VISIBLE | ES_CENTER;
    m_editW = CreateWindowExW(0, L"EDIT", FormatFloat(m_params.width).c_str(), es, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    m_editH = CreateWindowExW(0, L"EDIT", FormatFloat(m_params.height).c_str(), es, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    SetPlaceholder(m_editW, L"Width");
    SetPlaceholder(m_editH, L"Height");
    HFONT hFont = GetEditFont();
    SendMessage(m_editW, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_editH, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    m_oldEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(m_editW, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc)));
    SetWindowLongPtr(m_editH, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc));
    SetWindowLongPtr(m_editW, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetWindowLongPtr(m_editH, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

void PageSizeDialog::OnLayout(float w, float h) {
    m_rectApply = {w - 220, h - 45, w - 120, h - 15};
    m_rectCancel = {w - 110, h - 45, w - 20, h - 15};
    
    m_rectSize0 = {40, 70, 160, 94};
    m_rectSize1 = {180, 70, 300, 94};
    m_rectSize2 = {40, 106, 160, 130};
    
    m_rectPortrait = {40, 180, 160, 204};
    m_rectLandscape = {180, 180, 300, 204};
    
    if (m_editW) SetChildPos(m_editW, 220, 108, 60, 22);
    if (m_editH) SetChildPos(m_editH, 320, 108, 60, 22);
}

void PageSizeDialog::OnRender() {
    m_target->DrawTextW(L"Standard Sizes", 14, m_formatTitle.Get(), {30, 45, 200, 65}, m_textBrush.Get());
    DrawRadio(m_rectSize0, L"Letter", 6, m_params.pageSizeIndex == 0);
    DrawRadio(m_rectSize1, L"A4", 2, m_params.pageSizeIndex == 1);
    DrawRadio(m_rectSize2, L"Custom:", 7, m_params.pageSizeIndex == 2);
    
    m_target->DrawTextW(L"W:", 2, m_formatLabel.Get(), {190, 110, 210, 130}, m_textBrush.Get());
    DrawEditBorder({216, 104, 284, 134});
    m_target->DrawTextW(L"H:", 2, m_formatLabel.Get(), {290, 110, 310, 130}, m_textBrush.Get());
    DrawEditBorder({316, 104, 384, 134});
    
    m_target->DrawTextW(L"Orientation", 11, m_formatTitle.Get(), {30, 155, 200, 175}, m_textBrush.Get());
    DrawRadio(m_rectPortrait, L"Portrait", 8, m_params.isPortrait);
    DrawRadio(m_rectLandscape, L"Landscape", 9, !m_params.isPortrait);
    
    DrawPrimaryButton(m_rectApply, L"Apply", m_hoverButton == 1, m_downButton == 1);
    DrawSecondaryButton(m_rectCancel, L"Cancel", m_hoverButton == 0, m_downButton == 0);
}

void PageSizeDialog::SyncToEdits() {
    SetWindowTextW(m_editW, FormatFloat(m_params.width).c_str());
    SetWindowTextW(m_editH, FormatFloat(m_params.height).c_str());
}

void PageSizeDialog::OnMouseMove(float x, float y) {
    int hb = -1;
    if (PtInR(x, y, m_rectCancel)) hb = 0;
    else if (PtInR(x, y, m_rectApply)) hb = 1;
    else if (PtInR(x, y, m_rectSize0) || PtInR(x, y, m_rectSize1) || PtInR(x, y, m_rectSize2) || 
             PtInR(x, y, m_rectPortrait) || PtInR(x, y, m_rectLandscape)) {
        SetCursor(LoadCursor(nullptr, IDC_HAND));
    }
    
    if (hb != m_hoverButton) {
        m_hoverButton = hb;
        Invalidate();
    }
}

void PageSizeDialog::OnMouseDown(float x, float y) {
    m_downButton = m_hoverButton;
    if (m_downButton != -1) Invalidate();
    
    if (PtInR(x, y, m_rectSize0)) { 
        m_params.pageSizeIndex = 0; 
        m_params.width = m_params.isPortrait ? 612.0f : 792.0f;
        m_params.height = m_params.isPortrait ? 792.0f : 612.0f;
        SyncToEdits(); Invalidate();
    } else if (PtInR(x, y, m_rectSize1)) { 
        m_params.pageSizeIndex = 1; 
        m_params.width = m_params.isPortrait ? 595.0f : 842.0f;
        m_params.height = m_params.isPortrait ? 842.0f : 595.0f;
        SyncToEdits(); Invalidate();
    } else if (PtInR(x, y, m_rectSize2)) { 
        m_params.pageSizeIndex = 2; Invalidate();
    }
    
    if (PtInR(x, y, m_rectPortrait) && !m_params.isPortrait) {
        m_params.isPortrait = true;
        float t = m_params.width; m_params.width = m_params.height; m_params.height = t;
        SyncToEdits(); Invalidate();
    } else if (PtInR(x, y, m_rectLandscape) && m_params.isPortrait) {
        m_params.isPortrait = false;
        float t = m_params.width; m_params.width = m_params.height; m_params.height = t;
        SyncToEdits(); Invalidate();
    }
    
    SetFocus(m_hwnd);
}

void PageSizeDialog::OnMouseUp(float x, float y) {
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
                GetWindowTextW(m_editW, buf, 256); m_params.width = (float)_wtof(buf);
                GetWindowTextW(m_editH, buf, 256); m_params.height = (float)_wtof(buf);
                m_resultOk = true;
                m_running = false;
            }
        }
    }
}

} // namespace ui::dialogs

