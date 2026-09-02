#include "ui/dialogs/HeaderFooterDialog.h"
#include "ui/dialogs/MessageDialog.h"
#include "ui/resource.h"
#include <commctrl.h>
#include <commdlg.h>
#include <vector>

namespace ui::dialogs {

static int s_lastFocusedEdit = IDC_HF_TOP_CENTER_EDIT;

bool HeaderFooterDialog::Show(HWND parentHwnd, HeaderFooterParams& params) {
    INT_PTR res = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_HEADER_FOOTER_DIALOG),
        parentHwnd,
        DialogProc,
        reinterpret_cast<LPARAM>(&params)
    );
    return (res == IDOK);
}

INT_PTR CALLBACK HeaderFooterDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        SetWindowLongPtrW(hDlg, DWLP_USER, lParam);
        auto* pParams = reinterpret_cast<HeaderFooterParams*>(lParam);
        if (pParams) {
            OnInitDialog(hDlg, pParams);
        }
        return TRUE;
    }
    case WM_COMMAND: {
        auto* pParams = reinterpret_cast<HeaderFooterParams*>(GetWindowLongPtrW(hDlg, DWLP_USER));
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

void HeaderFooterDialog::OnInitDialog(HWND hDlg, HeaderFooterParams* pParams) {
    s_lastFocusedEdit = IDC_HF_TOP_CENTER_EDIT;

    SetDlgItemTextW(hDlg, IDC_HF_TOP_LEFT_EDIT, pParams->leftHeader.c_str());
    SetDlgItemTextW(hDlg, IDC_HF_TOP_CENTER_EDIT, pParams->centerHeader.c_str());
    SetDlgItemTextW(hDlg, IDC_HF_TOP_RIGHT_EDIT, pParams->rightHeader.c_str());
    SetDlgItemTextW(hDlg, IDC_HF_BOTTOM_LEFT_EDIT, pParams->leftFooter.c_str());
    SetDlgItemTextW(hDlg, IDC_HF_BOTTOM_CENTER_EDIT, pParams->centerFooter.c_str());
    SetDlgItemTextW(hDlg, IDC_HF_BOTTOM_RIGHT_EDIT, pParams->rightFooter.c_str());

    // Font combo
    HWND hFontCombo = GetDlgItem(hDlg, IDC_HF_FONT_COMBO);
    const wchar_t* fonts[] = { L"Helvetica", L"Times-Roman", L"Courier", L"Arial", L"Segoe UI" };
    int selectedFont = 0;
    for (int i = 0; i < 5; ++i) {
        SendMessageW(hFontCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fonts[i]));
        if (_wcsicmp(fonts[i], pParams->fontName.c_str()) == 0) {
            selectedFont = i;
        }
    }
    SendMessageW(hFontCombo, CB_SETCURSEL, selectedFont, 0);

    SetDlgItemInt(hDlg, IDC_HF_SIZE_EDIT, static_cast<UINT>(pParams->fontSize), FALSE);

    SetDlgItemInt(hDlg, IDC_HF_MARGIN_TOP_EDIT, static_cast<UINT>(pParams->topMargin), FALSE);
    SetDlgItemInt(hDlg, IDC_HF_MARGIN_BOTTOM_EDIT, static_cast<UINT>(pParams->bottomMargin), FALSE);
    SetDlgItemInt(hDlg, IDC_HF_MARGIN_LEFT_EDIT, static_cast<UINT>(pParams->leftMargin), FALSE);
    SetDlgItemInt(hDlg, IDC_HF_MARGIN_RIGHT_EDIT, static_cast<UINT>(pParams->rightMargin), FALSE);

    int pageBtn = (pParams->pageScope == 1) ? IDC_HF_PAGE_RANGE : IDC_HF_PAGE_ALL;
    CheckRadioButton(hDlg, IDC_HF_PAGE_ALL, IDC_HF_PAGE_RANGE, pageBtn);

    SetDlgItemTextW(hDlg, IDC_HF_RANGE_EDIT, pParams->pageRange.c_str());
    EnableWindow(GetDlgItem(hDlg, IDC_HF_RANGE_EDIT), (pParams->pageScope == 1) ? TRUE : FALSE);
}

void HeaderFooterDialog::InsertMacroToken(HWND hDlg, const std::wstring& token) {
    HWND hEdit = GetDlgItem(hDlg, s_lastFocusedEdit);
    if (!hEdit) {
        hEdit = GetDlgItem(hDlg, IDC_HF_BOTTOM_CENTER_EDIT);
    }
    if (hEdit) {
        SetFocus(hEdit);
        SendMessageW(hEdit, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(token.c_str()));
    }
}

