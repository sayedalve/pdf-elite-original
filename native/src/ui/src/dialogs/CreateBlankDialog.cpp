#include "ui/dialogs/CreateBlankDialog.h"
#include <iomanip>
#include <sstream>
#include <commdlg.h>
#include <shlobj.h>
#include <windowsx.h>

namespace ui::dialogs {

static double PointsToUnit(double pt, int unitIdx) {
    if (unitIdx == 1) return pt / 72.0;
    if (unitIdx == 2) return (pt / 72.0) * 25.4;
    return pt;
}

static double UnitToPoints(double val, int unitIdx) {
    if (unitIdx == 1) return val * 72.0;
    if (unitIdx == 2) return (val / 25.4) * 72.0;
    return val;
}

static std::wstring FormatDouble(double val) {
    std::wostringstream ss;
    ss << std::fixed << std::setprecision(2) << val;
    std::wstring s = ss.str();
    if (s.find(L'.') != std::wstring::npos) {
        while (s.back() == L'0') s.pop_back();
        if (s.back() == L'.') s.pop_back();
    }
    return s;
}

bool CreateBlankDialog::Show(HWND parentHwnd, CreateBlankParams& params) {
    CreateBlankDialog dlg(parentHwnd, params);
    return dlg.DoModal();
}

CreateBlankDialog::CreateBlankDialog(HWND parent, CreateBlankParams& params)
    : ModernDialog(parent, L"Create PDF", 600, 480), m_params(params) {}
CreateBlankDialog::~CreateBlankDialog() {}

LRESULT CALLBACK CreateBlankDialog::EditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto pThis = reinterpret_cast<CreateBlankDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (msg == WM_CHAR && wp == VK_RETURN) return 0;
    if (msg == WM_KILLFOCUS) pThis->SyncFromEdits();
    return CallWindowProc(pThis->m_oldEditProc, hwnd, msg, wp, lp);
}

