#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include "../../include/ui/dialogs/ModernDialog.h"

class SearchBar : public ui::dialogs::ModernDialog {
public:
    SearchBar();
    ~SearchBar();

    bool Create(HWND parentHwnd);
    void Show();
    void Hide();
    void SetBounds(int, int, int, int) {}
    bool IsVisible() const;
    bool GetWholeWord() const { return m_chkWholeWord; }
    bool GetCaseSensitive() const { return m_chkCase; }
    void SetFocus();
    
    void SetResultCount(int count, int current);
    
    void SetOnSearchCallback(std::function<void(const std::wstring&)> cb) { m_onSearch = cb; }
    void SetOnNextCallback(std::function<void()> cb) { m_onNext = cb; }
    void SetOnPrevCallback(std::function<void()> cb) { m_onPrev = cb; }
    void SetOnCloseCallback(std::function<void()> cb) { m_onClose = cb; }

protected:
    void OnCreate() override;
    void OnLayout(float w, float h) override;
    void OnRender() override;
    void OnMouseMove(float x, float y) override;
    void OnMouseDown(float x, float y) override;
    void OnMouseUp(float x, float y) override;

private:
    static LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    HWND m_editHwnd = nullptr;
    
    bool m_chkWholeWord = false;
    bool m_chkCase = false;
    bool m_chkComments = false;
    bool m_chkForms = false;
    bool m_chkBookmarks = false;
    
    D2D1_RECT_F m_rectEdit = {};
    D2D1_RECT_F m_rectWholeWord = {};
    D2D1_RECT_F m_rectCase = {};
    D2D1_RECT_F m_rectComments = {};
    D2D1_RECT_F m_rectForms = {};
    D2D1_RECT_F m_rectBookmarks = {};
    
    D2D1_RECT_F m_rectPrev = {};
    D2D1_RECT_F m_rectNext = {};
    
    bool m_btnPrevHover = false, m_btnPrevDown = false;
    bool m_btnNextHover = false, m_btnNextDown = false;
    
    std::function<void(const std::wstring&)> m_onSearch;
    std::function<void()> m_onNext;
    std::function<void()> m_onPrev;
    std::function<void()> m_onClose;
};
