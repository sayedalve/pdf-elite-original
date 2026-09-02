#pragma once
#include "ModernDialog.h"
#include <string>

namespace ui {
namespace dialogs {

enum class MessageDialogType {
    Ok,
    YesNo,
    YesNoCancel
};

enum class MessageDialogResult {
    Ok,
    Yes,
    No,
    Cancel
};

class MessageDialog : public ModernDialog {
public:
    static MessageDialogResult Show(HWND parent, const std::wstring& title, const std::wstring& message, MessageDialogType type = MessageDialogType::Ok);

protected:
    MessageDialog(HWND parent, const std::wstring& title, const std::wstring& message, MessageDialogType type);
    
    void OnLayout(float w, float h) override;
    void OnRender() override;
    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;

private:
    std::wstring m_title;
    std::wstring m_message;
    MessageDialogType m_type;
    MessageDialogResult m_dialogResult = MessageDialogResult::Cancel;
    
    D2D1_RECT_F m_rectBtn1; // Right-most (OK or Cancel)
    D2D1_RECT_F m_rectBtn2; // Middle (No)
    D2D1_RECT_F m_rectBtn3; // Left-most (Yes)
    
    int m_hoverButton = -1;
    int m_downButton = -1;
};


class GoToPageDialog : public ModernDialog {
public:
    GoToPageDialog(HWND parent, int maxPages);
    ~GoToPageDialog();
    int GetPage() const { return m_page; }
protected:
    void OnCreate() override;
    void OnLayout(float w, float h) override;
    void OnRender() override;
    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;
private:
    int m_maxPages;
    int m_page = 0;
    HWND m_hEdit = nullptr;
    D2D1_RECT_F m_rectEdit = {};
    D2D1_RECT_F m_rectBtnOk = {};
    D2D1_RECT_F m_rectBtnCancel = {};
    bool m_btnOkHover = false;
    bool m_btnOkDown = false;
    bool m_btnCancelHover = false;
    bool m_btnCancelDown = false;
};

} // namespace dialogs
} // namespace ui
