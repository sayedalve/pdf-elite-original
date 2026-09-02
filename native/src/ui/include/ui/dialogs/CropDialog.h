#pragma once
#include "ui/dialogs/ModernDialog.h"

namespace ui::dialogs {

struct CropParams {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

class CropDialog : public ModernDialog {
public:
    static bool Show(HWND parentHwnd, CropParams& params);

private:
    CropDialog(HWND parent, CropParams& params);
    ~CropDialog() override;

    void OnLayout(float w, float h) override;
    void OnRender() override;
    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    void OnCreate() override;

    static LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND m_editL = nullptr;
    HWND m_editT = nullptr;
    HWND m_editR = nullptr;
    HWND m_editB = nullptr;
    WNDPROC m_oldEditProc = nullptr;
    CropParams& m_params;

    int m_hoverButton = -1;
    int m_downButton = -1;

    D2D1_RECT_F m_rectApply;
    D2D1_RECT_F m_rectCancel;
};

} // namespace ui::dialogs
