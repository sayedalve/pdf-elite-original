#pragma once

#include <memory>
#include <vector>
#include <d2d1_1.h>
#include "../framework/Panel.h"
#include <chrono>

namespace ui::components {

class PopupManager {
public:
    static PopupManager& Instance() {
        static PopupManager instance;
        return instance;
    }

    void ShowPopup(std::shared_ptr<framework::Panel> popup, const D2D1_RECT_F& bounds, const D2D1_RECT_F& triggerBounds) {
        if (m_activePopup != popup) {
            m_activePopup = popup;
        }
        m_triggerBounds = triggerBounds;
        m_popupBounds = bounds;
        m_pendingHide = false;
        if (m_activePopup) {
            m_activePopup->Layout(bounds);
        }
    }

    void RequestHide() {
        m_pendingHide = true;
        m_hideTime = std::chrono::steady_clock::now();
    }
    
    void CancelHide() {
        m_pendingHide = false;
    }

    void HidePopup() {
        m_activePopup.reset();
        m_pendingHide = false;
    }

    bool HasActivePopup() const {
        return m_activePopup != nullptr;
    }

    std::shared_ptr<framework::Panel> GetActivePopup() const {
        return m_activePopup;
    }

    void Render(Microsoft::WRL::ComPtr<ID2D1RenderTarget> target) {
        CheckPendingHide();
        if (m_activePopup) {
            m_activePopup->Render(target);
        }
    }

    bool HitTest(float x, float y) const {
        if (m_activePopup) {
            return m_activePopup->HitTest(x, y);
        }
        return false;
    }
    
    void OnMouseMove(float x, float y) {
        CheckPendingHide();
        if (!m_activePopup) return;
        
        bool inTrigger = (x >= m_triggerBounds.left && x <= m_triggerBounds.right && y >= m_triggerBounds.top && y <= m_triggerBounds.bottom);
        bool inPopup = m_activePopup->HitTest(x, y);
        
        if (inPopup || inTrigger) {
            CancelHide();
        } else {
            if (!m_pendingHide) {
                RequestHide();
            }
        }

        if (m_activePopup) {
            m_activePopup->OnMouseMove(x, y);
        }
    }
    
    bool OnLButtonDown(float x, float y) {
        CheckPendingHide();
        if (m_activePopup && m_activePopup->HitTest(x, y)) {
            m_activePopup->OnMouseDown(x, y);
            return true;
        }
        // If click outside popup and trigger, close it immediately
        bool inTrigger = (x >= m_triggerBounds.left && x <= m_triggerBounds.right && y >= m_triggerBounds.top && y <= m_triggerBounds.bottom);
        if (!inTrigger) {
            HidePopup();
        }
        return false;
    }
    
    void OnLButtonUp(float x, float y) {
        if (m_activePopup) {
            m_activePopup->OnMouseUp(x, y);
        }
    }

private:
    PopupManager() = default;
    
    void CheckPendingHide() {
        if (m_pendingHide && m_activePopup) {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_hideTime).count() > 500) {
                HidePopup();
            }
        }
    }
    
    std::shared_ptr<framework::Panel> m_activePopup;
    D2D1_RECT_F m_triggerBounds = {0,0,0,0};
    D2D1_RECT_F m_popupBounds = {0,0,0,0};
    bool m_pendingHide = false;
    std::chrono::steady_clock::time_point m_hideTime;
};

} // namespace ui::components


