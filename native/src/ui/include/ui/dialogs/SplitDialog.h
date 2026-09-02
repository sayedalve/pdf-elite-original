#pragma once
#include "ui/dialogs/ModernDialog.h"
#include <string>

namespace ui::dialogs {

struct SplitParams {
    int splitMethod = 0; // 0=Page count, 1=File size, 2=Bookmarks
    std::wstring methodValue = L"1";
    std::wstring outputFolder = L"C:\\";
    int maxPages = 1;
};

class SplitDialog : public ModernDialog {
public:
    static bool Show(HWND parentHwnd, SplitParams& params);

private:
    SplitDialog(HWND parent, SplitParams& params);
    ~SplitDialog() override;
    
    void OnRender() override;
    void OnLayout(float w, float h) override;
    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    void OnCreate() override;
    
    static LRESULT CALLBACK EditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    SplitParams& m_params;
    WNDPROC m_oldEditProc = nullptr;
    
    D2D1_RECT_F m_rectMethod0;
    D2D1_RECT_F m_rectMethod1;
    D2D1_RECT_F m_rectMethod2;
    D2D1_RECT_F m_rectSplit;
    D2D1_RECT_F m_rectCancel;
    D2D1_RECT_F m_rectBrowse;
    
    HWND m_editValue = nullptr;
    HWND m_editPath = nullptr;
    
    int m_hoverButton = -1;
    int m_downButton = -1;
};

} // namespace ui::dialogs
