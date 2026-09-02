#include "ui/dialogs/LinkDialog.h"
#include "ui/resource.h"
#include <commctrl.h>
#include <commdlg.h>
#include <vector>

namespace ui::dialogs {

bool LinkDialog::Show(HWND parentHwnd, LinkParams& params) {
    INT_PTR res = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_LINK_DIALOG),
        parentHwnd,
        DialogProc,
        reinterpret_cast<LPARAM>(&params)
    );
    return (res == IDOK);
}

INT_PTR CALLBACK LinkDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        SetWindowLongPtrW(hDlg, DWLP_USER, lParam);
        auto* pParams = reinterpret_cast<LinkParams*>(lParam);
        if (pParams) {
            OnInitDialog(hDlg, pParams);
        }
        return TRUE;
    }
    case WM_COMMAND: {
        auto* pParams = reinterpret_cast<LinkParams*>(GetWindowLongPtrW(hDlg, DWLP_USER));
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

void LinkDialog::OnInitDialog(HWND hDlg, LinkParams* pParams) {
    CheckRadioButton(hDlg, IDC_LINK_TYPE_URL, IDC_LINK_TYPE_PAGE,
                     pParams->isUrl ? IDC_LINK_TYPE_URL : IDC_LINK_TYPE_PAGE);

    SetDlgItemTextW(hDlg, IDC_LINK_URL_EDIT, pParams->url.c_str());

    HWND hSpin = GetDlgItem(hDlg, IDC_LINK_PAGE_SPIN);
    if (hSpin) {
        int maxPage = (pParams->totalPages > 0) ? pParams->totalPages : 99999;
        SendMessageW(hSpin, UDM_SETRANGE32, 1, maxPage);
    }
    SetDlgItemInt(hDlg, IDC_LINK_PAGE_EDIT, pParams->targetPage, FALSE);

    CheckDlgButton(hDlg, IDC_LINK_BORDER_CHECK, pParams->drawBorder ? BST_CHECKED : BST_UNCHECKED);

    EnableWindow(GetDlgItem(hDlg, IDC_LINK_URL_EDIT), pParams->isUrl ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_LINK_PAGE_EDIT), pParams->isUrl ? FALSE : TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_LINK_PAGE_SPIN), pParams->isUrl ? FALSE : TRUE);
}

bool LinkDialog::OnCommand(HWND hDlg, WPARAM wParam, LPARAM /*lParam*/, LinkParams* pParams) {
    WORD cmdId = LOWORD(wParam);
    switch (cmdId) {
    case IDC_LINK_TYPE_URL:
        EnableWindow(GetDlgItem(hDlg, IDC_LINK_URL_EDIT), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_LINK_PAGE_EDIT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LINK_PAGE_SPIN), FALSE);
        return true;

    case IDC_LINK_TYPE_PAGE:
        EnableWindow(GetDlgItem(hDlg, IDC_LINK_URL_EDIT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LINK_PAGE_EDIT), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_LINK_PAGE_SPIN), TRUE);
        return true;

    case IDC_LINK_COLOR_BTN: {
        static COLORREF customColors[16] = {
            RGB(0, 102, 204), RGB(255, 0, 0), RGB(0, 153, 76), RGB(255, 128, 0),
            RGB(128, 0, 128), RGB(0, 0, 0), RGB(255, 255, 255), RGB(128, 128, 128)
        };
        CHOOSECOLORW cc = { sizeof(CHOOSECOLORW) };
        cc.hwndOwner = hDlg;
        cc.lpCustColors = customColors;
        cc.rgbResult = pParams->borderColor;
        cc.Flags = CC_RGBINIT | CC_FULLOPEN;
        if (ChooseColorW(&cc)) {
            pParams->borderColor = cc.rgbResult;
        }
        return true;
    }

    case IDOK: {
        pParams->isUrl = (IsDlgButtonChecked(hDlg, IDC_LINK_TYPE_URL) == BST_CHECKED);
        
        wchar_t urlBuf[2048] = {};
        GetDlgItemTextW(hDlg, IDC_LINK_URL_EDIT, urlBuf, 2048);
        pParams->url = urlBuf;
        if (pParams->isUrl && pParams->url.empty()) {
            pParams->url = L"https://";
        }

        BOOL success = FALSE;
        int page = GetDlgItemInt(hDlg, IDC_LINK_PAGE_EDIT, &success, FALSE);
        if (success && page > 0) {
            pParams->targetPage = page;
        }

        pParams->drawBorder = (IsDlgButtonChecked(hDlg, IDC_LINK_BORDER_CHECK) == BST_CHECKED);

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
