#include "AnnotationHandlerFactory.h"
#include "TextAnnotationHandler.h"
#include "HighlightAnnotationHandler.h"
#include "InkAnnotationHandler.h"
#include "ShapeAnnotationHandler.h"
#include "FreeTextAnnotationHandler.h"

namespace ui::annotation {

std::shared_ptr<IAnnotationHandler> AnnotationHandlerFactory::CreateHandler(ToolMode mode, const AnnotationHandlerContext& context) {
    std::shared_ptr<IAnnotationHandler> handler;

    switch (mode) {
        case ToolMode::StickyNote:
            handler = std::make_shared<TextAnnotationHandler>();
            break;
        case ToolMode::Highlight:
            handler = std::make_shared<HighlightAnnotationHandler>(ToolMode::Highlight);
            break;
        case ToolMode::Underline:
            handler = std::make_shared<HighlightAnnotationHandler>(ToolMode::Underline);
            break;
        case ToolMode::Strikeout:
            handler = std::make_shared<HighlightAnnotationHandler>(ToolMode::Strikeout);
            break;
        case ToolMode::Ink:
            handler = std::make_shared<InkAnnotationHandler>();
            break;
        case ToolMode::Rectangle:
            handler = std::make_shared<ShapeAnnotationHandler>(ToolMode::Rectangle);
            break;
        case ToolMode::Ellipse:
            handler = std::make_shared<ShapeAnnotationHandler>(ToolMode::Ellipse);
            break;
        case ToolMode::Line:
            handler = std::make_shared<ShapeAnnotationHandler>(ToolMode::Line);
            break;
        case ToolMode::Arrow:
            handler = std::make_shared<ShapeAnnotationHandler>(ToolMode::Arrow);
            break;
        case ToolMode::FreeText:
            handler = std::make_shared<FreeTextAnnotationHandler>();
            break;
        default:
            return nullptr;
    }

    if (handler) {
        handler->Initialize(context);
    }
    return handler;
}

std::map<ToolMode, std::shared_ptr<IAnnotationHandler>> AnnotationHandlerFactory::CreateAllHandlers(const AnnotationHandlerContext& context) {
    std::map<ToolMode, std::shared_ptr<IAnnotationHandler>> handlers;

    ToolMode supportedTools[] = {
        ToolMode::StickyNote,
        ToolMode::Highlight,
        ToolMode::Underline,
        ToolMode::Strikeout,
        ToolMode::Ink,
        ToolMode::Rectangle,
        ToolMode::Ellipse,
        ToolMode::Line,
        ToolMode::Arrow,
        ToolMode::FreeText
    };

    for (auto tool : supportedTools) {
        handlers[tool] = CreateHandler(tool, context);
    }

    return handlers;
}

} // namespace ui::annotation
