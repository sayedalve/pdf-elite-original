#pragma once

namespace app {

// The high-level workspace modes shown in the left vertical mode rail.
// Order here matches the visual top-to-bottom order in the rail.
enum class AppMode {
    Home,      // returns to the start/home view
    View,      // reading: zoom, fit, navigation, layout
    Comment,   // annotations: highlight / underline / strikeout / line / arrow
    Edit,      // content editing: add text / insert image
    Organize,  // page operations: rotate / delete / insert / extract
    Convert,   // format conversion (backend not implemented -> disabled)
    Tools,     // misc utilities: select; OCR / combine / compress (mostly disabled)
    Form       // form filling (backend not implemented -> disabled)
};

} // namespace app
