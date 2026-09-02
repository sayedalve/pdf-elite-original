#pragma once

namespace ui::input {

enum class EventResult {
    Ignored = 0,   // Event was not handled; propagate to next handler
    Handled = 1,   // Event was handled; continue normal processing
    Consumed = 2   // Event was fully consumed; halt further propagation
};

} // namespace ui::input
