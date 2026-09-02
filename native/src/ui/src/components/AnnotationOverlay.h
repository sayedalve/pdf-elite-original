#pragma once
#include <d2d1_1.h>
#include <wrl/client.h>
#include "../../../core/interfaces/dom/IAnnotation.h"
#include "../../../core/interfaces/dom/IPage.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace ui {
namespace components {

class AnnotationOverlay {
public:
    template<typename CoordinateConverterCallback>
    static void Render(ID2D1RenderTarget* target, core::interfaces::dom::IPage* page, const CoordinateConverterCallback& pdfToCanvas) {
        if (!target || !page) return;
        
        for (const auto& annot : page->GetAnnotations()) {
            if (annot) { // FORCE OVERLAY RENDER
                RenderAnnotation(target, annot.get(), pdfToCanvas);
            }
        }
    }

private:
    template<typename CoordinateConverterCallback>
    static void RenderAnnotation(ID2D1RenderTarget* target, core::interfaces::dom::IAnnotation* annot, const CoordinateConverterCallback& pdfToCanvas) {
        auto bounds = annot->GetBounds();
        PointF tl = pdfToCanvas(PointF{bounds.left, bounds.top});
        PointF br = pdfToCanvas(PointF{bounds.right, bounds.bottom});
        
        float l = (std::min)(tl.x, br.x);
        float t = (std::min)(tl.y, br.y);
        float r = (std::max)(tl.x, br.x);
        float b = (std::max)(tl.y, br.y);
        
        int red = 255, green = 255, blue = 0, alpha = 255;
        annot->GetColor(red, green, blue, alpha);
        float opacity = annot->GetOpacity();
        float a = (alpha / 255.0f) * opacity;
        
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> strokeBrush;
        target->CreateSolidColorBrush(D2D1::ColorF(red / 255.0f, green / 255.0f, blue / 255.0f, a), &strokeBrush);
        
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> fillBrush;
        int fr = 0, fg = 0, fb = 0, fa = 0;
        if (annot->GetFillColor(fr, fg, fb, fa)) {
            target->CreateSolidColorBrush(D2D1::ColorF(fr / 255.0f, fg / 255.0f, fb / 255.0f, (fa / 255.0f) * opacity), &fillBrush);
        }

        auto type = annot->GetType();
        
        if (type == core::interfaces::dom::AnnotationType::Highlight || 
            type == core::interfaces::dom::AnnotationType::Underline ||
            type == core::interfaces::dom::AnnotationType::StrikeOut ||
            type == core::interfaces::dom::AnnotationType::Squiggly) {
            
            auto quads = annot->GetQuadPoints();
            if (!quads.empty()) {
                for (const auto& q : quads) {
                    PointF q0 = pdfToCanvas(q.p1);
                    PointF q1 = pdfToCanvas(q.p2);
                    PointF q2 = pdfToCanvas(q.p3);
                    PointF q3 = pdfToCanvas(q.p4);
                    
                    float ql = (std::min)({q0.x, q1.x, q2.x, q3.x});
                    float qr = (std::max)({q0.x, q1.x, q2.x, q3.x});
                    float qt = (std::min)({q0.y, q1.y, q2.y, q3.y});
                    float qb = (std::max)({q0.y, q1.y, q2.y, q3.y});
                    
                    if (type == core::interfaces::dom::AnnotationType::Highlight) {
                        target->FillRectangle(D2D1::RectF(ql, qt, qr, qb), strokeBrush.Get());
                    } else if (type == core::interfaces::dom::AnnotationType::Underline) {
                        target->DrawLine(D2D1::Point2F(ql, qb - 2.0f), D2D1::Point2F(qr, qb - 2.0f), strokeBrush.Get(), 2.0f);
                    } else if (type == core::interfaces::dom::AnnotationType::StrikeOut) {
                        float mid = (qt + qb) / 2.0f;
                        target->DrawLine(D2D1::Point2F(ql, mid), D2D1::Point2F(qr, mid), strokeBrush.Get(), 2.0f);
                    } else if (type == core::interfaces::dom::AnnotationType::Squiggly) {
                        target->DrawLine(D2D1::Point2F(ql, qb - 2.0f), D2D1::Point2F(qr, qb - 2.0f), strokeBrush.Get(), 2.0f);
                    }
                }
            } else {
                if (type == core::interfaces::dom::AnnotationType::Highlight) {
                    target->FillRectangle(D2D1::RectF(l, t, r, b), strokeBrush.Get());
                }
            }
        }
        else if (type == core::interfaces::dom::AnnotationType::Square) {
            if (fillBrush) target->FillRectangle(D2D1::RectF(l, t, r, b), fillBrush.Get());
            if (strokeBrush) target->DrawRectangle(D2D1::RectF(l, t, r, b), strokeBrush.Get(), annot->GetBorderWidth());
        }
        else if (type == core::interfaces::dom::AnnotationType::Circle) {
            D2D1_ELLIPSE ellipse = { D2D1::Point2F((l + r) / 2.0f, (t + b) / 2.0f), (r - l) / 2.0f, (b - t) / 2.0f };
            if (fillBrush) target->FillEllipse(ellipse, fillBrush.Get());
            if (strokeBrush) target->DrawEllipse(ellipse, strokeBrush.Get(), annot->GetBorderWidth());
        }
        else if (type == core::interfaces::dom::AnnotationType::Line) {
            core::interfaces::dom::LineGeometry lineGeom;
            if (annot->GetLineGeometry(lineGeom)) {
                PointF p1 = pdfToCanvas(lineGeom.start);
                PointF p2 = pdfToCanvas(lineGeom.end);
                if (strokeBrush) target->DrawLine(D2D1::Point2F(p1.x, p1.y), D2D1::Point2F(p2.x, p2.y), strokeBrush.Get(), annot->GetBorderWidth());
            } else {
                if (strokeBrush) target->DrawLine(D2D1::Point2F(l, t), D2D1::Point2F(r, b), strokeBrush.Get(), annot->GetBorderWidth());
            }
        }
    }
};

}
}

