#include "ui/dialogs/CropDialog.h"
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

bool CropDialog::Show(HWND parentHwnd, CropParams& params) {
    CropDialog dlg(parentHwnd, params);
    return dlg.DoModal();
}

CropDialog::CropDialog(HWND parent, CropParams& params)
    : ModernDialog(parent, L"Crop Pages", 400, 320), m_params(params) {}
CropDialog::~CropDialog() {}

LRESULT CALLBACK CropDialog::EditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto pThis = reinterpret_cast<CropDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (msg == WM_CHAR && wp == VK_RETURN) return 0;
    return CallWindowProc(pThis->m_oldEditProc, hwnd, msg, wp, lp);
}

void CropDialog::OnCreate() {
    auto hInst = GetModuleHandleW(nullptr);
    DWORD es = WS_CHILD | WS_VISIBLE | ES_CENTER;
    m_editT = CreateWindowExW(0, L"EDIT", FormatFloat(m_params.top).c_str(), es, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    m_editB = CreateWindowExW(0, L"EDIT", FormatFloat(m_params.bottom).c_str(), es, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    m_editL = CreateWindowExW(0, L"EDIT", FormatFloat(m_params.left).c_str(), es, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    m_editR = CreateWindowExW(0, L"EDIT", FormatFloat(m_params.right).c_str(), es, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    SetPlaceholder(m_editT, L"Top Margin");
    SetPlaceholder(m_editB, L"Bottom Margin");
    SetPlaceholder(m_editL, L"Left Margin");
    SetPlaceholder(m_editR, L"Right Margin");
    HFONT hFont = GetEditFont();
    SendMessage(m_editT, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_editB, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_editL, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_editR, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    m_oldEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(m_editT, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc)));
    SetWindowLongPtr(m_editB, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc));
    SetWindowLongPtr(m_editL, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc));
    SetWindowLongPtr(m_editR, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc));
    
    SetWindowLongPtr(m_editT, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetWindowLongPtr(m_editB, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetWindowLongPtr(m_editL, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetWindowLongPtr(m_editR, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

void CropDialog::OnLayout(float w, float h) {
    m_rectApply = {w - 220, h - 45, w - 120, h - 15};
    m_rectCancel = {w - 110, h - 45, w - 20, h - 15};
    
    if (m_editT) SetChildPos(m_editT, 170, 71, 60, 22);
    if (m_editB) SetChildPos(m_editB, 170, 191, 60, 22);
    if (m_editL) SetChildPos(m_editL, 80, 131, 60, 22);
    if (m_editR) SetChildPos(m_editR, 260, 131, 60, 22);
}

void CropDialog::OnRender() {
    m_formatLabel->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_target->DrawTextW(L"Top:", 4, m_formatLabel.Get(), {170, 50, 230, 70}, m_textBrush.Get());
    m_target->DrawTextW(L"Bottom:", 7, m_formatLabel.Get(), {170, 170, 230, 190}, m_textBrush.Get());
    m_target->DrawTextW(L"Left:", 5, m_formatLabel.Get(), {80, 110, 140, 130}, m_textBrush.Get());
    m_target->DrawTextW(L"Right:", 6, m_formatLabel.Get(), {260, 110, 320, 130}, m_textBrush.Get());
    m_formatLabel->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    
    DrawEditBorder({166, 67, 234, 97});
    DrawEditBorder({166, 187, 234, 217});
    DrawEditBorder({76, 127, 144, 157});
    DrawEditBorder({256, 127, 324, 157});
    
    DrawPrimaryButton(m_rectApply, L"Apply", m_hoverButton == 1, m_downButton == 1);
    DrawSecondaryButton(m_rectCancel, L"Cancel", m_hoverButton == 0, m_downButton == 0);
}

void CropDialog::OnMouseMove(float x, float y) {
    int hb = -1;
    if (PtInR(x, y, m_rectCancel)) hb = 0;
    else if (PtInR(x, y, m_rectApply)) hb = 1;
    
    if (hb != m_hoverButton) {
        m_hoverButton = hb;
        Invalidate();
    }
}

void CropDialog::OnMouseDown(float x, float y) {
    (void)x; (void)y;
    m_downButton = m_hoverButton;
    if (m_downButton != -1) Invalidate();
    SetFocus(m_hwnd);
}

void CropDialog::OnMouseUp(float x, float y) {
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
                GetWindowTextW(m_editT, buf, 256); m_params.top = (float)_wtof(buf);
                GetWindowTextW(m_editB, buf, 256); m_params.bottom = (float)_wtof(buf);
                GetWindowTextW(m_editL, buf, 256); m_params.left = (float)_wtof(buf);
                GetWindowTextW(m_editR, buf, 256); m_params.right = (float)_wtof(buf);
                m_resultOk = true;
                m_running = false;
            }
        }
    }
}

} // namespace ui::dialogs

