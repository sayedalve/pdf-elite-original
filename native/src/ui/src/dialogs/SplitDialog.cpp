#include "ui/dialogs/SplitDialog.h"
#include "ui/dialogs/MessageDialog.h"
#include <commdlg.h>
#include <shlobj.h>
#include <windowsx.h>

namespace ui::dialogs {

bool SplitDialog::Show(HWND parentHwnd, SplitParams& params) {
    SplitDialog dlg(parentHwnd, params);
    return dlg.DoModal();
}

SplitDialog::SplitDialog(HWND parent, SplitParams& params)
    : ModernDialog(parent, L"Split", 500, 360), m_params(params) {}
SplitDialog::~SplitDialog() {}

LRESULT CALLBACK SplitDialog::EditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto pThis = reinterpret_cast<SplitDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (msg == WM_CHAR && wp == VK_RETURN) return 0;
    return CallWindowProc(pThis->m_oldEditProc, hwnd, msg, wp, lp);
}

void SplitDialog::OnCreate() {
    auto hInst = GetModuleHandleW(nullptr);
    DWORD es = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
    m_editValue = CreateWindowExW(0, L"EDIT", m_params.methodValue.c_str(), es | ES_CENTER | ES_NUMBER, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    m_editPath = CreateWindowExW(0, L"EDIT", m_params.outputFolder.c_str(), es, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    SetPlaceholder(m_editValue, L"1");
    SetPlaceholder(m_editPath, L"Output Folder");
    HFONT hFont = GetEditFont();
    SendMessage(m_editValue, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_editPath, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    m_oldEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(m_editValue, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc)));
    SetWindowLongPtr(m_editPath, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc));
    SetWindowLongPtr(m_editValue, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetWindowLongPtr(m_editPath, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

void SplitDialog::OnLayout(float w, float h) {
    m_rectSplit = {w - 240.0f, h - 50.0f, w - 130.0f, h - 15.0f};
    m_rectCancel = {w - 120.0f, h - 50.0f, w - 20.0f, h - 15.0f};
    m_rectBrowse = {w - 60.0f, h - 87.0f, w - 20.0f, h - 57.0f};
    
    m_rectMethod0 = {30, 75, 400, 95};
    m_rectMethod1 = {30, 110, 400, 130};
    m_rectMethod2 = {30, 150, 400, 170};
    
    if (m_editValue) {
        SetChildPos(m_editValue, 30, 97, 60, 22);
    }
    if (m_editPath) {
        SetChildPos(m_editPath, 30, h - 85, w - 130, 22);
    }
}

void SplitDialog::OnRender() {
    float h = static_cast<float>(m_height);
    float w = static_cast<float>(m_width);

    m_target->DrawTextW(L"Split Type", 10, m_formatHeader.Get(), {30, 42, 450, 60}, m_textBrush.Get());
    
    DrawRadio(m_rectMethod0, L"Split by number of pages", 24, m_params.splitMethod == 0);
    if (m_params.splitMethod == 0) {
        DrawEditBorder({26, 93, 94, 123});
        m_target->DrawTextW(L"Page / Per file", 15, m_formatLabel.Get(), {110, 97, 250, 115}, m_textBrush.Get());
    }
    
    float rm1y = m_rectMethod1.top + (m_params.splitMethod==0 ? 30.0f : 0.0f);
    DrawRadio({m_rectMethod1.left, rm1y, m_rectMethod1.right, rm1y+20}, L"Split by file size", 18, m_params.splitMethod == 1);
    if (m_params.splitMethod == 1) {
        DrawEditBorder({26, rm1y+24, 94, rm1y+54});
        m_target->DrawTextW(L"MB / Per file", 13, m_formatLabel.Get(), {110, rm1y+28, 250, rm1y+48}, m_textBrush.Get());
    }
    
    float m2y = m_rectMethod2.top + (m_params.splitMethod != 2 ? 30.0f : 0.0f);
    DrawRadio({30, m2y, 400, m2y+20}, L"Split by top level bookmarks", 28, m_params.splitMethod == 2);
    
    
    
    D2D1_RECT_F pathLabel = {30, h - 115, 300, h - 95};
    m_target->DrawTextW(L"Output Folder", 13, m_formatHeader.Get(), pathLabel, m_textBrush.Get());
    
    DrawEditBorder({26, h - 89, w - 96, h - 59});
    
    DrawPrimaryButton(m_rectSplit, L"OK", m_hoverButton == 1, m_downButton == 1);
    DrawSecondaryButton(m_rectCancel, L"Cancel", m_hoverButton == 0, m_downButton == 0);
    DrawSecondaryButton(m_rectBrowse, L"...", m_hoverButton == 2, m_downButton == 2);
}

void SplitDialog::OnMouseMove(float x, float y) {
    int hb = -1;
    if (PtInR(x, y, m_rectCancel)) hb = 0;
    else if (PtInR(x, y, m_rectSplit)) hb = 1;
    else if (PtInR(x, y, m_rectBrowse)) hb = 2;
    
    if (hb != m_hoverButton) {
        m_hoverButton = hb;
        Invalidate();
    }
}

void SplitDialog::OnMouseDown(float x, float y) {
    m_downButton = m_hoverButton;
    if (m_downButton != -1) Invalidate();
    
    float rm1y = m_rectMethod1.top + (m_params.splitMethod==0 ? 30.0f : 0.0f);
    D2D1_RECT_F rm1 = {m_rectMethod1.left, rm1y, m_rectMethod1.right, rm1y+20};
    float rm2y = m_rectMethod2.top + (m_params.splitMethod != 2 ? 30.0f : 0.0f);
    D2D1_RECT_F rm2 = {30, rm2y, 400, rm2y+20};
    
    if (PtInR(x, y, m_rectMethod0)) { 
        m_params.splitMethod = 0; 
        SetChildPos(m_editValue, 30, 97, 60, 22);
        ShowWindow(m_editValue, SW_SHOW);
        Invalidate(); 
    }
    else if (PtInR(x, y, rm1)) { 
        m_params.splitMethod = 1; 
        SetChildPos(m_editValue, 30.0f, rm1y + 28.0f, 60.0f, 22.0f);
        ShowWindow(m_editValue, SW_SHOW);
        Invalidate(); 
    }
    else if (PtInR(x, y, rm2)) { 
        m_params.splitMethod = 2; 
        ShowWindow(m_editValue, SW_HIDE);
        Invalidate(); 
    }
    
    SetFocus(m_hwnd);
}

void SplitDialog::OnMouseUp(float x, float y) {
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
                wchar_t buf[MAX_PATH];
                GetWindowTextW(m_editValue, buf, MAX_PATH);
                int val = _wtoi(buf);
                if (m_params.splitMethod == 0) {
                    if (val <= 0 || val > m_params.maxPages) {
                        std::wstring msg = L"Please enter a valid page count (1 to " + std::to_wstring(m_params.maxPages) + L").";
                        MessageDialog::Show(m_hwnd, L"Invalid Input", msg);
                        return;
                    }
                } else {
                    if (val <= 0) {
                        MessageDialog::Show(m_hwnd, L"Invalid Input", L"Please enter a valid positive integer.");
                        return;
                    }
                }
                m_params.methodValue = buf;
                GetWindowTextW(m_editPath, buf, MAX_PATH);
                m_params.outputFolder = buf;
                
                if (m_params.outputFolder.empty()) {
                    // Trigger Browse
                    IFileDialog *pfd;
                    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
                        DWORD dwOptions;
                        pfd->GetOptions(&dwOptions);
                        pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
                        if (SUCCEEDED(pfd->Show(m_hwnd))) {
                            IShellItem *psi;
                            if (SUCCEEDED(pfd->GetResult(&psi))) {
                                PWSTR pszPath;
                                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                                    SetWindowTextW(m_editPath, pszPath);
                                    m_params.outputFolder = pszPath;
                                    CoTaskMemFree(pszPath);
                                }
                                psi->Release();
                            } else {
                                pfd->Release();
                                return;
                            }
                        } else {
                            pfd->Release();
                            return;
                        }
                        pfd->Release();
                    }
                }
                
                if (!m_params.outputFolder.empty()) {
                    m_resultOk = true;
                    m_running = false;
                }
            } else if (db == 2) {
                IFileDialog *pfd;
                if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
                    DWORD dwOptions;
                    pfd->GetOptions(&dwOptions);
                    pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
                    if (SUCCEEDED(pfd->Show(m_hwnd))) {
                        IShellItem *psi;
                        if (SUCCEEDED(pfd->GetResult(&psi))) {
                            PWSTR pszPath;
                            if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                                SetWindowTextW(m_editPath, pszPath);
                                CoTaskMemFree(pszPath);
                            }
                            psi->Release();
                        }
                    }
                    pfd->Release();
                }
            }
        }
    }
}

} // namespace ui::dialogs


