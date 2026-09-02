#include <cmath>
#include "AnnotationSelectableObject.h"

namespace ui {
namespace interaction {

AnnotationSelectableObject::AnnotationSelectableObject(std::shared_ptr<core::interfaces::dom::IAnnotation> annot, int pageIndex)
    : m_annot(annot), m_pageIndex(pageIndex) {
}

std::string AnnotationSelectableObject::GetId() const {
    if (m_annot) return m_annot->GetId();
    return "";
}

int AnnotationSelectableObject::GetPageIndex() const {
    return m_pageIndex;
}

Rect AnnotationSelectableObject::GetBounds() const {
    if (m_annot) {
        auto b = m_annot->GetBounds();
        return { static_cast<double>(b.left), static_cast<double>(b.top), 
                 static_cast<double>(b.right), static_cast<double>(b.bottom) };
    }
    return {0, 0, 0, 0};
}

void AnnotationSelectableObject::SetBounds(const Rect& bounds) {
    if (m_annot) {
        m_annot->SetBounds({ static_cast<float>(bounds.left), static_cast<float>(bounds.top), 
                             static_cast<float>(bounds.right), static_cast<float>(bounds.bottom) });
    }
}

double AnnotationSelectableObject::GetRotation() const {
    return m_rotation;
}

void AnnotationSelectableObject::SetRotation(double degrees) {
    m_rotation = degrees;
}

bool AnnotationSelectableObject::HitTest(double px, double py, double tolerance) const {
    if (!m_annot) return false;
    auto type = m_annot->GetType();
    
    if (type == core::interfaces::dom::AnnotationType::Line) {
        core::interfaces::dom::LineGeometry geom;
        if (m_annot->GetLineGeometry(geom)) {
            // Distance from point (px, py) to segment (geom.start, geom.end)
            double l2 = (geom.end.x - geom.start.x)*(geom.end.x - geom.start.x) + 
                        (geom.end.y - geom.start.y)*(geom.end.y - geom.start.y);
            if (l2 == 0.0) {
                double dx = px - geom.start.x;
                double dy = py - geom.start.y;
                return std::sqrt(dx*dx + dy*dy) <= tolerance;
            }
            
            double t = std::max(0.0, std::min(1.0, ((px - geom.start.x) * (geom.end.x - geom.start.x) + 
                                                    (py - geom.start.y) * (geom.end.y - geom.start.y)) / l2));
            double projX = geom.start.x + t * (geom.end.x - geom.start.x);
            double projY = geom.start.y + t * (geom.end.y - geom.start.y);
            
            double dx = px - projX;
            double dy = py - projY;
            return std::sqrt(dx*dx + dy*dy) <= tolerance;
        }
    }

    if (type == core::interfaces::dom::AnnotationType::Circle) {
        auto b = m_annot->GetBounds();
        double cx = (b.left + b.right) / 2.0;
        double cy = (b.top + b.bottom) / 2.0;
        double rx = std::abs(b.right - b.left) / 2.0 + tolerance;
        double ry = std::abs(b.top - b.bottom) / 2.0 + tolerance;
        if (rx > 0.0 && ry > 0.0) {
            double normX = (px - cx) / rx;
            double normY = (py - cy) / ry;
            return (normX * normX + normY * normY) <= 1.0;
        }
    }

    if (type == core::interfaces::dom::AnnotationType::Highlight ||
        type == core::interfaces::dom::AnnotationType::Underline ||
        type == core::interfaces::dom::AnnotationType::StrikeOut ||
        type == core::interfaces::dom::AnnotationType::Squiggly) {
        auto quads = m_annot->GetQuadPoints();
        if (!quads.empty()) {
            for (const auto& q : quads) {
                double minX = std::min({q.p1.x, q.p2.x, q.p3.x, q.p4.x}) - tolerance;
                double maxX = std::max({q.p1.x, q.p2.x, q.p3.x, q.p4.x}) + tolerance;
                double minY = std::min({q.p1.y, q.p2.y, q.p3.y, q.p4.y}) - tolerance;
                double maxY = std::max({q.p1.y, q.p2.y, q.p3.y, q.p4.y}) + tolerance;
                if (px >= minX && px <= maxX && py >= minY && py <= maxY) {
                    return true;
                }
            }
            return false;
        }
    }

    if (type == core::interfaces::dom::AnnotationType::Ink) {
        auto inkList = m_annot->GetInkList();
        if (!inkList.empty()) {
            for (const auto& stroke : inkList) {
                if (stroke.empty()) continue;
                if (stroke.size() == 1) {
                    double dx = px - stroke[0].x;
                    double dy = py - stroke[0].y;
                    if (std::sqrt(dx * dx + dy * dy) <= tolerance) return true;
                }
                for (size_t i = 0; i + 1 < stroke.size(); ++i) {
                    double x1 = stroke[i].x, y1 = stroke[i].y;
                    double x2 = stroke[i + 1].x, y2 = stroke[i + 1].y;
                    double l2 = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
                    double dist = 0.0;
                    if (l2 == 0.0) {
                        double dx = px - x1, dy = py - y1;
                        dist = std::sqrt(dx * dx + dy * dy);
                    } else {
                        double t = std::max(0.0, std::min(1.0, ((px - x1) * (x2 - x1) + (py - y1) * (y2 - y1)) / l2));
                        double projX = x1 + t * (x2 - x1);
                        double projY = y1 + t * (y2 - y1);
                        double dx = px - projX, dy = py - projY;
                        dist = std::sqrt(dx * dx + dy * dy);
                    }
                    if (dist <= tolerance) return true;
                }
            }
            return false;
        }
    }
    
    return ISelectableObject::HitTest(px, py, tolerance);
}

} // namespace interaction
} // namespace ui
