#pragma once
#include "../framework/Panel.h"
#include <string>

#include <functional>
#include "../controls/IconButton.h"

namespace components {

class StatusBar : public framework::Panel {
public:
    StatusBar();
    
    void Render(ComPtr<ID2D1RenderTarget> target) override;
    void Layout(const D2D1_RECT_F& bounds) override;
    void OnMouseUp(float x, float y) override;
    
    void SetPageInfo(int currentPage, int totalPages);
    void SetZoom(float zoom);
    void SetFileName(const std::wstring& fileName);
    void SetState(const std::wstring& state);

    std::function<void(const std::wstring&)> onAction;

private:
    std::shared_ptr<controls::IconButton> AddButton(const std::wstring& text, controls::IconType icon);
    
    std::shared_ptr<controls::IconButton> m_btnDarkMode;
    std::shared_ptr<controls::IconButton> m_btnThumbnails;
    std::shared_ptr<controls::IconButton> m_btnBookmarks;
    std::shared_ptr<controls::IconButton> m_btnComments;
        std::shared_ptr<controls::IconButton> m_btnMore;
    
    std::shared_ptr<controls::IconButton> m_btnPageUp;
    std::shared_ptr<controls::IconButton> m_btnPageDown;
    std::shared_ptr<controls::IconButton> m_btnSelect;
    std::shared_ptr<controls::IconButton> m_btnFit;
    std::shared_ptr<controls::IconButton> m_btnZoomIn;
    std::shared_ptr<controls::IconButton> m_btnZoomOut;

    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
    void EnsureTextFormat();
    int m_currentPage = 0;
    int m_totalPages = 0;
    float m_zoom = 1.0f;
    std::wstring m_fileName;
    std::wstring m_state;

};

} // namespace components
