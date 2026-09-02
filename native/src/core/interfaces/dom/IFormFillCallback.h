#pragma once

#include <string>

namespace core {
namespace interfaces {
namespace dom {

class IFormFillCallback {
public:
    virtual ~IFormFillCallback() = default;
    
    virtual void Invalidate(int pageIndex, double left, double top, double right, double bottom) = 0;
    virtual void SetCursor(int nCursorType) = 0;
    virtual int SetTimer(int uElapse, void(*lpTimerFunc)(int)) = 0;
    virtual void KillTimer(int nTimerID) = 0;
    virtual void SetTextFieldFocus(const std::wstring& value, bool isFocus) = 0;
};

} // namespace dom
} // namespace interfaces
} // namespace core
