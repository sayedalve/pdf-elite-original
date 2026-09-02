#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>

namespace ui::dialogs {

struct CombinePdfItem {
    std::wstring filePath;
    std::wstring fileName;
    int pageCount = 0;
    uint64_t fileSize = 0;
};

struct CombineParams {
    std::vector<std::wstring> sourceFiles;   // Ordered list of filepaths to combine
    std::vector<CombinePdfItem> items;       // Detailed item metadata
    std::wstring outputFile;                 // Target merged PDF filepath
    bool openAfterMerge = true;              // Automatically open combined PDF
};

class CombinePdfDialog {
public:
    static bool Show(HWND parentHwnd, CombineParams& params);
    static bool ShowModal(HWND parentHwnd, CombineParams& params) { return Show(parentHwnd, params); }

private:
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void OnInitDialog(HWND hDlg, CombineParams* pParams);
    static bool OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam, CombineParams* pParams);
    static void RefreshListView(HWND hDlg, CombineParams* pParams);
    static void AddFiles(HWND hDlg, CombineParams* pParams);
    static void RemoveSelected(HWND hDlg, CombineParams* pParams);
    static void MoveItem(HWND hDlg, CombineParams* pParams, bool moveUp);
};

} // namespace ui::dialogs
