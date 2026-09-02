#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace ui::dialogs {

struct ExtractImagesParams {
    std::wstring srcPdfPath;                // Source PDF file path
    std::wstring outputDir;                 // Target folder path
    std::wstring format = L"PNG";           // Image format: PNG, JPEG, BMP
    std::wstring prefix = L"img_p";         // Filename prefix
    int pageScope = 0;                      // 0 = All pages, 1 = Custom range
    std::wstring pageRange = L"1";
    int currentPage = 1;
    int totalPages = 1;
};

class ExtractImagesDialog {
public:
    static bool Show(HWND parentHwnd, ExtractImagesParams& params);
    static bool ShowModal(HWND parentHwnd, ExtractImagesParams& params) { return Show(parentHwnd, params); }

private:
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void OnInitDialog(HWND hDlg, ExtractImagesParams* pParams);
    static bool OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam, ExtractImagesParams* pParams);
};

} // namespace ui::dialogs
