#pragma once
#include "ui/dialogs/ModernDialog.h"
#include <string>

namespace ui::dialogs {

struct CreateBlankParams {
    int pageSizeIndex = 0;                  
    double widthPt = 612.0;                 
    double heightPt = 792.0;                
    int unitIndex = 0;                      
    bool isPortrait = true;                 
    int pageCount = 1;                      
    std::wstring outputPath;                
};

class CreateBlankDialog : public ModernDialog {
public:
    static bool Show(HWND parentHwnd, CreateBlankParams& params);

private:
    CreateBlankDialog(HWND parent, CreateBlankParams& params);
    ~CreateBlankDialog() override;

    void OnLayout(float w, float h) override;
    void OnRender() override;
    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
    void OnCreate() override;

    static LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND m_editW = nullptr;
    HWND m_editH = nullptr;
    HWND m_editCount = nullptr;
    HWND m_editPath = nullptr;
    
    WNDPROC m_oldEditProc = nullptr;
    CreateBlankParams& m_params;

    int m_hoverButton = -1;
    int m_downButton = -1;

    D2D1_RECT_F m_rectCreate;
    D2D1_RECT_F m_rectCancel;
    D2D1_RECT_F m_rectBrowse;
    
    D2D1_RECT_F m_rectPortrait;
    D2D1_RECT_F m_rectLandscape;
    
    D2D1_RECT_F m_rectPreset[5];

    void SyncToEdits();
    void SyncFromEdits();
    void SetPreset(int index);
};

} // namespace ui::dialogs
