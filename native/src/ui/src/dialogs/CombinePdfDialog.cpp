#include "ui/dialogs/CombinePdfDialog.h"
#include "ui/dialogs/MessageDialog.h"
#include "ui/resource.h"
#include <commctrl.h>
#include <commdlg.h>
#include <vector>
#include <filesystem>
#include <sstream>
#include <iomanip>

namespace ui::dialogs {

static std::wstring FormatFileSize(uint64_t bytes) {
    if (bytes == 0) return L"0 KB";
    double kb = bytes / 1024.0;
    if (kb < 1024.0) {
        std::wostringstream ss;
        ss << std::fixed << std::setprecision(1) << kb << L" KB";
        return ss.str();
    }
    double mb = kb / 1024.0;
    std::wostringstream ss;
    ss << std::fixed << std::setprecision(2) << mb << L" MB";
    return ss.str();
}

bool CombinePdfDialog::Show(HWND parentHwnd, CombineParams& params) {
    INT_PTR res = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_COMBINE_PDF_DIALOG),
        parentHwnd,
        DialogProc,
        reinterpret_cast<LPARAM>(&params)
    );
    return (res == IDOK);
}

INT_PTR CALLBACK CombinePdfDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        SetWindowLongPtrW(hDlg, DWLP_USER, lParam);
        auto* pParams = reinterpret_cast<CombineParams*>(lParam);
        if (pParams) {
            OnInitDialog(hDlg, pParams);
        }
        return TRUE;
    }
    case WM_COMMAND: {
        auto* pParams = reinterpret_cast<CombineParams*>(GetWindowLongPtrW(hDlg, DWLP_USER));
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

void CombinePdfDialog::OnInitDialog(HWND hDlg, CombineParams* pParams) {
    HWND hList = GetDlgItem(hDlg, IDC_COMBINE_FILE_LIST);
    if (hList) {
        ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        LVCOLUMNW col = {};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

        col.cx = 30;
        col.pszText = const_cast<LPWSTR>(L"#");
        col.iSubItem = 0;
        ListView_InsertColumn(hList, 0, &col);

        col.cx = 110;
        col.pszText = const_cast<LPWSTR>(L"File Name");
        col.iSubItem = 1;
        ListView_InsertColumn(hList, 1, &col);

        col.cx = 50;
        col.pszText = const_cast<LPWSTR>(L"Pages");
        col.iSubItem = 2;
        ListView_InsertColumn(hList, 2, &col);

        col.cx = 65;
        col.pszText = const_cast<LPWSTR>(L"Size");
        col.iSubItem = 3;
        ListView_InsertColumn(hList, 3, &col);

        col.cx = 190;
        col.pszText = const_cast<LPWSTR>(L"Path");
        col.iSubItem = 4;
        ListView_InsertColumn(hList, 4, &col);
    }

    // Populate initial items if any
    if (pParams->items.empty() && !pParams->sourceFiles.empty()) {
        for (const auto& path : pParams->sourceFiles) {
            CombinePdfItem itm;
            itm.filePath = path;
            itm.fileName = std::filesystem::path(path).filename().wstring();
            std::error_code ec;
            itm.fileSize = std::filesystem::file_size(path, ec);
            pParams->items.push_back(itm);
        }
    }

    RefreshListView(hDlg, pParams);

    SetDlgItemTextW(hDlg, IDC_COMBINE_OUTPUT_EDIT, pParams->outputFile.c_str());
    CheckDlgButton(hDlg, IDC_COMBINE_OPEN_CHECK, pParams->openAfterMerge ? BST_CHECKED : BST_UNCHECKED);
}

void CombinePdfDialog::RefreshListView(HWND hDlg, CombineParams* pParams) {
    HWND hList = GetDlgItem(hDlg, IDC_COMBINE_FILE_LIST);
    if (!hList) return;

    ListView_DeleteAllItems(hList);

    for (size_t i = 0; i < pParams->items.size(); ++i) {
        const auto& item = pParams->items[i];
        
        std::wstring idxStr = std::to_wstring(i + 1);
        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = static_cast<int>(i);
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(idxStr.c_str());
        ListView_InsertItem(hList, &lvi);

        ListView_SetItemText(hList, static_cast<int>(i), 1, const_cast<LPWSTR>(item.fileName.c_str()));

        std::wstring pagesStr = (item.pageCount > 0) ? std::to_wstring(item.pageCount) : L"-";
        ListView_SetItemText(hList, static_cast<int>(i), 2, const_cast<LPWSTR>(pagesStr.c_str()));

        std::wstring sizeStr = FormatFileSize(item.fileSize);
        ListView_SetItemText(hList, static_cast<int>(i), 3, const_cast<LPWSTR>(sizeStr.c_str()));

        ListView_SetItemText(hList, static_cast<int>(i), 4, const_cast<LPWSTR>(item.filePath.c_str()));
    }
}

void CombinePdfDialog::AddFiles(HWND hDlg, CombineParams* pParams) {
    const DWORD bufSize = 65536;
    std::vector<wchar_t> buffer(bufSize, 0);

    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    ofn.hwndOwner = hDlg;
    ofn.lpstrFilter = L"PDF Files (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = buffer.data();
    ofn.nMaxFile = bufSize;
    ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        const wchar_t* p = buffer.data();
        std::wstring firstString = p;
        p += firstString.length() + 1;

        if (*p == L'\0') {
            // Single file selected
            CombinePdfItem itm;
            itm.filePath = firstString;
            itm.fileName = std::filesystem::path(firstString).filename().wstring();
            std::error_code ec;
            itm.fileSize = std::filesystem::file_size(firstString, ec);
            pParams->items.push_back(itm);
        } else {
            // Multiple files selected: firstString is folder directory
            std::filesystem::path dirPath(firstString);
            while (*p != L'\0') {
                std::wstring fn = p;
                std::filesystem::path full = dirPath / fn;

                CombinePdfItem itm;
                itm.filePath = full.wstring();
                itm.fileName = fn;
                std::error_code ec;
                itm.fileSize = std::filesystem::file_size(full, ec);
                pParams->items.push_back(itm);

                p += fn.length() + 1;
            }
        }

        RefreshListView(hDlg, pParams);
    }
}