void CreateBlankDialog::OnCreate() {
    auto hInst = GetModuleHandleW(nullptr);
    DWORD es = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
    m_editW = CreateWindowExW(0, L"EDIT", L"", es, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    m_editH = CreateWindowExW(0, L"EDIT", L"", es, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    m_editCount = CreateWindowExW(0, L"EDIT", L"", es | ES_NUMBER, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    m_editPath = CreateWindowExW(0, L"EDIT", L"", es, 0,0,0,0, m_hwnd, nullptr, hInst, nullptr);
    
    HFONT hFont = GetEditFont();
    SendMessage(m_editW, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_editH, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_editCount, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_editPath, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    m_oldEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(m_editW, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc)));
    SetWindowLongPtr(m_editH, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc));
    SetWindowLongPtr(m_editCount, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc));
    SetWindowLongPtr(m_editPath, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProc));
    
    SetWindowLongPtr(m_editW, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetWindowLongPtr(m_editH, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetWindowLongPtr(m_editCount, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetWindowLongPtr(m_editPath, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    
    SyncToEdits();
}

void CreateBlankDialog::OnLayout(float w, float h) {
    m_rectCreate = {w - 220, h - 45, w - 120, h - 15};
    m_rectCancel = {w - 110, h - 45, w - 20, h - 15};
    m_rectBrowse = {w - 120, h - 134, w - 30, h - 104};
    for (int i=0; i<5; i++) {
        m_rectPreset[i] = {30, 80.0f + i*30.0f, 180, 104.0f + i*30.0f};
    }
    
    m_rectPortrait = {220, 110, 320, 134};
    m_rectLandscape = {330, 110, 440, 134};
    
    if (m_editW) SetChildPos(m_editW, 270, 153, 60, 22);
    if (m_editH) SetChildPos(m_editH, 350, 153, 60, 22);
    if (m_editCount) SetChildPos(m_editCount, 330, 213, 60, 22);
    if (m_editPath) SetChildPos(m_editPath, 40, h - 129, w - 180, 22);
}

void CreateBlankDialog::OnRender() {
    float h = static_cast<float>(m_height);
    float w = static_cast<float>(m_width);

    m_target->DrawTextW(L"Configure new document properties.", 34, m_formatLabel.Get(), {30, 45, 450, 65}, m_textBrush.Get());
    
    m_target->DrawTextW(L"Page Size", 9, m_formatTitle.Get(), {30, 60, 200, 80}, m_textBrush.Get());
    const wchar_t* presets[] = {L"Letter", L"A4", L"Legal", L"A3", L"Custom"};
    for (int i=0; i<5; i++) {
        DrawRadio(m_rectPreset[i], presets[i], (int)wcslen(presets[i]), m_params.pageSizeIndex == i);
    }
    
    m_target->DrawTextW(L"Orientation", 11, m_formatTitle.Get(), {220, 80, 400, 100}, m_textBrush.Get());
    DrawRadio(m_rectPortrait, L"Portrait", 8, m_params.isPortrait);
    DrawRadio(m_rectLandscape, L"Landscape", 9, !m_params.isPortrait);
    
    m_target->DrawTextW(L"Dimensions", 10, m_formatTitle.Get(), {220, 130, 400, 150}, m_textBrush.Get());
    m_target->DrawTextW(L"W:", 2, m_formatLabel.Get(), {240, 153, 260, 173}, m_textBrush.Get());
    DrawEditBorder({266, 149, 334, 179});
    m_target->DrawTextW(L"H:", 2, m_formatLabel.Get(), {340, 153, 360, 173}, m_textBrush.Get());
    DrawEditBorder({346, 149, 414, 179});
    
    m_target->DrawTextW(L"Page Count", 10, m_formatTitle.Get(), {220, 190, 400, 210}, m_textBrush.Get());
    m_target->DrawTextW(L"Number of pages:", 16, m_formatLabel.Get(), {220, 213, 320, 233}, m_textBrush.Get());
    DrawEditBorder({326, 209, 394, 239});
    
    m_target->DrawTextW(L"Save As", 7, m_formatTitle.Get(), {40, h - 160, 300, h - 135}, m_textBrush.Get());
    DrawEditBorder({36, h - 133, w - 176, h - 103});
    
    DrawPrimaryButton(m_rectCreate, L"Create", m_hoverButton == 1, m_downButton == 1);
    DrawSecondaryButton(m_rectCancel, L"Cancel", m_hoverButton == 0, m_downButton == 0);
    DrawSecondaryButton(m_rectBrowse, L"Browse...", m_hoverButton == 2, m_downButton == 2);
}

void CreateBlankDialog::OnMouseMove(float x, float y) {
    int hb = -1;
    if (PtInR(x, y, m_rectCancel)) hb = 0;
    else if (PtInR(x, y, m_rectCreate)) hb = 1;
    else if (PtInR(x, y, m_rectBrowse)) hb = 2;
    else if (PtInR(x, y, m_rectPortrait) || PtInR(x, y, m_rectLandscape)) SetCursor(LoadCursor(nullptr, IDC_HAND));
    else {
        for (int i=0; i<5; i++) {
            if (PtInR(x, y, m_rectPreset[i])) SetCursor(LoadCursor(nullptr, IDC_HAND));
        }
    }
    
    if (hb != m_hoverButton) {
        m_hoverButton = hb;
        Invalidate();
    }
}

void CreateBlankDialog::OnMouseDown(float x, float y) {
    m_downButton = m_hoverButton;
    if (m_downButton != -1) Invalidate();
    
    for (int i=0; i<5; i++) {
        if (PtInR(x, y, m_rectPreset[i])) { SetPreset(i); Invalidate(); }
    }
    
    if (PtInR(x, y, m_rectPortrait)) {
        if (!m_params.isPortrait) {
            m_params.isPortrait = true;
            double t = m_params.widthPt; m_params.widthPt = m_params.heightPt; m_params.heightPt = t;
            SyncToEdits(); Invalidate();
        }
    } else if (PtInR(x, y, m_rectLandscape)) {
        if (m_params.isPortrait) {
            m_params.isPortrait = false;
            double t = m_params.widthPt; m_params.widthPt = m_params.heightPt; m_params.heightPt = t;
            SyncToEdits(); Invalidate();
        }
    }
    
    SetFocus(m_hwnd);
}

void CreateBlankDialog::OnMouseUp(float x, float y) {
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
                SyncFromEdits();
                m_resultOk = true;
                m_running = false;
            } else if (db == 2) {
                IFileDialog *pfd;
                if (SUCCEEDED(CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
                    DWORD dwOptions;
                    pfd->GetOptions(&dwOptions);
                    pfd->SetOptions(dwOptions | FOS_FORCEFILESYSTEM);
                    COMDLG_FILTERSPEC rgSpec[] = {{L"PDF Document", L"*.pdf"}};
                    pfd->SetFileTypes(1, rgSpec);
                    pfd->SetDefaultExtension(L"pdf");
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

void CreateBlankDialog::SyncToEdits() {
    double w = PointsToUnit(m_params.widthPt, m_params.unitIndex);
    double h = PointsToUnit(m_params.heightPt, m_params.unitIndex);
    SetWindowTextW(m_editW, FormatDouble(w).c_str());
    SetWindowTextW(m_editH, FormatDouble(h).c_str());
    SetWindowTextW(m_editCount, std::to_wstring(m_params.pageCount).c_str());
    SetWindowTextW(m_editPath, m_params.outputPath.c_str());
}

void CreateBlankDialog::SyncFromEdits() {
    wchar_t buf[256];
    GetWindowTextW(m_editW, buf, 256);
    double w = _wtof(buf);
    GetWindowTextW(m_editH, buf, 256);
    double h = _wtof(buf);
    
    m_params.widthPt = UnitToPoints(w, m_params.unitIndex);
    m_params.heightPt = UnitToPoints(h, m_params.unitIndex);
    
    GetWindowTextW(m_editCount, buf, 256);
    m_params.pageCount = _wtoi(buf);
    if (m_params.pageCount < 1) m_params.pageCount = 1;
    
    GetWindowTextW(m_editPath, buf, 256);
    m_params.outputPath = buf;
    
    if (m_params.pageSizeIndex != 4) {
        // If they manually edited w/h to something else, switch to Custom
        // For simplicity, we just set custom if they lose focus.
        m_params.pageSizeIndex = 4;
        Invalidate();
    }
}

void CreateBlankDialog::SetPreset(int index) {
    m_params.pageSizeIndex = index;
    if (index == 0) { // Letter
        m_params.widthPt = m_params.isPortrait ? 612.0 : 792.0;
        m_params.heightPt = m_params.isPortrait ? 792.0 : 612.0;
    } else if (index == 1) { // A4
        m_params.widthPt = m_params.isPortrait ? 595.28 : 841.89;
        m_params.heightPt = m_params.isPortrait ? 841.89 : 595.28;
    } else if (index == 2) { // Legal
        m_params.widthPt = m_params.isPortrait ? 612.0 : 1008.0;
        m_params.heightPt = m_params.isPortrait ? 1008.0 : 612.0;
    } else if (index == 3) { // A3
        m_params.widthPt = m_params.isPortrait ? 841.89 : 1190.55;
        m_params.heightPt = m_params.isPortrait ? 1190.55 : 841.89;
    }
    SyncToEdits();
}

} // namespace ui::dialogs

