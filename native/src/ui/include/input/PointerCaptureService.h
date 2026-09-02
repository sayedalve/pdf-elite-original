#pragma once

#include <windows.h>
#include <functional>

namespace ui::input {

class IPointerCaptureService {
public:
    virtual ~IPointerCaptureService() = default;

    virtual bool AcquireCapture(HWND hwnd, void* ownerToken) = 0;
    virtual bool ReleaseCapture(void* ownerToken) = 0;
    virtual bool HasCapture(void* ownerToken) const = 0;
    virtual bool IsAnyCaptured() const = 0;
    virtual void* GetCaptureOwner() const = 0;
    virtual void ForceRelease() = 0;
    virtual void OnCaptureLost() = 0;
    virtual void SetCaptureLostCallback(std::function<void(void* ownerToken)> callback) = 0;
};

class PointerCaptureService : public IPointerCaptureService {
public:
    PointerCaptureService() = default;
    ~PointerCaptureService() override;

    bool AcquireCapture(HWND hwnd, void* ownerToken) override;
    bool ReleaseCapture(void* ownerToken) override;
    bool HasCapture(void* ownerToken) const override;
    bool IsAnyCaptured() const override;
    void* GetCaptureOwner() const override { return m_ownerToken; }
    void ForceRelease() override;
    void OnCaptureLost() override;
    void SetCaptureLostCallback(std::function<void(void* ownerToken)> callback) override {
        m_onCaptureLostCallback = callback;
    }

private:
    HWND m_capturedHwnd = nullptr;
    void* m_ownerToken = nullptr;
    std::function<void(void* ownerToken)> m_onCaptureLostCallback;
};

// RAII Guard for safe pointer capture lifetime management
class PointerCaptureGuard {
public:
    PointerCaptureGuard(IPointerCaptureService* service, HWND hwnd, void* ownerToken);
    ~PointerCaptureGuard();

    PointerCaptureGuard(const PointerCaptureGuard&) = delete;
    PointerCaptureGuard& operator=(const PointerCaptureGuard&) = delete;

    PointerCaptureGuard(PointerCaptureGuard&& other) noexcept;
    PointerCaptureGuard& operator=(PointerCaptureGuard&& other) noexcept;

    bool IsAcquired() const { return m_acquired; }
    void Release();

private:
    IPointerCaptureService* m_service = nullptr;
    void* m_ownerToken = nullptr;
    bool m_acquired = false;
};

} // namespace ui::input
