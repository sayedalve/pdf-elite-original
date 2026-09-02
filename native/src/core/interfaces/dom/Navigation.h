#pragma once
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "core/Geometry.h"

namespace core {
namespace interfaces {
namespace dom {

struct NavigationTarget {
    int pageIndex = 0;
    std::optional<double> left;
    std::optional<double> top;
    std::optional<double> zoom;
};

struct PdfBookmark {
    std::wstring title;
    std::optional<NavigationTarget> destination;
    std::vector<std::unique_ptr<PdfBookmark>> children;
    bool expanded = false;

    ~PdfBookmark() {
        if (children.empty()) return;
        
        std::vector<std::unique_ptr<PdfBookmark>> stack;
        stack.reserve(children.size());
        for (auto& child : children) {
            if (child) {
                stack.push_back(std::move(child));
            }
        }
        children.clear();
        
        while (!stack.empty()) {
            auto current = std::move(stack.back());
            stack.pop_back();
            if (!current) continue;
            
            if (!current->children.empty()) {
                stack.reserve(stack.size() + current->children.size());
                for (auto& child : current->children) {
                    if (child) {
                        stack.push_back(std::move(child));
                    }
                }
                current->children.clear();
            }
        }
    }
};

struct PdfLink {
    RectF bounds;
    std::optional<NavigationTarget> destination;
    std::optional<std::string> uri;
};

} // namespace dom
} // namespace interfaces
} // namespace core
