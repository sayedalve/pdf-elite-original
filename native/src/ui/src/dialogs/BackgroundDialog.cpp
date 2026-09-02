#include "ui/dialogs/BackgroundDialog.h"
#include "ui/dialogs/MessageDialog.h"
#include "ui/resource.h"
#include <commctrl.h>
#include <commdlg.h>
#include <vector>
#include <filesystem>

namespace ui::dialogs {

bool BackgroundDialog::Show(HWND parentHwnd, BackgroundParams& params) {
    INT_PTR res = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_BACKGROUND_DIALOG),
        parentHwnd,
        DialogProc,
        reinterpret_cast<LPARAM>(&params)
    );
    return (res == IDOK);
}

INT_PTR CALLBACK BackgroundDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        SetWindowLongPtrW(hDlg, DWLP_USER, lParam);
        auto* pParams = reinterpret_cast<BackgroundParams*>(lParam);
        if (pParams) {
            OnInitDialog(hDlg, pParams);
        }
        return TRUE;
    }
    case WM_HSCROLL: {
        auto* pParams = reinterpret_cast<BackgroundParams*>(GetWindowLongPtrW(hDlg, DWLP_USER));
        if (pParams) {
            OnHScroll(hDlg, wParam, lParam, pParams);
        }
        return TRUE;
    }
    case WM_COMMAND: {
        auto* pParams = reinterpret_cast<BackgroundParams*>(GetWindowLongPtrW(hDlg, DWLP_USER));
        if (pParams && OnCommand(hDlg, wParam, lParam, pParams)) {
            return TRUE;
        }
        break;
    }
    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

void BackgroundDialog::OnInitDialog(HWND hDlg, BackgroundParams* pParams) {
    CheckRadioButton(hDlg, IDC_BG_TYPE_COLOR, IDC_BG_TYPE_IMAGE,
                     pParams->isColor ? IDC_BG_TYPE_COLOR : IDC_BG_TYPE_IMAGE);

    SetDlgItemTextW(hDlg, IDC_BG_IMAGE_PATH_EDIT, pParams->imagePath.c_str());

    HWND hSlider = GetDlgItem(hDlg, IDC_BG_OPACITY_SLIDER);
    if (hSlider) {
        SendMessageW(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
        int pos = static_cast<int>(pParams->opacity * 100.0);
        if (pos < 0) pos = 0;
        if (pos > 100) pos = 100;
        SendMessageW(hSlider, TBM_SETPOS, TRUE, pos);
        SetDlgItemTextW(hDlg, IDC_BG_OPACITY_EDIT, (std::to_wstring(pos) + L"%").c_str());
    }

    int pageBtn = IDC_BG_PAGE_ALL;
    if (pParams->pageScope == 1) pageBtn = IDC_BG_PAGE_CURRENT;
    else if (pParams->pageScope == 2) pageBtn = IDC_BG_PAGE_RANGE;
    CheckRadioButton(hDlg, IDC_BG_PAGE_ALL, IDC_BG_PAGE_RANGE, pageBtn);

    SetDlgItemTextW(hDlg, IDC_BG_RANGE_EDIT, pParams->pageRange.c_str());

    EnableWindow(GetDlgItem(hDlg, IDC_BG_COLOR_BTN), pParams->isColor ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_BG_IMAGE_PATH_EDIT), pParams->isColor ? FALSE : TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_BG_IMAGE_BROWSE_BTN), pParams->isColor ? FALSE : TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_BG_RANGE_EDIT), (pParams->pageScope == 2) ? TRUE : FALSE);
}

void BackgroundDialog::OnHScroll(HWND hDlg, WPARAM /*wParam*/, LPARAM lParam, BackgroundParams* pParams) {
    HWND hSlider = GetDlgItem(hDlg, IDC_BG_OPACITY_SLIDER);
    if (reinterpret_cast<HWND>(lParam) == hSlider) {
        int pos = static_cast<int>(SendMessageW(hSlider, TBM_GETPOS, 0, 0));
        pParams->opacity = pos / 100.0;
        SetDlgItemTextW(hDlg, IDC_BG_OPACITY_EDIT, (std::to_wstring(pos) + L"%").c_str());
    }
}

