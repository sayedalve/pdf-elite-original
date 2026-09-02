#pragma once
#include "ui/dialogs/ModernDialog.h"
#include <string>

namespace ui::dialogs {

struct ExtractParams {
    bool extractAll = true;
    std::wstring pageRange; // e.g. "1, 3, 5-7"
    std::wstring outputPath;
    bool deleteAfterExtract = false;
};

class ExtractDialog : public ModernDialog {
public:
    static bool Show(HWND parentHwnd, ExtractParams& params);

private:
    ExtractDialog(HWND parent, ExtractParams& params);
    ~ExtractDialog() override;

    void OnLayout(float w, float h) override;
    void OnRender() override;
    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    void OnCreate() override;

    static LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND m_editRange = nullptr;
    HWND m_editPath = nullptr;
    WNDPROC m_oldEditProc = nullptr;
    ExtractParams& m_params;

    int m_hoverButton = -1;
    int m_downButton = -1;

    D2D1_RECT_F m_rectExtract;
    D2D1_RECT_F m_rectCancel;
    D2D1_RECT_F m_rectBrowse;
    
    D2D1_RECT_F m_rectAll;
    D2D1_RECT_F m_rectPages;
    D2D1_RECT_F m_rectDelete;
};

} // namespace ui::dialogs