bool HeaderFooterDialog::OnCommand(HWND hDlg, WPARAM wParam, LPARAM /*lParam*/, HeaderFooterParams* pParams) {
    WORD cmdId = LOWORD(wParam);
    WORD notif = HIWORD(wParam);

    if (notif == EN_SETFOCUS) {
        if (cmdId == IDC_HF_TOP_LEFT_EDIT || cmdId == IDC_HF_TOP_CENTER_EDIT || cmdId == IDC_HF_TOP_RIGHT_EDIT ||
            cmdId == IDC_HF_BOTTOM_LEFT_EDIT || cmdId == IDC_HF_BOTTOM_CENTER_EDIT || cmdId == IDC_HF_BOTTOM_RIGHT_EDIT) {
            s_lastFocusedEdit = cmdId;
        }
    }

    switch (cmdId) {
    case IDC_HF_MACRO_PAGE_NUM:
        InsertMacroToken(hDlg, L"<<PageNumber>>");
        return true;

    case IDC_HF_MACRO_PAGE_COUNT:
        InsertMacroToken(hDlg, L"<<TotalPages>>");
        return true;

    case IDC_HF_MACRO_DATE:
        InsertMacroToken(hDlg, L"<<Date>>");
        return true;

    case IDC_HF_COLOR_BTN: {
        static COLORREF customColors[16] = {
            RGB(0, 0, 0), RGB(64, 64, 64), RGB(128, 128, 128), RGB(0, 102, 204),
            RGB(255, 0, 0), RGB(0, 153, 76), RGB(255, 128, 0), RGB(128, 0, 128)
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

    case IDC_HF_PAGE_ALL:
        EnableWindow(GetDlgItem(hDlg, IDC_HF_RANGE_EDIT), FALSE);
        return true;

    case IDC_HF_PAGE_RANGE:
        EnableWindow(GetDlgItem(hDlg, IDC_HF_RANGE_EDIT), TRUE);
        SetFocus(GetDlgItem(hDlg, IDC_HF_RANGE_EDIT));
        return true;

    case IDOK: {
        wchar_t buf[1024] = {};
        GetDlgItemTextW(hDlg, IDC_HF_TOP_LEFT_EDIT, buf, 1024);
        pParams->leftHeader = buf;

        GetDlgItemTextW(hDlg, IDC_HF_TOP_CENTER_EDIT, buf, 1024);
        pParams->centerHeader = buf;

        GetDlgItemTextW(hDlg, IDC_HF_TOP_RIGHT_EDIT, buf, 1024);
        pParams->rightHeader = buf;

        GetDlgItemTextW(hDlg, IDC_HF_BOTTOM_LEFT_EDIT, buf, 1024);
        pParams->leftFooter = buf;

        GetDlgItemTextW(hDlg, IDC_HF_BOTTOM_CENTER_EDIT, buf, 1024);
        pParams->centerFooter = buf;

        GetDlgItemTextW(hDlg, IDC_HF_BOTTOM_RIGHT_EDIT, buf, 1024);
        pParams->rightFooter = buf;

        if (pParams->leftHeader.empty() && pParams->centerHeader.empty() && pParams->rightHeader.empty() &&
            pParams->leftFooter.empty() && pParams->centerFooter.empty() && pParams->rightFooter.empty()) {
            MessageDialog::Show(hDlg, L"Empty Header/Footer", L"Please enter text or tokens into at least one header or footer field.");
            return true;
        }

        wchar_t fontBuf[128] = {};
        GetDlgItemTextW(hDlg, IDC_HF_FONT_COMBO, fontBuf, 128);
        pParams->fontName = fontBuf;

        BOOL szOk = FALSE;
        int sz = GetDlgItemInt(hDlg, IDC_HF_SIZE_EDIT, &szOk, FALSE);
        if (szOk && sz >= 4 && sz <= 72) {
            pParams->fontSize = static_cast<float>(sz);
        }

        BOOL mOk = FALSE;
        int mTop = GetDlgItemInt(hDlg, IDC_HF_MARGIN_TOP_EDIT, &mOk, FALSE);
        if (mOk) pParams->topMargin = static_cast<float>(mTop);

        int mBot = GetDlgItemInt(hDlg, IDC_HF_MARGIN_BOTTOM_EDIT, &mOk, FALSE);
        if (mOk) pParams->bottomMargin = static_cast<float>(mBot);

        int mLeft = GetDlgItemInt(hDlg, IDC_HF_MARGIN_LEFT_EDIT, &mOk, FALSE);
        if (mOk) pParams->leftMargin = static_cast<float>(mLeft);

        int mRight = GetDlgItemInt(hDlg, IDC_HF_MARGIN_RIGHT_EDIT, &mOk, FALSE);
        if (mOk) pParams->rightMargin = static_cast<float>(mRight);

        pParams->pageScope = (IsDlgButtonChecked(hDlg, IDC_HF_PAGE_RANGE) == BST_CHECKED) ? 1 : 0;

        wchar_t rangeBuf[256] = {};
        GetDlgItemTextW(hDlg, IDC_HF_RANGE_EDIT, rangeBuf, 256);
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
