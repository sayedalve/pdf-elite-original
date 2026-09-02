#include "ui/dialogs/WatermarkDialog.h"
#include "ui/dialogs/MessageDialog.h"
#include "ui/resource.h"
#include <commctrl.h>
#include <commdlg.h>
#include <vector>
#include <cmath>

namespace ui::dialogs {

bool WatermarkDialog::Show(HWND parentHwnd, WatermarkParams& params) {
    INT_PTR res = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_WATERMARK_DIALOG),
        parentHwnd,
        DialogProc,
        reinterpret_cast<LPARAM>(&params)
    );
    return (res == IDOK);
}

INT_PTR CALLBACK WatermarkDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        SetWindowLongPtrW(hDlg, DWLP_USER, lParam);
        auto* pParams = reinterpret_cast<WatermarkParams*>(lParam);
        if (pParams) {
            OnInitDialog(hDlg, pParams);
        }
        return TRUE;
    }
    case WM_HSCROLL: {
        auto* pParams = reinterpret_cast<WatermarkParams*>(GetWindowLongPtrW(hDlg, DWLP_USER));
        if (pParams) {
            OnHScroll(hDlg, wParam, lParam, pParams);
        }
        return TRUE;
    }
    case WM_COMMAND: {
        auto* pParams = reinterpret_cast<WatermarkParams*>(GetWindowLongPtrW(hDlg, DWLP_USER));
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

void WatermarkDialog::OnInitDialog(HWND hDlg, WatermarkParams* pParams) {
    SetDlgItemTextW(hDlg, IDC_WM_TEXT_EDIT, pParams->text.c_str());

    // Font combo
    HWND hFontCombo = GetDlgItem(hDlg, IDC_WM_FONT_COMBO);
    const wchar_t* fonts[] = { L"Helvetica", L"Times-Roman", L"Courier", L"Arial", L"Calibri", L"Segoe UI" };
    int selectedFont = 0;
    for (int i = 0; i < 6; ++i) {
        SendMessageW(hFontCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fonts[i]));
        if (_wcsicmp(fonts[i], pParams->fontName.c_str()) == 0) {
            selectedFont = i;
        }
    }
    SendMessageW(hFontCombo, CB_SETCURSEL, selectedFont, 0);

    // Font size
    HWND hSpin = GetDlgItem(hDlg, IDC_WM_SIZE_SPIN);
    if (hSpin) {
        SendMessageW(hSpin, UDM_SETRANGE32, 6, 288);
    }
    SetDlgItemInt(hDlg, IDC_WM_SIZE_EDIT, static_cast<UINT>(pParams->fontSize), FALSE);

    CheckDlgButton(hDlg, IDC_WM_BOLD_CHECK, pParams->bold ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_WM_ITALIC_CHECK, pParams->italic ? BST_CHECKED : BST_UNCHECKED);

    // Opacity slider
    HWND hSlider = GetDlgItem(hDlg, IDC_WM_OPACITY_SLIDER);
    if (hSlider) {
        SendMessageW(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
        int pos = static_cast<int>(pParams->opacity * 100.0f);
        if (pos < 0) pos = 0;
        if (pos > 100) pos = 100;
        SendMessageW(hSlider, TBM_SETPOS, TRUE, pos);
        SetDlgItemTextW(hDlg, IDC_WM_OPACITY_EDIT, (std::to_wstring(pos) + L"%").c_str());
    }

    // Rotation combo
    HWND hRotCombo = GetDlgItem(hDlg, IDC_WM_ROT_COMBO);
    SendMessageW(hRotCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"45\x00B0 Diagonal"));
    SendMessageW(hRotCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"-45\x00B0 Diagonal"));
    SendMessageW(hRotCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"0\x00B0 Horizontal"));
    SendMessageW(hRotCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"90\x00B0 Vertical"));
    SendMessageW(hRotCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Custom Angle"));

    int rotSel = 4;
    if (std::abs(pParams->rotation - 45.0f) < 0.1f) rotSel = 0;
    else if (std::abs(pParams->rotation - (-45.0f)) < 0.1f) rotSel = 1;
    else if (std::abs(pParams->rotation - 0.0f) < 0.1f) rotSel = 2;
    else if (std::abs(pParams->rotation - 90.0f) < 0.1f) rotSel = 3;
    SendMessageW(hRotCombo, CB_SETCURSEL, rotSel, 0);

    SetDlgItemTextW(hDlg, IDC_WM_ROT_EDIT, std::to_wstring(static_cast<int>(pParams->rotation)).c_str());
    EnableWindow(GetDlgItem(hDlg, IDC_WM_ROT_EDIT), (rotSel == 4) ? TRUE : FALSE);

    // Position combo
    HWND hPosCombo = GetDlgItem(hDlg, IDC_WM_POS_COMBO);
    const wchar_t* positions[] = {
        L"Center", L"Top Left", L"Top Center", L"Top Right",
        L"Bottom Left", L"Bottom Center", L"Bottom Right"
    };
    for (int i = 0; i < 7; ++i) {
        SendMessageW(hPosCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(positions[i]));
    }
    int posIndex = pParams->positionIndex;
    if (posIndex < 0 || posIndex > 6) posIndex = 0;
    SendMessageW(hPosCombo, CB_SETCURSEL, posIndex, 0);

    // Layer selection
    CheckRadioButton(hDlg, IDC_WM_LAYER_OVER, IDC_WM_LAYER_UNDER,
                     pParams->layerOver ? IDC_WM_LAYER_OVER : IDC_WM_LAYER_UNDER);

    // Page range
    int pageBtn = IDC_WM_PAGE_ALL;
    if (pParams->pageScope == 1) pageBtn = IDC_WM_PAGE_CURRENT;
    else if (pParams->pageScope == 2) pageBtn = IDC_WM_PAGE_RANGE;
    CheckRadioButton(hDlg, IDC_WM_PAGE_ALL, IDC_WM_PAGE_RANGE, pageBtn);

    SetDlgItemTextW(hDlg, IDC_WM_RANGE_EDIT, pParams->pageRange.c_str());
    EnableWindow(GetDlgItem(hDlg, IDC_WM_RANGE_EDIT), (pParams->pageScope == 2) ? TRUE : FALSE);
}