void CombinePdfDialog::RemoveSelected(HWND hDlg, CombineParams* pParams) {
    HWND hList = GetDlgItem(hDlg, IDC_COMBINE_FILE_LIST);
    if (!hList) return;

    int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (sel >= 0 && sel < static_cast<int>(pParams->items.size())) {
        pParams->items.erase(pParams->items.begin() + sel);
        RefreshListView(hDlg, pParams);
        if (!pParams->items.empty()) {
            int newSel = (sel < static_cast<int>(pParams->items.size())) ? sel : static_cast<int>(pParams->items.size() - 1);
            ListView_SetItemState(hList, newSel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        }
    }
}

void CombinePdfDialog::MoveItem(HWND hDlg, CombineParams* pParams, bool moveUp) {
    HWND hList = GetDlgItem(hDlg, IDC_COMBINE_FILE_LIST);
    if (!hList) return;

    int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (sel < 0) return;

    if (moveUp && sel > 0) {
        std::swap(pParams->items[sel], pParams->items[sel - 1]);
        RefreshListView(hDlg, pParams);
        ListView_SetItemState(hList, sel - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    } else if (!moveUp && sel >= 0 && sel < static_cast<int>(pParams->items.size() - 1)) {
        std::swap(pParams->items[sel], pParams->items[sel + 1]);
        RefreshListView(hDlg, pParams);
        ListView_SetItemState(hList, sel + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

bool CombinePdfDialog::OnCommand(HWND hDlg, WPARAM wParam, LPARAM /*lParam*/, CombineParams* pParams) {
    WORD cmdId = LOWORD(wParam);
    switch (cmdId) {
    case IDC_COMBINE_ADD_BTN:
        AddFiles(hDlg, pParams);
        return true;

    case IDC_COMBINE_REMOVE_BTN:
        RemoveSelected(hDlg, pParams);
        return true;

    case IDC_COMBINE_MOVE_UP_BTN:
        MoveItem(hDlg, pParams, true);
        return true;

    case IDC_COMBINE_MOVE_DOWN_BTN:
        MoveItem(hDlg, pParams, false);
        return true;

    case IDC_COMBINE_CLEAR_BTN:
        pParams->items.clear();
        pParams->sourceFiles.clear();
        RefreshListView(hDlg, pParams);
        return true;

    case IDC_COMBINE_OUTPUT_BROWSE: {
        wchar_t szFile[MAX_PATH] = {};
        OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
        ofn.hwndOwner = hDlg;
        ofn.lpstrFilter = L"PDF Files (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrDefExt = L"pdf";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        if (GetSaveFileNameW(&ofn)) {
            SetDlgItemTextW(hDlg, IDC_COMBINE_OUTPUT_EDIT, szFile);
        }
        return true;
    }

    case IDOK: {
        if (pParams->items.size() < 2) {
            MessageDialog::Show(hDlg, L"Insufficient Files", L"Please add at least 2 PDF files to combine.");
            return true;
        }

        wchar_t outBuf[MAX_PATH] = {};
        GetDlgItemTextW(hDlg, IDC_COMBINE_OUTPUT_EDIT, outBuf, MAX_PATH);
        pParams->outputFile = outBuf;
        if (pParams->outputFile.empty()) {
            MessageDialog::Show(hDlg, L"Missing Output Path", L"Please specify an output file destination for the merged PDF.");
            return true;
        }

        pParams->sourceFiles.clear();
        for (const auto& itm : pParams->items) {
            pParams->sourceFiles.push_back(itm.filePath);
        }

        pParams->openAfterMerge = (IsDlgButtonChecked(hDlg, IDC_COMBINE_OPEN_CHECK) == BST_CHECKED);

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
