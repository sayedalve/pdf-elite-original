#pragma once
#include "ui/dialogs/ModernDialog.h"

namespace ui::dialogs {

struct PageSizeParams {
    int pageSizeIndex = 0; // 0=Letter, 1=A4, 2=Custom
    bool isPortrait = true;
    float width = 612.0f;
    float height = 792.0f;
};

class PageSizeDialog : public ModernDialog {
public:
    static bool Show(HWND parentHwnd, PageSizeParams& params);

private:
    PageSizeDialog(HWND parent, PageSizeParams& params);
    ~PageSizeDialog() override;

    void OnLayout(float w, float h) override;
    void OnRender() override;
    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    void OnCreate() override;

    static LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND m_editW = nullptr;
    HWND m_editH = nullptr;
    WNDPROC m_oldEditProc = nullptr;
    PageSizeParams& m_params;

    int m_hoverButton = -1;
    int m_downButton = -1;

    D2D1_RECT_F m_rectApply;
    D2D1_RECT_F m_rectCancel;
    D2D1_RECT_F m_rectSize0;
    D2D1_RECT_F m_rectSize1;
    D2D1_RECT_F m_rectSize2;
    D2D1_RECT_F m_rectPortrait;
    D2D1_RECT_F m_rectLandscape;

    void SyncToEdits();
};

} // namespace ui::dialogs
