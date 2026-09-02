#include "../../include/input/PointerCaptureService.h"

namespace ui::input {

PointerCaptureService::~PointerCaptureService() {
    ForceRelease();
}

bool PointerCaptureService::AcquireCapture(HWND hwnd, void* ownerToken) {
    if (!hwnd || !ownerToken) {
        return false;
    }

    if (m_ownerToken && m_ownerToken != ownerToken) {
        void* oldOwner = m_ownerToken;
        if (m_onCaptureLostCallback) {
            m_onCaptureLostCallback(oldOwner);
        }
    }

    if (::IsWindow(hwnd)) {
        ::SetCapture(hwnd);
    }
    m_capturedHwnd = hwnd;
    m_ownerToken = ownerToken;
    return true;
}

bool PointerCaptureService::ReleaseCapture(void* ownerToken) {
    if (m_ownerToken && m_ownerToken == ownerToken) {
        if (m_capturedHwnd && ::IsWindow(m_capturedHwnd) && ::GetCapture() == m_capturedHwnd) {
            ::ReleaseCapture();
        }
        m_capturedHwnd = nullptr;
        m_ownerToken = nullptr;
        return true;
    }
    return false;
}

bool PointerCaptureService::HasCapture(void* ownerToken) const {
    if (!m_ownerToken || m_ownerToken != ownerToken) {
        return false;
    }
    if (m_capturedHwnd && ::IsWindow(m_capturedHwnd) && ::GetCapture() != m_capturedHwnd) {
        return false;
    }
    return true;
}

bool PointerCaptureService::IsAnyCaptured() const {
    return m_ownerToken != nullptr;
}

void PointerCaptureService::ForceRelease() {
    if (m_capturedHwnd && ::IsWindow(m_capturedHwnd) && ::GetCapture() == m_capturedHwnd) {
        ::ReleaseCapture();
    }
    void* oldOwner = m_ownerToken;
    m_capturedHwnd = nullptr;
    m_ownerToken = nullptr;
    if (oldOwner && m_onCaptureLostCallback) {
        m_onCaptureLostCallback(oldOwner);
    }
}

void PointerCaptureService::OnCaptureLost() {
    // Invoked on WM_CAPTURECHANGED when Windows has already released the capture
    void* oldOwner = m_ownerToken;
    m_capturedHwnd = nullptr;
    m_ownerToken = nullptr;
    if (oldOwner && m_onCaptureLostCallback) {
        m_onCaptureLostCallback(oldOwner);
    }
}

// ----------------------------------------------------------------------------
// PointerCaptureGuard RAII Implementation
// ----------------------------------------------------------------------------

PointerCaptureGuard::PointerCaptureGuard(IPointerCaptureService* service, HWND hwnd, void* ownerToken)
    : m_service(service), m_ownerToken(ownerToken), m_acquired(false) {
    if (m_service && hwnd && m_ownerToken) {
        m_acquired = m_service->AcquireCapture(hwnd, m_ownerToken);
    }
}

PointerCaptureGuard::~PointerCaptureGuard() {
    Release();
}

PointerCaptureGuard::PointerCaptureGuard(PointerCaptureGuard&& other) noexcept
    : m_service(other.m_service), m_ownerToken(other.m_ownerToken), m_acquired(other.m_acquired) {
    other.m_service = nullptr;
    other.m_ownerToken = nullptr;
    other.m_acquired = false;
}

PointerCaptureGuard& PointerCaptureGuard::operator=(PointerCaptureGuard&& other) noexcept {
    if (this != &other) {
        Release();
        m_service = other.m_service;
        m_ownerToken = other.m_ownerToken;
        m_acquired = other.m_acquired;
        other.m_service = nullptr;
        other.m_ownerToken = nullptr;
        other.m_acquired = false;
    }
    return *this;
}

void PointerCaptureGuard::Release() {
    if (m_acquired && m_service && m_ownerToken) {
        m_service->ReleaseCapture(m_ownerToken);
        m_acquired = false;
    }
}

} // namespace ui::input
