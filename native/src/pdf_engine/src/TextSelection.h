#pragma once
#include <vector>
#include <string>
#include "core/interfaces/dom/IDocument.h"

namespace pdf_engine {

struct TextRect {
    double left, top, right, bottom;
};

class TextSelection {
public:
    TextSelection(std::shared_ptr<core::interfaces::dom::IPage> page);
    
    std::wstring ExtractText(double left, double top, double right, double bottom);
    std::vector<TextRect> GetSelectionRects(int startCharIndex, int endCharIndex);

private:
    std::shared_ptr<core::interfaces::dom::IPage> m_page;
};

}
