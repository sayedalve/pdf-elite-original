#pragma once

#include <windows.h>
#include <string>

namespace ui::dialogs {

struct LinkParams {
    int pageIndex = 0;              // 0-based page index where link is placed
    double x = 50.0;                // PDF points
    double y = 50.0;
    double width = 150.0;
    double height = 30.0;
    bool isUrl = true;              // true = URL, false = internal page navigation
    std::wstring url = L"https://";
    int targetPage = 1;             // 1-based target page
    bool drawBorder = false;
    COLORREF borderColor = RGB(0, 102, 204);
    int totalPages = 1;
};

class LinkDialog {
public:
    static bool Show(HWND parentHwnd, LinkParams& params);
    static bool ShowModal(HWND parentHwnd, LinkParams& params) { return Show(parentHwnd, params); }

private:
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void OnInitDialog(HWND hDlg, LinkParams* pParams);
    static bool OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam, LinkParams* pParams);
};

} // namespace ui::dialogs
