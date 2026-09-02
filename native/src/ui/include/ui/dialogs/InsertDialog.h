#pragma once
#include "ui/dialogs/ModernDialog.h"

namespace ui::dialogs {

struct InsertParams {
    int placeAt = 2; // 0=First, 1=Last, 2=Page
    int pageNum = 1;
    int maxPages = 1; 
    int location = 0; // 0=After, 1=Before
    int copies = 1;
};

class InsertDialog : public ModernDialog {
public:
    static bool Show(HWND parentHwnd, InsertParams& params);

private:
    InsertDialog(HWND parent, InsertParams& params);
    ~InsertDialog() override;

    void OnLayout(float w, float h) override;
    void OnRender() override;
    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    void OnCreate() override;

    static LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND m_editCopies = nullptr;
    HWND m_editPage = nullptr;
    WNDPROC m_oldEditProc = nullptr;
    InsertParams& m_params;

    int m_hoverButton = -1;
    int m_downButton = -1;

    D2D1_RECT_F m_rectInsert;
    D2D1_RECT_F m_rectCancel;
    D2D1_RECT_F m_rectLocFirst;
    D2D1_RECT_F m_rectLocLast;
    D2D1_RECT_F m_rectLocPage;
    D2D1_RECT_F m_rectAfter;
    D2D1_RECT_F m_rectBefore;
};

} // namespace ui::dialogs