bool BackgroundDialog::OnCommand(HWND hDlg, WPARAM wParam, LPARAM /*lParam*/, BackgroundParams* pParams) {
    WORD cmdId = LOWORD(wParam);
    switch (cmdId) {
    case IDC_BG_TYPE_COLOR:
        EnableWindow(GetDlgItem(hDlg, IDC_BG_COLOR_BTN), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BG_IMAGE_PATH_EDIT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BG_IMAGE_BROWSE_BTN), FALSE);
        return true;

    case IDC_BG_TYPE_IMAGE:
        EnableWindow(GetDlgItem(hDlg, IDC_BG_COLOR_BTN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BG_IMAGE_PATH_EDIT), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BG_IMAGE_BROWSE_BTN), TRUE);
        return true;

    case IDC_BG_COLOR_BTN: {
        static COLORREF customColors[16] = {
            RGB(255, 255, 255), RGB(245, 245, 245), RGB(255, 255, 204), RGB(204, 230, 255),
            RGB(230, 255, 204), RGB(255, 204, 204), RGB(230, 204, 255), RGB(200, 200, 200)
        };
        CHOOSECOLORW cc = { sizeof(CHOOSECOLORW) };
        cc.hwndOwner = hDlg;
        cc.lpCustColors = customColors;
        cc.rgbResult = pParams->color;
        cc.Flags = CC_RGBINIT | CC_FULLOPEN;
        if (ChooseColorW(&cc)) {
            pParams->color = cc.rgbResult;
        }
        return true;
    }

    case IDC_BG_IMAGE_BROWSE_BTN: {
        wchar_t szFile[MAX_PATH] = {};
        OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
        ofn.hwndOwner = hDlg;
        ofn.lpstrFilter = L"Image Files (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (GetOpenFileNameW(&ofn)) {
            SetDlgItemTextW(hDlg, IDC_BG_IMAGE_PATH_EDIT, szFile);
        }
        return true;
    }

    case IDC_BG_PAGE_ALL:
    case IDC_BG_PAGE_CURRENT:
        EnableWindow(GetDlgItem(hDlg, IDC_BG_RANGE_EDIT), FALSE);
        return true;

    case IDC_BG_PAGE_RANGE:
        EnableWindow(GetDlgItem(hDlg, IDC_BG_RANGE_EDIT), TRUE);
        SetFocus(GetDlgItem(hDlg, IDC_BG_RANGE_EDIT));
        return true;

    case IDOK: {
        pParams->isColor = (IsDlgButtonChecked(hDlg, IDC_BG_TYPE_COLOR) == BST_CHECKED);
        if (!pParams->isColor) {
            wchar_t pathBuf[MAX_PATH] = {};
            GetDlgItemTextW(hDlg, IDC_BG_IMAGE_PATH_EDIT, pathBuf, MAX_PATH);
            pParams->imagePath = pathBuf;
            if (pParams->imagePath.empty() || !std::filesystem::exists(pParams->imagePath)) {
                MessageDialog::Show(hDlg, L"Invalid Image", L"Please select a valid existing image file for the background.");
                return true;
            }
        }

        HWND hSlider = GetDlgItem(hDlg, IDC_BG_OPACITY_SLIDER);
        if (hSlider) {
            int pos = static_cast<int>(SendMessageW(hSlider, TBM_GETPOS, 0, 0));
            pParams->opacity = pos / 100.0;
        }

        if (IsDlgButtonChecked(hDlg, IDC_BG_PAGE_ALL) == BST_CHECKED) {
            pParams->pageScope = 0;
        } else if (IsDlgButtonChecked(hDlg, IDC_BG_PAGE_CURRENT) == BST_CHECKED) {
            pParams->pageScope = 1;
        } else {
            pParams->pageScope = 2;
        }

        wchar_t rangeBuf[256] = {};
        GetDlgItemTextW(hDlg, IDC_BG_RANGE_EDIT, rangeBuf, 256);
        pParams->pageRange = rangeBuf;

        EndDialog(hDlg, IDOK);
        return true;
    }

    case IDCANCEL:
        EndDialog(hDlg, IDCANCEL);
        return true;
    }

    return false;
}

} // namespace ui::dialogs
