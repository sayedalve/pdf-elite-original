#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace ui::dialogs {

struct BackgroundParams {
    bool isColor = true;                    // true = solid color, false = image file
    COLORREF color = RGB(245, 245, 245);
    double opacity = 1.0;                   // 0.0 to 1.0
    std::wstring imagePath;
    int pageScope = 0;                      // 0 = All pages, 1 = Current page, 2 = Custom range
    std::wstring pageRange = L"1";
    int currentPage = 1;                    // 1-based current page
    int totalPages = 1;                     // Total document pages
};

class BackgroundDialog {
public:
    static bool Show(HWND parentHwnd, BackgroundParams& params);
    static bool ShowModal(HWND parentHwnd, BackgroundParams& params) { return Show(parentHwnd, params); }

private:
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void OnInitDialog(HWND hDlg, BackgroundParams* pParams);
    static bool OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam, BackgroundParams* pParams);
    static void OnHScroll(HWND hDlg, WPARAM wParam, LPARAM lParam, BackgroundParams* pParams);
    static void UpdatePreview(HWND hDlg, BackgroundParams* pParams);
};

} // namespace ui::dialogs