void WatermarkDialog::OnHScroll(HWND hDlg, WPARAM /*wParam*/, LPARAM lParam, WatermarkParams* pParams) {
    HWND hSlider = GetDlgItem(hDlg, IDC_WM_OPACITY_SLIDER);
    if (reinterpret_cast<HWND>(lParam) == hSlider) {
        int pos = static_cast<int>(SendMessageW(hSlider, TBM_GETPOS, 0, 0));
        pParams->opacity = pos / 100.0f;
        SetDlgItemTextW(hDlg, IDC_WM_OPACITY_EDIT, (std::to_wstring(pos) + L"%").c_str());
    }
}

bool WatermarkDialog::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam, WatermarkParams* pParams) {
    WORD cmdId = LOWORD(wParam);
    WORD notif = HIWORD(wParam);

    switch (cmdId) {
    case IDC_WM_ROT_COMBO:
        if (notif == CBN_SELCHANGE) {
            int sel = static_cast<int>(SendMessageW(reinterpret_cast<HWND>(lParam), CB_GETCURSEL, 0, 0));
            switch (sel) {
            case 0: SetDlgItemTextW(hDlg, IDC_WM_ROT_EDIT, L"45"); EnableWindow(GetDlgItem(hDlg, IDC_WM_ROT_EDIT), FALSE); break;
            case 1: SetDlgItemTextW(hDlg, IDC_WM_ROT_EDIT, L"-45"); EnableWindow(GetDlgItem(hDlg, IDC_WM_ROT_EDIT), FALSE); break;
            case 2: SetDlgItemTextW(hDlg, IDC_WM_ROT_EDIT, L"0"); EnableWindow(GetDlgItem(hDlg, IDC_WM_ROT_EDIT), FALSE); break;
            case 3: SetDlgItemTextW(hDlg, IDC_WM_ROT_EDIT, L"90"); EnableWindow(GetDlgItem(hDlg, IDC_WM_ROT_EDIT), FALSE); break;
            case 4: EnableWindow(GetDlgItem(hDlg, IDC_WM_ROT_EDIT), TRUE); SetFocus(GetDlgItem(hDlg, IDC_WM_ROT_EDIT)); break;
            }
            return true;
        }
        break;

    case IDC_WM_COLOR_BTN: {
        static COLORREF customColors[16] = {
            RGB(192, 192, 192), RGB(255, 0, 0), RGB(0, 153, 76), RGB(0, 102, 204),
            RGB(255, 128, 0), RGB(128, 0, 128), RGB(0, 0, 0), RGB(128, 128, 128)
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

    case IDC_WM_PAGE_ALL:
    case IDC_WM_PAGE_CURRENT:
        EnableWindow(GetDlgItem(hDlg, IDC_WM_RANGE_EDIT), FALSE);
        return true;

    case IDC_WM_PAGE_RANGE:
        EnableWindow(GetDlgItem(hDlg, IDC_WM_RANGE_EDIT), TRUE);
        SetFocus(GetDlgItem(hDlg, IDC_WM_RANGE_EDIT));
        return true;

    case IDOK: {
        wchar_t textBuf[1024] = {};
        GetDlgItemTextW(hDlg, IDC_WM_TEXT_EDIT, textBuf, 1024);
        pParams->text = textBuf;
        if (pParams->text.empty()) {
            MessageDialog::Show(hDlg, L"Empty Text", L"Please enter text for the watermark.");
            return true;
        }

        wchar_t fontBuf[128] = {};
        GetDlgItemTextW(hDlg, IDC_WM_FONT_COMBO, fontBuf, 128);
        pParams->fontName = fontBuf;

        BOOL sizeOk = FALSE;
        int sz = GetDlgItemInt(hDlg, IDC_WM_SIZE_EDIT, &sizeOk, FALSE);
        if (sizeOk && sz >= 6 && sz <= 288) {
            pParams->fontSize = static_cast<float>(sz);
        }

        pParams->bold = (IsDlgButtonChecked(hDlg, IDC_WM_BOLD_CHECK) == BST_CHECKED);
        pParams->italic = (IsDlgButtonChecked(hDlg, IDC_WM_ITALIC_CHECK) == BST_CHECKED);

        HWND hSlider = GetDlgItem(hDlg, IDC_WM_OPACITY_SLIDER);
        if (hSlider) {
            int pos = static_cast<int>(SendMessageW(hSlider, TBM_GETPOS, 0, 0));
            pParams->opacity = pos / 100.0f;
        }

        BOOL rotOk = FALSE;
        int rot = GetDlgItemInt(hDlg, IDC_WM_ROT_EDIT, &rotOk, TRUE);
        if (rotOk) {
            pParams->rotation = static_cast<float>(rot);
        }

        int posIdx = static_cast<int>(SendMessageW(GetDlgItem(hDlg, IDC_WM_POS_COMBO), CB_GETCURSEL, 0, 0));
        if (posIdx >= 0 && posIdx <= 6) {
            pParams->positionIndex = posIdx;
        }

        pParams->layerOver = (IsDlgButtonChecked(hDlg, IDC_WM_LAYER_OVER) == BST_CHECKED);

        if (IsDlgButtonChecked(hDlg, IDC_WM_PAGE_ALL) == BST_CHECKED) {
            pParams->pageScope = 0;
        } else if (IsDlgButtonChecked(hDlg, IDC_WM_PAGE_CURRENT) == BST_CHECKED) {
            pParams->pageScope = 1;
        } else {
            pParams->pageScope = 2;
        }

        wchar_t rangeBuf[256] = {};
        GetDlgItemTextW(hDlg, IDC_WM_RANGE_EDIT, rangeBuf, 256);
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
