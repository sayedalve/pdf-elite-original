#include "EngineEditingStubs.h"

namespace pdf_engine {

bool AnnotationEngine::AddHighlight(core::interfaces::dom::IPage* page, double left, double top, double right, double bottom, unsigned int colorARGB) { 
    (void)page; (void)left; (void)top; (void)right; (void)bottom; (void)colorARGB;
    return true; 
}
bool AnnotationEngine::AddTextMarkup(core::interfaces::dom::IPage* page, double x, double y, const std::wstring& text) { 
    (void)page; (void)x; (void)y; (void)text;
    return true; 
}

bool PageOperations::RotatePage(core::interfaces::dom::IPage* page, int rotation) { 
    (void)page; (void)rotation;
    return true; 
}
bool PageOperations::DeletePage(core::interfaces::dom::IDocument* doc, int pageIndex) { 
    (void)doc; (void)pageIndex;
    return true; 
}
bool PageOperations::ExtractPages(core::interfaces::dom::IDocument* doc, const std::wstring& outputPath, int startPage, int endPage) { 
    (void)doc; (void)outputPath; (void)startPage; (void)endPage;
    return true; 
}

bool TextEditor::SetText(core::interfaces::dom::IPage* page, void* textObject, const std::wstring& text) { 
    (void)page; (void)textObject; (void)text;
    return true; 
}
bool TextEditor::DeleteTextObject(core::interfaces::dom::IPage* page, void* textObject) { 
    (void)page; (void)textObject;
    return true; 
}

bool ImageEditor::InsertImage(core::interfaces::dom::IPage* page, const std::wstring& imagePath, double x, double y, double width, double height) { 
    (void)page; (void)imagePath; (void)x; (void)y; (void)width; (void)height;
    return true; 
}

}
