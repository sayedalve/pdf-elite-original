#pragma once
#include "IAnnotationHandler.h"
#include <memory>
#include <map>

namespace ui::annotation {

class AnnotationHandlerFactory {
public:
    static std::shared_ptr<IAnnotationHandler> CreateHandler(ToolMode mode, const AnnotationHandlerContext& context);
    static std::map<ToolMode, std::shared_ptr<IAnnotationHandler>> CreateAllHandlers(const AnnotationHandlerContext& context);
};

} // namespace ui::annotation
