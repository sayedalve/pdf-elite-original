#pragma once
#include <windows.h>
#include "core/interfaces/dom/IDocument.h"

namespace app {

class PrintManager {
public:
    static bool PrintDocument(HWND hwndOwner, std::shared_ptr<core::interfaces::dom::IDocument> doc);
};

}
