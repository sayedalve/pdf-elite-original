#include "ui/dialogs/MessageDialog.h"

namespace ui {
namespace dialogs {

MessageDialogResult MessageDialog::Show(HWND parent, const std::wstring& title, const std::wstring& message, MessageDialogType type) {
    MessageDialog dlg(parent, title, message, type);
    dlg.DoModal();
    return dlg.m_dialogResult;
}

MessageDialog::MessageDialog(HWND parent, const std::wstring& title, const std::wstring& message, MessageDialogType type)
    : ModernDialog(parent, title, 400, 180), m_title(title), m_message(message), m_type(type) {
}

void MessageDialog::OnLayout(float w, float h) {
    float btnW = 100.0f;
    float btnH = 35.0f;
    float padding = 20.0f;
    float gap = 10.0f;
    
    if (m_type == MessageDialogType::Ok) {
        m_rectBtn1 = {w - padding - btnW, h - padding - btnH, w - padding, h - padding};
    } else if (m_type == MessageDialogType::YesNo) {
        m_rectBtn1 = {w - padding - btnW, h - padding - btnH, w - padding, h - padding}; // No
        m_rectBtn2 = {m_rectBtn1.left - gap - btnW, m_rectBtn1.top, m_rectBtn1.left - gap, m_rectBtn1.bottom}; // Yes
    } else if (m_type == MessageDialogType::YesNoCancel) {
        m_rectBtn1 = {w - padding - btnW, h - padding - btnH, w - padding, h - padding}; // Cancel
        m_rectBtn2 = {m_rectBtn1.left - gap - btnW, m_rectBtn1.top, m_rectBtn1.left - gap, m_rectBtn1.bottom}; // No
        m_rectBtn3 = {m_rectBtn2.left - gap - btnW, m_rectBtn2.top, m_rectBtn2.left - gap, m_rectBtn2.bottom}; // Yes
    }
}

void MessageDialog::OnRender() {
    float w = static_cast<float>(m_width);
    float h = static_cast<float>(m_height);
    
    m_target->DrawTextW(m_message.c_str(), static_cast<UINT32>(m_message.length()), m_formatLabel.Get(), {20, 45, w - 20, h - 70}, m_textBrush.Get());
    
    if (m_type == MessageDialogType::Ok) {
        DrawPrimaryButton(m_rectBtn1, L"OK", m_hoverButton == 1, m_downButton == 1);
    } else if (m_type == MessageDialogType::YesNo) {
        DrawPrimaryButton(m_rectBtn2, L"Yes", m_hoverButton == 2, m_downButton == 2);
        DrawSecondaryButton(m_rectBtn1, L"No", m_hoverButton == 1, m_downButton == 1);
    } else if (m_type == MessageDialogType::YesNoCancel) {
        DrawPrimaryButton(m_rectBtn3, L"Yes", m_hoverButton == 3, m_downButton == 3);
        DrawSecondaryButton(m_rectBtn2, L"No", m_hoverButton == 2, m_downButton == 2);
        DrawSecondaryButton(m_rectBtn1, L"Cancel", m_hoverButton == 1, m_downButton == 1);
    }
}

void MessageDialog::OnMouseMove(float x, float y) {
    int hb = -1;
    if (m_type == MessageDialogType::Ok) {
        if (PtInR(x, y, m_rectBtn1)) hb = 1;
    } else if (m_type == MessageDialogType::YesNo) {
        if (PtInR(x, y, m_rectBtn1)) hb = 1;
        else if (PtInR(x, y, m_rectBtn2)) hb = 2;
    } else if (m_type == MessageDialogType::YesNoCancel) {
        if (PtInR(x, y, m_rectBtn1)) hb = 1;
        else if (PtInR(x, y, m_rectBtn2)) hb = 2;
        else if (PtInR(x, y, m_rectBtn3)) hb = 3;
    }
    
    if (hb != m_hoverButton) {
        m_hoverButton = hb;
        Invalidate();
    }
}

void MessageDialog::OnMouseDown(float x, float y) {
    (void)x; (void)y;
    m_downButton = m_hoverButton;
    if (m_downButton != -1) Invalidate();
}

void MessageDialog::OnMouseUp(float x, float y) {
    (void)x; (void)y;
    int db = m_downButton;
    m_downButton = -1;
    Invalidate();
    
    if (db != -1 && db == m_hoverButton) {
        if (m_type == MessageDialogType::Ok) {
            m_dialogResult = MessageDialogResult::Ok;
        } else if (m_type == MessageDialogType::YesNo) {
            if (db == 2) m_dialogResult = MessageDialogResult::Yes;
            else m_dialogResult = MessageDialogResult::No;
        } else if (m_type == MessageDialogType::YesNoCancel) {
            if (db == 3) m_dialogResult = MessageDialogResult::Yes;
            else if (db == 2) m_dialogResult = MessageDialogResult::No;
            else m_dialogResult = MessageDialogResult::Cancel;
        }
        m_running = false;
    }
}


GoToPageDialog::GoToPageDialog(HWND parent, int maxPages)
    : ModernDialog(parent, L"Go To Page", 300, 160), m_maxPages(maxPages) {
}

GoToPageDialog::~GoToPageDialog() {
}

void GoToPageDialog::OnCreate() {
    m_hEdit = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_AUTOHSCROLL,
        0, 0, 0, 0, m_hwnd, (HMENU)101, GetModuleHandle(nullptr), nullptr);
    SendMessage(m_hEdit, WM_SETFONT, (WPARAM)GetEditFont(), TRUE);
}

void GoToPageDialog::OnLayout(float w, float h) {
    m_rectEdit = {20.0f, 60.0f, w - 20.0f, 85.0f};
    SetChildPos(m_hEdit, 20.0f, 60.0f, w - 40.0f, 25.0f);
    
    m_rectBtnOk = {w - 180.0f, h - 44.0f, w - 100.0f, h - 12.0f};
    m_rectBtnCancel = {w - 90.0f, h - 44.0f, w - 10.0f, h - 12.0f};
}

void GoToPageDialog::OnRender() {
    RenderBase();
    if (m_formatLabel && m_textBrush) {
        D2D1_RECT_F labelRect = {20.0f, 40.0f, 280.0f, 60.0f};
        std::wstring label = L"Enter page number (1 - " + std::to_wstring(m_maxPages) + L"):";
        m_target->DrawTextW(label.c_str(), (UINT32)label.length(), m_formatLabel.Get(), labelRect, m_textBrush.Get());
    }
    DrawPrimaryButton(m_rectBtnOk, L"Go", m_btnOkHover, m_btnOkDown);
    DrawSecondaryButton(m_rectBtnCancel, L"Cancel", m_btnCancelHover, m_btnCancelDown);
}

void GoToPageDialog::OnMouseMove(float x, float y) {
    bool okH = PtInR(x, y, m_rectBtnOk);
    bool cH = PtInR(x, y, m_rectBtnCancel);
    if (okH != m_btnOkHover || cH != m_btnCancelHover) {
        m_btnOkHover = okH;
        m_btnCancelHover = cH;
        Invalidate();
    }
}

void GoToPageDialog::OnMouseDown(float x, float y) {
    if (PtInR(x, y, m_rectBtnOk)) m_btnOkDown = true;
    if (PtInR(x, y, m_rectBtnCancel)) m_btnCancelDown = true;
    if (m_btnOkDown || m_btnCancelDown) Invalidate();
}

void GoToPageDialog::OnMouseUp(float x, float y) {
    if (m_btnOkDown && PtInR(x, y, m_rectBtnOk)) {
        wchar_t buf[256] = {0};
        GetWindowTextW(m_hEdit, buf, 256);
        m_page = _wtoi(buf) - 1;
        if (m_page >= 0 && m_page < m_maxPages) {
            m_resultOk = true;
            m_running = false;
        } else {
            MessageBoxW(m_hwnd, L"Invalid page number.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
    if (m_btnCancelDown && PtInR(x, y, m_rectBtnCancel)) {
        m_running = false;
    }
    m_btnOkDown = false;
    m_btnCancelDown = false;
    Invalidate();
}

} // namespace dialogs
} // namespace ui
