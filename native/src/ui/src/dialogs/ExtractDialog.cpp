#include "ui/dialogs/ExtractDialog.h"
#include "ui/dialogs/MessageDialog.h"
#include "../NativeDesignSystem.h"
#include <commdlg.h>

namespace ui::dialogs {

bool ExtractDialog::Show(HWND parentHwnd, ExtractParams& params) {
    ExtractDialog dlg(parentHwnd, params);
    return dlg.DoModal();
}

ExtractDialog::ExtractDialog(HWND parent, ExtractParams& params)
    : ModernDialog(parent, L"Extract Pages", 440, 360), m_params(params) {}

ExtractDialog::~ExtractDialog() {}

LRESULT CALLBACK ExtractDialog::EditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto pThis = reinterpret_cast<ExtractDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (msg == WM_CHAR && wp == VK_RETURN) return 0;
    return CallWindowProc(pThis->m_oldEditProc, hwnd, msg, wp, lp);
}

void ExtractDialog::OnCreate() {
    auto hInst = GetModuleHandleW(nullptr);
    DWORD es = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
    m_editRange = CreateWindowExW(0, L"EDIT", m_params.pageRange.c_str(), es, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    m_editPath = CreateWindowExW(0, L"EDIT", m_params.outputPath.c_str(), es, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    SetPlaceholder(m_editRange, L"1, 3, 5-7");
    SetPlaceholder(m_editPath, L"Output filename.pdf");
    HFONT hFont = GetEditFont();
    SendMessage(m_editRange, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_editPath, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    m_oldEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(m_editRange, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc)));
    SetWindowLongPtr(m_editPath, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc));
    SetWindowLongPtr(m_editRange, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetWindowLongPtr(m_editPath, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

void ExtractDialog::OnLayout(float w, float h) {
    m_rectExtract = {w - 220, h - 50, w - 120, h - 20};
    m_rectCancel = {w - 110, h - 50, w - 20, h - 20};
    m_rectBrowse = {w - 120, h - 114, w - 30, h - 84};
    
    m_rectAll = {30, 80, 180, 104};
    m_rectPages = {30, 120, 100, 144};
    m_rectDelete = {30, 160, 300, 184};
    
    SetChildPos(m_editRange, 110, 119, w - 264, 22);
    SetChildPos(m_editPath, 34, h - 109, w - 194, 22);
}

void ExtractDialog::OnRender() {
    float h = static_cast<float>(m_height);
    float w = static_cast<float>(m_width);

    m_target->DrawTextW(L"Select pages to extract into a new PDF document.", 48, m_formatLabel.Get(), {30, 50, w - 30, 70}, m_textBrush.Get());
    
    DrawRadio(m_rectAll, L"All Pages", 9, m_params.extractAll);
    DrawRadio(m_rectPages, L"Pages:", 6, !m_params.extractAll);
    DrawCheckbox(m_rectDelete, L"Delete pages after extracting", 29, m_params.deleteAfterExtract);
    
    DrawEditBorder({106.0f, 115.0f, w - 146.0f, 145.0f});
    
    D2D1_RECT_F pathLabel = {30, h - 140, 300, h - 120};
    m_target->DrawTextW(L"Save As", 7, m_formatHeader.Get(), pathLabel, m_textBrush.Get());
    DrawEditBorder({30, h - 113, w - 152, h - 83});
    
    DrawPrimaryButton(m_rectExtract, L"OK", m_hoverButton == 1, m_downButton == 1);
    DrawSecondaryButton(m_rectCancel, L"Cancel", m_hoverButton == 0, m_downButton == 0);
    DrawSecondaryButton(m_rectBrowse, L"Browse...", m_hoverButton == 2, m_downButton == 2);
}

void ExtractDialog::OnMouseMove(float x, float y) {
    int hb = -1;
    if (PtInR(x, y, m_rectCancel)) hb = 0;
    else if (PtInR(x, y, m_rectExtract)) hb = 1;
    else if (PtInR(x, y, m_rectBrowse)) hb = 2;
    else if (PtInR(x, y, m_rectAll) || PtInR(x, y, m_rectPages) || PtInR(x, y, m_rectDelete)) {
        SetCursor(LoadCursor(nullptr, IDC_HAND));
    }
    
    if (hb != m_hoverButton) {
        m_hoverButton = hb;
        Invalidate();
    }
}

void ExtractDialog::OnMouseDown(float x, float y) {
    (void)x; (void)y;
    m_downButton = m_hoverButton;
    if (m_downButton != -1) Invalidate();
    
    if (PtInR(x, y, m_rectAll)) { 
        m_params.extractAll = true; 
        Invalidate(); 
    } else if (PtInR(x, y, m_rectPages)) { 
        m_params.extractAll = false; 
        Invalidate(); 
    } else if (PtInR(x, y, m_rectDelete)) {
        m_params.deleteAfterExtract = !m_params.deleteAfterExtract;
        Invalidate();
    }
    
    SetFocus(m_hwnd);
}

void ExtractDialog::OnMouseUp(float x, float y) {
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
                wchar_t buf[1024];
                GetWindowTextW(m_editRange, buf, 1024);
                m_params.pageRange = buf;
                GetWindowTextW(m_editPath, buf, 1024);
                m_params.outputPath = buf;
                
                if (!m_params.extractAll && m_params.pageRange.empty()) {
                    MessageDialog::Show(m_hwnd, L"Missing Input", L"Please enter a valid page range.");
                    return;
                }
                
                if (m_params.outputPath.empty()) {
                    // Trigger Browse
                    OPENFILENAMEW ofn = {};
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = m_hwnd;
                    ofn.lpstrFilter = L"PDF Files (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0";
                    wchar_t szFile[MAX_PATH] = L"Extracted.pdf";
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
                    ofn.lpstrDefExt = L"pdf";
                    if (GetSaveFileNameW(&ofn)) {
                        SetWindowTextW(m_editPath, szFile);
                        m_params.outputPath = szFile;
                    } else {
                        return; // User cancelled
                    }
                }
                
                m_resultOk = true;
                m_running = false;
            } else if (db == 2) {
                OPENFILENAMEW ofn = {};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = m_hwnd;
                ofn.lpstrFilter = L"PDF Files (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0";
                wchar_t szFile[MAX_PATH] = L"Extracted.pdf";
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
                ofn.lpstrDefExt = L"pdf";
                if (GetSaveFileNameW(&ofn)) {
                    SetWindowTextW(m_editPath, szFile);
                }
            }
        }
    }
}

} // namespace ui::dialogs

