#pragma once
#include "core/interfaces/dom/IPage.h"
#include "core/interfaces/dom/IDocument.h"
#include <string>

namespace pdf_engine {

class AnnotationEngine {
public:
    static bool AddHighlight(core::interfaces::dom::IPage* page, double left, double top, double right, double bottom, unsigned int colorARGB);
    static bool AddTextMarkup(core::interfaces::dom::IPage* page, double x, double y, const std::wstring& text);
};

class PageOperations {
public:
    static bool RotatePage(core::interfaces::dom::IPage* page, int rotation);
    static bool DeletePage(core::interfaces::dom::IDocument* doc, int pageIndex);
    static bool ExtractPages(core::interfaces::dom::IDocument* doc, const std::wstring& outputPath, int startPage, int endPage);
};

class TextEditor {
public:
    static bool SetText(core::interfaces::dom::IPage* page, void* textObject, const std::wstring& text);
    static bool DeleteTextObject(core::interfaces::dom::IPage* page, void* textObject);
};

class ImageEditor {
public:
    static bool InsertImage(core::interfaces::dom::IPage* page, const std::wstring& imagePath, double x, double y, double width, double height);
};

}
