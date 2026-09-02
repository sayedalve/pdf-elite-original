#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace ui::dialogs {

struct HeaderFooterParams {
    std::wstring leftHeader;
    std::wstring centerHeader;
    std::wstring rightHeader;
    std::wstring leftFooter;
    std::wstring centerFooter;
    std::wstring rightFooter;
    std::wstring fontName = L"Helvetica";
    float fontSize = 10.0f;
    COLORREF color = RGB(0, 0, 0);
    float topMargin = 36.0f;                // Margins in PDF points (36 pt = 0.5 inch)
    float bottomMargin = 36.0f;
    float leftMargin = 36.0f;
    float rightMargin = 36.0f;
    int pageScope = 0;                      // 0 = All pages, 1 = Custom range
    std::wstring pageRange = L"1";
    int startPageNum = 1;
    int currentPage = 1;
    int totalPages = 1;
};

class HeaderFooterDialog {
public:
    static bool Show(HWND parentHwnd, HeaderFooterParams& params);
    static bool ShowModal(HWND parentHwnd, HeaderFooterParams& params) { return Show(parentHwnd, params); }

private:
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void OnInitDialog(HWND hDlg, HeaderFooterParams* pParams);
    static bool OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam, HeaderFooterParams* pParams);
    static void InsertMacroToken(HWND hDlg, const std::wstring& token);
};

} // namespace ui::dialogs
