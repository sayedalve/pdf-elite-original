#include "ui/dialogs/ExtractImagesDialog.h"
#include "ui/dialogs/MessageDialog.h"
#include "ui/resource.h"
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <vector>
#include <filesystem>

namespace ui::dialogs {

static bool PickFolder(HWND parentHwnd, std::wstring& selectedPath) {
    bool success = false;
    IFileOpenDialog* pFileOpen = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
    if (SUCCEEDED(hr)) {
        DWORD dwOptions = 0;
        if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions))) {
            pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        }
        pFileOpen->SetTitle(L"Select Destination Folder for Extracted Images");
        if (SUCCEEDED(pFileOpen->Show(parentHwnd))) {
            IShellItem* pItem = nullptr;
            if (SUCCEEDED(pFileOpen->GetResult(&pItem))) {
                PWSTR pszFilePath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    selectedPath = pszFilePath;
                    CoTaskMemFree(pszFilePath);
                    success = true;
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }
    return success;
}

bool ExtractImagesDialog::Show(HWND parentHwnd, ExtractImagesParams& params) {
    INT_PTR res = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_EXTRACT_IMAGES_DIALOG),
        parentHwnd,
        DialogProc,
        reinterpret_cast<LPARAM>(&params)
    );
    return (res == IDOK);
}

INT_PTR CALLBACK ExtractImagesDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        SetWindowLongPtrW(hDlg, DWLP_USER, lParam);
        auto* pParams = reinterpret_cast<ExtractImagesParams*>(lParam);
        if (pParams) {
            OnInitDialog(hDlg, pParams);
        }
        return TRUE;
    }
    case WM_COMMAND: {
        auto* pParams = reinterpret_cast<ExtractImagesParams*>(GetWindowLongPtrW(hDlg, DWLP_USER));
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

void ExtractImagesDialog::OnInitDialog(HWND hDlg, ExtractImagesParams* pParams) {
    SetDlgItemTextW(hDlg, IDC_EXTRACT_SRC_EDIT, pParams->srcPdfPath.c_str());
    SetDlgItemTextW(hDlg, IDC_EXTRACT_DEST_EDIT, pParams->outputDir.c_str());

    HWND hFmtCombo = GetDlgItem(hDlg, IDC_EXTRACT_FORMAT_COMBO);
    SendMessageW(hFmtCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"PNG"));
    SendMessageW(hFmtCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"JPEG"));
    SendMessageW(hFmtCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"BMP"));

    int fmtSel = 0;
    if (_wcsicmp(pParams->format.c_str(), L"JPEG") == 0 || _wcsicmp(pParams->format.c_str(), L"JPG") == 0) {
        fmtSel = 1;
    } else if (_wcsicmp(pParams->format.c_str(), L"BMP") == 0) {
        fmtSel = 2;
    }
    SendMessageW(hFmtCombo, CB_SETCURSEL, fmtSel, 0);

    SetDlgItemTextW(hDlg, IDC_EXTRACT_PREFIX_EDIT, pParams->prefix.c_str());

    int pageBtn = (pParams->pageScope == 1) ? IDC_EXTRACT_PAGE_RANGE : IDC_EXTRACT_PAGE_ALL;
    CheckRadioButton(hDlg, IDC_EXTRACT_PAGE_ALL, IDC_EXTRACT_PAGE_RANGE, pageBtn);

    SetDlgItemTextW(hDlg, IDC_EXTRACT_RANGE_EDIT, pParams->pageRange.c_str());
    EnableWindow(GetDlgItem(hDlg, IDC_EXTRACT_RANGE_EDIT), (pParams->pageScope == 1) ? TRUE : FALSE);
}

bool ExtractImagesDialog::OnCommand(HWND hDlg, WPARAM wParam, LPARAM /*lParam*/, ExtractImagesParams* pParams) {
    WORD cmdId = LOWORD(wParam);
    switch (cmdId) {
    case IDC_EXTRACT_SRC_BROWSE: {
        wchar_t szFile[MAX_PATH] = {};
        OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
        ofn.hwndOwner = hDlg;
        ofn.lpstrFilter = L"PDF Files (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (GetOpenFileNameW(&ofn)) {
            SetDlgItemTextW(hDlg, IDC_EXTRACT_SRC_EDIT, szFile);
        }
        return true;
    }

    case IDC_EXTRACT_DEST_BROWSE: {
        std::wstring folder;
        if (PickFolder(hDlg, folder)) {
            SetDlgItemTextW(hDlg, IDC_EXTRACT_DEST_EDIT, folder.c_str());
        }
        return true;
    }

    case IDC_EXTRACT_PAGE_ALL:
        EnableWindow(GetDlgItem(hDlg, IDC_EXTRACT_RANGE_EDIT), FALSE);
        return true;

    case IDC_EXTRACT_PAGE_RANGE:
        EnableWindow(GetDlgItem(hDlg, IDC_EXTRACT_RANGE_EDIT), TRUE);
        SetFocus(GetDlgItem(hDlg, IDC_EXTRACT_RANGE_EDIT));
        return true;

    case IDOK: {
        wchar_t srcBuf[MAX_PATH] = {};
        GetDlgItemTextW(hDlg, IDC_EXTRACT_SRC_EDIT, srcBuf, MAX_PATH);
        pParams->srcPdfPath = srcBuf;
        if (pParams->srcPdfPath.empty() || !std::filesystem::exists(pParams->srcPdfPath)) {
            MessageDialog::Show(hDlg, L"Invalid Source File", L"Please select a valid existing PDF file to extract images from.");
            return true;
        }

        wchar_t destBuf[MAX_PATH] = {};
        GetDlgItemTextW(hDlg, IDC_EXTRACT_DEST_EDIT, destBuf, MAX_PATH);
        pParams->outputDir = destBuf;
        if (pParams->outputDir.empty()) {
            MessageDialog::Show(hDlg, L"Missing Destination", L"Please select a destination folder for extracted images.");
            return true;
        }

        wchar_t fmtBuf[32] = {};
        GetDlgItemTextW(hDlg, IDC_EXTRACT_FORMAT_COMBO, fmtBuf, 32);
        pParams->format = fmtBuf;

        wchar_t pfxBuf[128] = {};
        GetDlgItemTextW(hDlg, IDC_EXTRACT_PREFIX_EDIT, pfxBuf, 128);
        pParams->prefix = pfxBuf;
        if (pParams->prefix.empty()) pParams->prefix = L"img_p";

        pParams->pageScope = (IsDlgButtonChecked(hDlg, IDC_EXTRACT_PAGE_RANGE) == BST_CHECKED) ? 1 : 0;

        wchar_t rangeBuf[256] = {};
        GetDlgItemTextW(hDlg, IDC_EXTRACT_RANGE_EDIT, rangeBuf, 256);
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
