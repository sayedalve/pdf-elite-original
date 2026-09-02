#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace ui::dialogs {

struct WatermarkParams {
    std::wstring text = L"CONFIDENTIAL";
    std::wstring fontName = L"Helvetica";
    float fontSize = 48.0f;
    bool bold = false;
    bool italic = false;
    COLORREF color = RGB(192, 192, 192);
    float opacity = 0.5f;                   // 0.0 to 1.0
    float rotation = 45.0f;                 // rotation in degrees
    int positionIndex = 0;                  // 0=Center, 1=Top-Left, 2=Top-Center, 3=Top-Right, 4=Bottom-Left, 5=Bottom-Center, 6=Bottom-Right
    bool layerOver = true;                  // true = over page content, false = under content (background)
    int pageScope = 0;                      // 0 = All pages, 1 = Current page, 2 = Custom range
    std::wstring pageRange = L"1";
    int currentPage = 1;
    int totalPages = 1;
};

class WatermarkDialog {
public:
    static bool Show(HWND parentHwnd, WatermarkParams& params);
    static bool ShowModal(HWND parentHwnd, WatermarkParams& params) { return Show(parentHwnd, params); }

private:
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void OnInitDialog(HWND hDlg, WatermarkParams* pParams);
    static bool OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam, WatermarkParams* pParams);
    static void OnHScroll(HWND hDlg, WPARAM wParam, LPARAM lParam, WatermarkParams* pParams);
};

} // namespace ui::dialogs
