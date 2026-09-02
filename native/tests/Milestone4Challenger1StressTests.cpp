#include "TestFramework.h"
#include "../src/core/CoordinateConverter.h"
#include "../src/core/Geometry.h"
#include "../src/ui/include/selection/TransformHandles.h"
#include "../src/ui/include/selection/SelectionModel.h"
#include "../src/ui/include/selection/CursorResolver.h"
#include "../src/core/interfaces/dom/ITextPage.h"
#include <cmath>
#include <vector>
#include <random>
#include <iostream>
#include <algorithm>
#include <string>
#include <iomanip>

using namespace ui::selection;

// ============================================================================
// MOCK TEXT PAGE IMPLEMENTATION FOR PURE UNIT STRESS TESTING
// ============================================================================
class MockTextPage : public core::interfaces::dom::ITextPage {
public:
    explicit MockTextPage(std::wstring text) : m_text(std::move(text)) {
        m_charCount = static_cast<int>(m_text.length());
        m_rects.resize(m_charCount);
        float x = 50.0f;
        float y = 700.0f;
        for (int i = 0; i < m_charCount; ++i) {
            if (m_text[i] == L'\n') {
                x = 50.0f;
                y -= 20.0f;
                m_rects[i] = { x, y, x, y + 15.0f };
            } else {
                m_rects[i] = { x, y, x + 8.0f, y + 15.0f };
                x += 8.0f;
            }
        }
    }

    int GetCharCount() const override { return m_charCount; }
    
    std::wstring GetText(int startIndex, int count) const override {
        if (startIndex < 0 || startIndex >= m_charCount || count <= 0) return L"";
        int len = (std::min)(count, m_charCount - startIndex);
        return m_text.substr(startIndex, len);
    }

    RectF GetCharBox(int charIndex) const override {
        if (charIndex >= 0 && charIndex < m_charCount) {
            return m_rects[charIndex];
        }
        return { 0.0f, 0.0f, 0.0f, 0.0f };
    }

    int GetCharIndexAtPos(double x, double y, double xTolerance, double yTolerance) const override {
        for (int i = 0; i < m_charCount; ++i) {
            const auto& r = m_rects[i];
            if (x >= r.left - xTolerance && x <= r.right + xTolerance &&
                y >= r.bottom - yTolerance && y <= r.top + yTolerance) {
                return i;
            }
        }
        return -1;
    }

    std::vector<RectF> GetRects(int startIndex, int count) const override {
        if (startIndex < 0 || startIndex >= m_charCount || count <= 0) return {};
        int len = (std::min)(count, m_charCount - startIndex);
        std::vector<RectF> result;
        result.reserve(len);
        for (int i = 0; i < len; ++i) {
            result.push_back(m_rects[startIndex + i]);
        }
        return result;
    }

private:
    std::wstring m_text;
    int m_charCount = 0;
    std::vector<RectF> m_rects;
};

// ============================================================================
// SUITE 1: COORDINATE TRANSFORMATION PRECISION & EXTREME BOUNDARY CONDITIONS
// ============================================================================

TEST(Challenger1_CoordinateConverter_CanvasToDip_RoundTrip_Stress) {
    // Stress test canvasToDip and dipToCanvas invariant:
    // canvasPt = dipPt - m_bounds.origin + scroll
    // dipPt = canvasPt - scroll + m_bounds.origin
    const std::vector<float> scales = { 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 2.5f, 3.0f, 4.0f };
    const std::vector<float> boundsOrigins = { -1000.0f, -50.0f, 0.0f, 100.0f, 500.0f, 1920.0f };
    const std::vector<float> scrolls = { -5000.0f, -1000.0f, 0.0f, 250.0f, 5000.0f };

    float maxObservedError = 0.0f;

    for (float dpiScale : scales) {
        for (float originX : boundsOrigins) {
            for (float originY : boundsOrigins) {
                for (float scrollX : scrolls) {
                    for (float scrollY : scrolls) {
                        RectF bounds = { originX, originY, originX + 1920.0f, originY + 1080.0f };
                        
                        auto dipToCanvas = [&](const PointF& dipPt) -> PointF {
                            return PointF{ dipPt.x - bounds.left + scrollX, dipPt.y - bounds.top + scrollY };
                        };
                        auto canvasToDip = [&](const PointF& canvasPt) -> PointF {
                            return PointF{ canvasPt.x - scrollX + bounds.left, canvasPt.y - scrollY + bounds.top };
                        };

                        PointF testPts[] = {
                            { 0.0f, 0.0f },
                            { 500.0f, 300.0f },
                            { -1234.56f, 9876.54f },
                            { 20000.0f, -20000.0f }
                        };

                        for (const auto& pt : testPts) {
                            PointF canvas = dipToCanvas(pt);
                            PointF recoveredDip = canvasToDip(canvas);
                            float errX = std::abs(pt.x - recoveredDip.x);
                            float errY = std::abs(pt.y - recoveredDip.y);
                            if (errX > maxObservedError) maxObservedError = errX;
                            if (errY > maxObservedError) maxObservedError = errY;

                            EXPECT_NEAR(pt.x, recoveredDip.x, 1e-3f);
                            EXPECT_NEAR(pt.y, recoveredDip.y, 1e-3f);

                            PointF dip = canvasToDip(pt);
                            PointF recoveredCanvas = dipToCanvas(dip);
                            EXPECT_NEAR(pt.x, recoveredCanvas.x, 1e-3f);
                            EXPECT_NEAR(pt.y, recoveredCanvas.y, 1e-3f);

                            // Also test ScreenToLogical / LogicalToScreen round-trip with dpiScale
                            PointF screenPt = CoordinateConverter::LogicalToScreen(pt.x, pt.y, dpiScale);
                            PointF recoveredLog = CoordinateConverter::ScreenToLogical(screenPt.x, screenPt.y, dpiScale);
                            EXPECT_NEAR(pt.x, recoveredLog.x, 1e-3f);
                            EXPECT_NEAR(pt.y, recoveredLog.y, 1e-3f);
                        }
                    }
                }
            }
        }
    }
    std::cout << "[CanvasToDip Stress] Max observed error: " << maxObservedError << " DIPs. ";
}

TEST(Challenger1_CoordinateConverter_CanvasToPdf_ExtremeZooms_0_01x_to_64x) {
    const std::vector<double> zooms = {
        0.01, 0.02, 0.05, 0.1, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0
    };
    
    struct PageTestDef {
        double w, h;
        const char* name;
    };
    const std::vector<PageTestDef> pageDefs = {
        { 612.0, 792.0, "US Letter" },
        { 595.28, 841.89, "A4" },
        { 10.0, 10.0, "Micro 10x10" },
        { 14400.0, 14400.0, "Max Spec 200x200 inch" },
        { 50.0, 5000.0, "Slender Tall 1:100" },
        { 5000.0, 50.0, "Slender Wide 100:1" }
    };

    const std::vector<float> viewportWidths = { 400.0f, 800.0f, 1920.0f, 4000.0f };

    float maxObservedPdfError = 0.0f;

    for (double zoom : zooms) {
        for (const auto& pageDef : pageDefs) {
            for (float vpWidth : viewportWidths) {
                ContinuousPageLayout layout;
                layout.index = 0;
                layout.yOffset = 50.0f;
                layout.width = static_cast<float>(pageDef.w) * static_cast<float>(zoom);
                layout.height = static_cast<float>(pageDef.h) * static_cast<float>(zoom);

                auto canvasToPdf = [&](const PointF& canvasPt, int& outPageIndex) -> PointF {
                    outPageIndex = -1;
                    float centerOffset = (std::max)(0.0f, (vpWidth - layout.width) / 2.0f);
                    if (canvasPt.y >= layout.yOffset && canvasPt.y <= layout.yOffset + layout.height &&
                        canvasPt.x >= centerOffset && canvasPt.x <= centerOffset + layout.width) {
                        outPageIndex = layout.index;
                        float pdfX = (canvasPt.x - centerOffset) / static_cast<float>(zoom);
                        float pdfY = (layout.height - (canvasPt.y - layout.yOffset)) / static_cast<float>(zoom);
                        return PointF{ pdfX, pdfY };
                    }
                    return PointF{ 0.0f, 0.0f };
                };

                auto pdfToCanvas = [&](int pageIndex, const PointF& pdfPt) -> PointF {
                    if (pageIndex == layout.index) {
                        float centerOffset = (std::max)(0.0f, (vpWidth - layout.width) / 2.0f);
                        float canvasX = centerOffset + (pdfPt.x * static_cast<float>(zoom));
                        float canvasY = layout.yOffset + (layout.height - (pdfPt.y * static_cast<float>(zoom)));
                        return PointF{ canvasX, canvasY };
                    }
                    return PointF{ 0.0f, 0.0f };
                };

                // Sample points across the page
                PointF testPoints[] = {
                    { 0.0f, 0.0f }, // Bottom-left origin in PDF
                    { static_cast<float>(pageDef.w), 0.0f }, // Bottom-right
                    { 0.0f, static_cast<float>(pageDef.h) }, // Top-left
                    { static_cast<float>(pageDef.w), static_cast<float>(pageDef.h) }, // Top-right
                    { static_cast<float>(pageDef.w * 0.5), static_cast<float>(pageDef.h * 0.5) }, // Center
                    { static_cast<float>(pageDef.w * 0.123), static_cast<float>(pageDef.h * 0.789) }
                };

                for (const auto& pdfPt : testPoints) {
                    PointF canvasPt = pdfToCanvas(0, pdfPt);
                    int recoveredPage = -1;
                    PointF recoveredPdf = canvasToPdf(canvasPt, recoveredPage);

                    EXPECT_EQ(recoveredPage, 0);
                    float diffX = std::abs(pdfPt.x - recoveredPdf.x);
                    float diffY = std::abs(pdfPt.y - recoveredPdf.y);
                    float err = std::max(diffX, diffY);
                    if (err > maxObservedPdfError) maxObservedPdfError = err;

                    // Standard IEEE 754 precision tolerance: proportional to dimensions
                    float tolerance = static_cast<float>(std::max(pageDef.w, pageDef.h) * 1e-4);
                    if (tolerance < 0.01f) tolerance = 0.01f;
                    EXPECT_NEAR(pdfPt.x, recoveredPdf.x, tolerance);
                    EXPECT_NEAR(pdfPt.y, recoveredPdf.y, tolerance);
                }

                // Test Out-of-Bounds rejection
                float centerOffset = (std::max)(0.0f, (vpWidth - layout.width) / 2.0f);
                int outPage = 0;
                // Left of page
                canvasToPdf(PointF{ centerOffset - 1.0f, layout.yOffset + 10.0f }, outPage);
                EXPECT_EQ(outPage, -1);
                // Right of page
                canvasToPdf(PointF{ centerOffset + layout.width + 1.0f, layout.yOffset + 10.0f }, outPage);
                EXPECT_EQ(outPage, -1);
                // Above page
                canvasToPdf(PointF{ centerOffset + 10.0f, layout.yOffset - 1.0f }, outPage);
                EXPECT_EQ(outPage, -1);
                // Below page
                canvasToPdf(PointF{ centerOffset + 10.0f, layout.yOffset + layout.height + 1.0f }, outPage);
                EXPECT_EQ(outPage, -1);
            }
        }
    }
    std::cout << "[CanvasToPdf Extreme Zooms] Max observed error: " << maxObservedPdfError << " PDF pts. ";
}

TEST(Challenger1_CoordinateConverter_AllRotations_0_90_180_270_Fidelity) {
    const std::vector<int> rotations = { 0, 90, 180, 270 };
    const std::vector<double> zooms = { 0.25, 0.5, 1.0, 2.0, 4.0 };
    CoordinateConverter::PageContext page{ 612.0, 792.0, 0 };
    CoordinateConverter::ViewContext view{ 1.0, 0.0, 0.0, 0.0, 0.0 };

    for (int rot : rotations) {
        page.rotation = rot;
        for (double z : zooms) {
            view.zoom = z;
            for (double px = 0.0; px <= 612.0; px += 102.0) {
                for (double py = 0.0; py <= 792.0; py += 132.0) {
                    PointF screen = CoordinateConverter::PdfToScreen(page, view, px, py);
                    PointF recovered = CoordinateConverter::ScreenToPdf(page, view, screen.x, screen.y);
                    EXPECT_NEAR(static_cast<float>(px), recovered.x, 1e-3f);
                    EXPECT_NEAR(static_cast<float>(py), recovered.y, 1e-3f);

                    PointF norm = CoordinateConverter::PdfToNormalized(page, px, py);
                    EXPECT_GE(norm.x, -1e-5f);
                    EXPECT_LE(norm.x, 1.0f + 1e-5f);
                    EXPECT_GE(norm.y, -1e-5f);
                    EXPECT_LE(norm.y, 1.0f + 1e-5f);

                    PointF recoveredFromNorm = CoordinateConverter::NormalizedToPdf(page, norm.x, norm.y);
                    EXPECT_NEAR(static_cast<float>(px), recoveredFromNorm.x, 1e-3f);
                    EXPECT_NEAR(static_cast<float>(py), recoveredFromNorm.y, 1e-3f);
                }
            }
        }
    }
}

TEST(Challenger1_CoordinateConverter_MatrixInvertibility_Invariants) {
    CoordinateConverter::PageContext page{ 612.0, 792.0, 0 };
    const std::vector<int> rotations = { 0, 90, 180, 270 };

    for (int rot : rotations) {
        page.rotation = rot;
        Matrix3x2F mPdfToNorm = CoordinateConverter::GetPdfToNormalizedMatrix(page);
        Matrix3x2F mNormToPdf = CoordinateConverter::GetNormalizedToPdfMatrix(page);

        // Product should be identity
        float a = mPdfToNorm.a * mNormToPdf.a + mPdfToNorm.b * mNormToPdf.c;
        float b = mPdfToNorm.a * mNormToPdf.b + mPdfToNorm.b * mNormToPdf.d;
        float c = mPdfToNorm.c * mNormToPdf.a + mPdfToNorm.d * mNormToPdf.c;
        float d = mPdfToNorm.c * mNormToPdf.b + mPdfToNorm.d * mNormToPdf.d;

        EXPECT_NEAR(a, 1.0f, 1e-5f);
        EXPECT_NEAR(b, 0.0f, 1e-5f);
        EXPECT_NEAR(c, 0.0f, 1e-5f);
        EXPECT_NEAR(d, 1.0f, 1e-5f);
    }
}

TEST(Challenger1_CoordinateConverter_ContinuousLayout_100Pages_Stacking) {
    std::vector<ContinuousPageLayout> layouts;
    float currentY = 10.0f;
    for (int i = 0; i < 100; ++i) {
        ContinuousPageLayout l;
        l.index = i;
        l.width = 612.0f;
        l.height = (i % 2 == 0) ? 792.0f : 841.89f;
        l.yOffset = currentY;
        currentY += l.height + 15.0f; // 15px gap
        layouts.push_back(l);
    }

    // Verify finding page index at various Y coordinates
    for (int i = 0; i < 100; ++i) {
        const auto& l = layouts[i];
        // Exact top
        int idxTop = CoordinateConverter::FindPageIndexAtViewportY(l.yOffset, layouts);
        EXPECT_EQ(idxTop, i);
        // Middle of page
        int idxMid = CoordinateConverter::FindPageIndexAtViewportY(l.yOffset + l.height * 0.5f, layouts);
        EXPECT_EQ(idxMid, i);
        // Near bottom of page (within half-open interval [yOffset, yOffset + height))
        int idxNearBot = CoordinateConverter::FindPageIndexAtViewportY(l.yOffset + l.height - 0.5f, layouts);
        EXPECT_EQ(idxNearBot, i);

        // MultiPageOffset helper
        PointF offset = CoordinateConverter::MultiPageOffset(i, layouts);
        EXPECT_NEAR(offset.y, l.yOffset, 1e-4f);
    }

    // Viewport clamping behavior for continuous scrolling
    int clampedTop = CoordinateConverter::FindPageIndexAtViewportY(0.0f, layouts);
    EXPECT_EQ(clampedTop, 0);

    int clampedBot = CoordinateConverter::FindPageIndexAtViewportY(currentY + 1000.0f, layouts);
    EXPECT_EQ(clampedBot, 99);
}

// ============================================================================
// SUITE 2: 8-WAY SELECTION HANDLE HIT-TESTING & ROTATION MATRIX MATH
// ============================================================================

TEST(Challenger1_TransformHandles_AllRotations_HitTesting_Exhaustive) {
    TransformHandles handles;
    RectF viewBounds = { 100.0f, 100.0f, 300.0f, 250.0f }; // Width 200, Height 150
    PointF center = { 200.0f, 175.0f };

    const std::vector<float> testAngles = {
        0.0f, 15.0f, 30.0f, 45.0f, 60.0f, 75.0f, 90.0f, 120.0f, 135.0f, 150.0f,
        180.0f, 210.0f, 225.0f, 270.0f, 300.0f, 315.0f, 345.0f, 359.9f, -45.0f, -90.0f, 37.42f
    };

    for (float angle : testAngles) {
        auto handleDescs = handles.ComputeHandles(viewBounds, angle, 8.0f, 24.0f, true);
        EXPECT_EQ(handleDescs.size(), 9); // 8 handles + 1 rotation stem

        for (const auto& desc : handleDescs) {
            // 1. Direct hit at center of handle
            HandleType hit = handles.HitTest(desc.position, viewBounds, angle, 8.0f, 5.0f, true);
            EXPECT_EQ(static_cast<int>(hit), static_cast<int>(desc.type));

            // 2. Hit near handle center within tolerance radius (r = 4 + 5 = 9 DIP)
            // Test 8 points at distance 6.0 DIP around handle
            for (float theta = 0.0f; theta < 360.0f; theta += 45.0f) {
                float rad = theta * 3.14159265f / 180.0f;
                PointF nearPt = { desc.position.x + 6.0f * std::cos(rad), desc.position.y + 6.0f * std::sin(rad) };
                HandleType nearHit = handles.HitTest(nearPt, viewBounds, angle, 8.0f, 5.0f, true);
                EXPECT_EQ(static_cast<int>(nearHit), static_cast<int>(desc.type));
            }

            // 3. Point far outside tolerance radius (r = 20.0 DIP away from handle in outward normal direction)
            PointF outwardVec = TransformHandles::GetHandleDirectionVector(desc.type, angle);
            PointF farPt = { desc.position.x + outwardVec.x * 20.0f, desc.position.y + outwardVec.y * 20.0f };
            HandleType farHit = handles.HitTest(farPt, viewBounds, angle, 8.0f, 5.0f, true);
            EXPECT_NE(static_cast<int>(farHit), static_cast<int>(desc.type));
        }

        // Test Body hit at rotated center
        HandleType bodyHit = handles.HitTest(center, viewBounds, angle, 8.0f, 5.0f, true);
        EXPECT_EQ(static_cast<int>(bodyHit), static_cast<int>(HandleType::Body));

        // Test Point far away from entire object
        HandleType outsideHit = handles.HitTest(PointF{ 10.0f, 10.0f }, viewBounds, angle, 8.0f, 5.0f, true);
        EXPECT_EQ(static_cast<int>(outsideHit), static_cast<int>(HandleType::None));
    }
}

TEST(Challenger1_TransformHandles_HitTestInverseMatrix_Equivalence_Stress) {
    TransformHandles handles;
    RectF localBounds = { 100.0f, 100.0f, 300.0f, 250.0f };
    PointF center = { 200.0f, 175.0f };

    const std::vector<float> testAngles = { 0.0f, 25.0f, 45.0f, 90.0f, 135.0f, 180.0f, 270.0f, 315.0f };

    for (float angle : testAngles) {
        float rad = angle * 3.14159265f / 180.0f;
        float cosA = std::cos(rad);
        float sinA = std::sin(rad);

        // Transform matrix representing rotation around center:
        // T(center) * R(angle) * T(-center)
        Matrix3x2F mat;
        mat.a = cosA;
        mat.b = sinA;
        mat.c = -sinA;
        mat.d = cosA;
        mat.e = center.x * (1.0f - cosA) + center.y * sinA;
        mat.f = center.y * (1.0f - cosA) - center.x * sinA;

        // Test handles
        auto handleDescs = handles.ComputeHandles(localBounds, angle, 8.0f, 24.0f, true);
        for (const auto& desc : handleDescs) {
            HandleType h1 = handles.HitTest(desc.position, localBounds, angle, 8.0f, 5.0f, true);
            HandleType h2 = handles.HitTestInverseMatrix(desc.position, localBounds, mat, 8.0f, 5.0f, true);
            EXPECT_EQ(static_cast<int>(h1), static_cast<int>(desc.type));
            EXPECT_EQ(static_cast<int>(h2), static_cast<int>(desc.type));
        }

        // Test body center
        HandleType b1 = handles.HitTest(center, localBounds, angle, 8.0f, 5.0f, true);
        HandleType b2 = handles.HitTestInverseMatrix(center, localBounds, mat, 8.0f, 5.0f, true);
        EXPECT_EQ(static_cast<int>(b1), static_cast<int>(HandleType::Body));
        EXPECT_EQ(static_cast<int>(b2), static_cast<int>(HandleType::Body));

        // Test outside point
        PointF outPt = { 10.0f, 10.0f };
        HandleType o1 = handles.HitTest(outPt, localBounds, angle, 8.0f, 5.0f, true);
        HandleType o2 = handles.HitTestInverseMatrix(outPt, localBounds, mat, 8.0f, 5.0f, true);
        EXPECT_EQ(static_cast<int>(o1), static_cast<int>(HandleType::None));
        EXPECT_EQ(static_cast<int>(o2), static_cast<int>(HandleType::None));
    }
}

TEST(Challenger1_TransformHandles_MatrixInversion_Robustness_Singularities) {
    // 1. Singular matrix (det = 0)
    Matrix3x2F singular1{ 0.0f, 0.0f, 0.0f, 0.0f, 10.0f, 20.0f };
    Matrix3x2F inv1 = TransformHandles::InvertMatrix(singular1);
    EXPECT_NEAR(inv1.a, 1.0f, 1e-5f);
    EXPECT_NEAR(inv1.d, 1.0f, 1e-5f);
    EXPECT_NEAR(inv1.b, 0.0f, 1e-5f);
    EXPECT_NEAR(inv1.c, 0.0f, 1e-5f);

    // 2. Collinear degenerate matrix (det = 0)
    Matrix3x2F singular2{ 2.0f, 4.0f, 1.0f, 2.0f, 5.0f, 5.0f };
    Matrix3x2F inv2 = TransformHandles::InvertMatrix(singular2);
    EXPECT_NEAR(inv2.a, 1.0f, 1e-5f);
    EXPECT_NEAR(inv2.d, 1.0f, 1e-5f);

    // 3. Valid affine transformation matrix: scale + rotate + translate
    float rad = 30.0f * 3.14159265f / 180.0f;
    Matrix3x2F validMat;
    validMat.a = 2.0f * std::cos(rad);
    validMat.b = 2.0f * std::sin(rad);
    validMat.c = -3.0f * std::sin(rad);
    validMat.d = 3.0f * std::cos(rad);
    validMat.e = 100.0f;
    validMat.f = 200.0f;

    Matrix3x2F validInv = TransformHandles::InvertMatrix(validMat);

    // Test point transformation and inverse recovery
    PointF origPt = { 45.67f, 89.12f };
    PointF transformed = TransformHandles::TransformPoint(origPt, validMat);
    PointF recovered = TransformHandles::TransformPoint(transformed, validInv);

    EXPECT_NEAR(origPt.x, recovered.x, 1e-3f);
    EXPECT_NEAR(origPt.y, recovered.y, 1e-3f);
}

TEST(Challenger1_TransformHandles_SnapAngle15_ExactTransitions) {
    // 0 to 15 deg transition at 7.5 deg
    EXPECT_NEAR(TransformHandles::SnapAngle15(0.0f), 0.0f, 1e-5f);
    EXPECT_NEAR(TransformHandles::SnapAngle15(7.49f), 0.0f, 1e-5f);
    EXPECT_NEAR(TransformHandles::SnapAngle15(7.50f), 15.0f, 1e-5f);
    EXPECT_NEAR(TransformHandles::SnapAngle15(14.99f), 15.0f, 1e-5f);
    EXPECT_NEAR(TransformHandles::SnapAngle15(15.00f), 15.0f, 1e-5f);
    EXPECT_NEAR(TransformHandles::SnapAngle15(22.49f), 15.0f, 1e-5f);
    EXPECT_NEAR(TransformHandles::SnapAngle15(22.50f), 30.0f, 1e-5f);

    // Near 360 deg boundary
    EXPECT_NEAR(TransformHandles::SnapAngle15(352.49f), 345.0f, 1e-5f);
    EXPECT_NEAR(TransformHandles::SnapAngle15(352.50f), 0.0f, 1e-5f);
    EXPECT_NEAR(TransformHandles::SnapAngle15(359.99f), 0.0f, 1e-5f);
    EXPECT_NEAR(TransformHandles::SnapAngle15(360.00f), 0.0f, 1e-5f);

    // Negative angles
    EXPECT_NEAR(TransformHandles::SnapAngle15(-7.49f), 0.0f, 1e-5f);
    EXPECT_NEAR(TransformHandles::SnapAngle15(-7.50f), 0.0f, 1e-5f);
    EXPECT_NEAR(TransformHandles::SnapAngle15(-7.51f), 345.0f, 1e-5f);
    EXPECT_NEAR(TransformHandles::SnapAngle15(-90.0f), 270.0f, 1e-5f);
}

TEST(Challenger1_TransformHandles_ComputeRotationAngle_FourQuadrants) {
    PointF center = { 100.0f, 100.0f };
    // Top (0 deg)
    EXPECT_NEAR(TransformHandles::ComputeRotationAngle(center, PointF{ 100.0f, 50.0f }), 0.0f, 1e-4f);
    // Right (90 deg)
    EXPECT_NEAR(TransformHandles::ComputeRotationAngle(center, PointF{ 150.0f, 100.0f }), 90.0f, 1e-4f);
    // Bottom (180 deg)
    EXPECT_NEAR(TransformHandles::ComputeRotationAngle(center, PointF{ 100.0f, 150.0f }), 180.0f, 1e-4f);
    // Left (270 deg)
    EXPECT_NEAR(TransformHandles::ComputeRotationAngle(center, PointF{ 50.0f, 100.0f }), 270.0f, 1e-4f);
    // Top-Right (45 deg)
    EXPECT_NEAR(TransformHandles::ComputeRotationAngle(center, PointF{ 150.0f, 50.0f }), 45.0f, 1e-4f);
    // Bottom-Right (135 deg)
    EXPECT_NEAR(TransformHandles::ComputeRotationAngle(center, PointF{ 150.0f, 150.0f }), 135.0f, 1e-4f);
    // Bottom-Left (225 deg)
    EXPECT_NEAR(TransformHandles::ComputeRotationAngle(center, PointF{ 50.0f, 150.0f }), 225.0f, 1e-4f);
    // Top-Left (315 deg)
    EXPECT_NEAR(TransformHandles::ComputeRotationAngle(center, PointF{ 50.0f, 50.0f }), 315.0f, 1e-4f);
}

// ============================================================================
// SUITE 3: MULTI-CLICK TEXT SELECTION & BOUNDARY CONDITIONS
// ============================================================================

TEST(Challenger1_SelectionModel_FindWordBoundaries_Comprehensive) {
    // 1. Empty string
    auto [e1, e2] = SelectionModel::FindWordBoundaries(L"", 0);
    EXPECT_EQ(e1, 0);
    EXPECT_EQ(e2, 0);

    // 2. Out of bounds index
    std::wstring text = L"Hello, World!";
    auto [o1, o2] = SelectionModel::FindWordBoundaries(text, -1);
    EXPECT_EQ(o1, 0);
    EXPECT_EQ(o2, 0);
    auto [o3, o4] = SelectionModel::FindWordBoundaries(text, 100);
    EXPECT_EQ(o3, 0);
    EXPECT_EQ(o4, 0);

    // 3. Normal word "Hello" [0..4]
    for (int i = 0; i <= 4; ++i) {
        auto [s, e] = SelectionModel::FindWordBoundaries(text, i);
        EXPECT_EQ(s, 0);
        EXPECT_EQ(e, 4);
    }

    // 4. Comma punctuation [5]
    auto [c1, c2] = SelectionModel::FindWordBoundaries(text, 5);
    EXPECT_EQ(c1, 5);
    EXPECT_EQ(c2, 5);

    // 5. Space between words [6]
    auto [sp1, sp2] = SelectionModel::FindWordBoundaries(text, 6);
    EXPECT_EQ(sp1, 6);
    EXPECT_EQ(sp2, 6);

    // 6. Word "World" [7..11]
    for (int i = 7; i <= 11; ++i) {
        auto [s, e] = SelectionModel::FindWordBoundaries(text, i);
        EXPECT_EQ(s, 7);
        EXPECT_EQ(e, 11);
    }

    // 7. Underscored identifier "user_id_alpha_1"
    std::wstring ident = L"const user_id_alpha_1 = 42;";
    for (int i = 6; i <= 20; ++i) {
        auto [s, e] = SelectionModel::FindWordBoundaries(ident, i);
        EXPECT_EQ(s, 6);
        EXPECT_EQ(e, 20);
    }

    // 8. Consecutive spaces "word    word"
    std::wstring spaces = L"alpha   beta";
    for (int i = 5; i <= 7; ++i) {
        auto [s, e] = SelectionModel::FindWordBoundaries(spaces, i);
        EXPECT_EQ(s, 5);
        EXPECT_EQ(e, 7);
    }

    // 9. Multilingual / Unicode word boundaries
    std::wstring cyrillic = L"Привет, мир!";
    for (int i = 0; i <= 5; ++i) {
        auto [s, e] = SelectionModel::FindWordBoundaries(cyrillic, i);
        EXPECT_EQ(s, 0);
        EXPECT_EQ(e, 5);
    }
    for (int i = 8; i <= 10; ++i) {
        auto [s, e] = SelectionModel::FindWordBoundaries(cyrillic, i);
        EXPECT_EQ(s, 8);
        EXPECT_EQ(e, 10);
    }

    // 10. French accented characters "Élégant café"
    std::wstring french = L"Élégant café";
    for (int i = 0; i <= 6; ++i) {
        auto [s, e] = SelectionModel::FindWordBoundaries(french, i);
        EXPECT_EQ(s, 0);
        EXPECT_EQ(e, 6);
    }
}

TEST(Challenger1_SelectionModel_FindLineBoundaries_Comprehensive) {
    // 1. Empty string
    auto [e1, e2] = SelectionModel::FindLineBoundaries(L"", 0);
    EXPECT_EQ(e1, 0);
    EXPECT_EQ(e2, 0);

    // 2. Out of bounds
    std::wstring single = L"Just a single line without newline";
    auto [o1, o2] = SelectionModel::FindLineBoundaries(single, -5);
    EXPECT_EQ(o1, 0);
    EXPECT_EQ(o2, 0);

    // 3. Single line
    for (int i = 0; i < static_cast<int>(single.length()); ++i) {
        auto [s, e] = SelectionModel::FindLineBoundaries(single, i);
        EXPECT_EQ(s, 0);
        EXPECT_EQ(e, static_cast<int>(single.length()) - 1);
    }

    // 4. Multi-line with \n, \r\n, \r
    std::wstring multi = L"First line\nSecond line\r\nThird line\rFourth line";

    // Line 1
    for (int i = 0; i <= 9; ++i) {
        auto [s, e] = SelectionModel::FindLineBoundaries(multi, i);
        EXPECT_EQ(s, 0);
        EXPECT_EQ(e, 9);
    }

    // Line 2
    for (int i = 11; i <= 21; ++i) {
        auto [s, e] = SelectionModel::FindLineBoundaries(multi, i);
        EXPECT_EQ(s, 11);
        EXPECT_EQ(e, 21);
    }

    // Line 3
    for (int i = 24; i <= 33; ++i) {
        auto [s, e] = SelectionModel::FindLineBoundaries(multi, i);
        EXPECT_EQ(s, 24);
        EXPECT_EQ(e, 33);
    }

    // Line 4
    for (int i = 35; i <= 45; ++i) {
        auto [s, e] = SelectionModel::FindLineBoundaries(multi, i);
        EXPECT_EQ(s, 35);
        EXPECT_EQ(e, 45);
    }
}

TEST(Challenger1_SelectionModel_MultiClick_Integration_StateMachine) {
    std::wstring docText = L"The quick brown fox jumps.\nOver the lazy dog today.\nThird paragraph line here.";
    MockTextPage textPage(docText);
    SelectionModel sel;

    bool notificationFired = false;
    sel.onSelectionChanged = [&]() { notificationFired = true; };

    // 1. Single Click (Character selection) at char 'q' (index 4)
    notificationFired = false;
    sel.SelectCharacterAt(0, 4, &textPage);
    EXPECT_TRUE(notificationFired);
    EXPECT_TRUE(sel.HasTextSelection());
    EXPECT_EQ(sel.GetSelectionMode(), SelectionMode::Text);
    EXPECT_EQ(sel.GetTextSelection().startCharIndex, 4);
    EXPECT_EQ(sel.GetTextSelection().endCharIndex, 4);
    EXPECT_EQ(sel.GetSelectedText(), L"q");

    // 2. Double Click (Word selection) at char 'u' in "quick" (index 5)
    notificationFired = false;
    sel.SelectWordAt(0, 5, &textPage);
    EXPECT_TRUE(notificationFired);
    EXPECT_EQ(sel.GetTextSelection().startCharIndex, 4);
    EXPECT_EQ(sel.GetTextSelection().endCharIndex, 8);
    EXPECT_EQ(sel.GetSelectedText(), L"quick");

    // 3. Triple Click (Line selection) at char in Line 1 (index 12)
    notificationFired = false;
    sel.SelectLineAt(0, 12, &textPage);
    EXPECT_TRUE(notificationFired);
    EXPECT_EQ(sel.GetTextSelection().startCharIndex, 0);
    EXPECT_EQ(sel.GetTextSelection().endCharIndex, 25);
    EXPECT_EQ(sel.GetSelectedText(), L"The quick brown fox jumps.");

    // 4. Drag Expansion with Word Snap (Double-click drag)
    sel.SelectWordAt(0, 5, &textPage);
    sel.ExpandSelectionTo(0, 22, &textPage, TextClickType::Double);
    EXPECT_EQ(sel.GetTextSelection().startCharIndex, 4);
    EXPECT_EQ(sel.GetTextSelection().endCharIndex, 24);
    EXPECT_EQ(sel.GetSelectedText(), L"quick brown fox jumps");

    // Drag backward before anchor: drag to "The" (index 1)
    sel.ExpandSelectionTo(0, 1, &textPage, TextClickType::Double);
    EXPECT_EQ(sel.GetTextSelection().startCharIndex, 0);
    EXPECT_EQ(sel.GetTextSelection().endCharIndex, 8);
    EXPECT_EQ(sel.GetSelectedText(), L"The quick");

    // 5. Drag Expansion with Line Snap (Triple-click drag)
    sel.SelectLineAt(0, 30, &textPage);
    sel.ExpandSelectionTo(0, 60, &textPage, TextClickType::Triple);
    EXPECT_EQ(sel.GetTextSelection().startCharIndex, 27); // Start of line 2
    EXPECT_EQ(sel.GetTextSelection().endCharIndex, 77);   // End of line 3 (index 77 is period)
}

TEST(Challenger1_SelectionModel_ModeTransitions_Objects_Vs_Text) {
    SelectionModel sel;
    MockTextPage textPage(L"Sample text content");

    // 1. Select text
    sel.SetTextSelection(0, 0, 5, &textPage);
    EXPECT_TRUE(sel.HasTextSelection());
    EXPECT_FALSE(sel.HasObjectSelection());
    EXPECT_EQ(sel.GetSelectionMode(), SelectionMode::Text);

    // 2. Select Object (should clear text selection)
    SelectedObject obj;
    obj.id = "annot_1";
    obj.pageIndex = 0;
    obj.pageBounds = { 10.0f, 10.0f, 100.0f, 50.0f };
    sel.Select(obj);
    EXPECT_TRUE(sel.HasObjectSelection());
    EXPECT_FALSE(sel.HasTextSelection());
    EXPECT_EQ(sel.GetSelectionMode(), SelectionMode::Objects);
    EXPECT_EQ(sel.GetSelectedCount(), 1);

    // 3. Add second object
    SelectedObject obj2;
    obj2.id = "annot_2";
    obj2.pageIndex = 0;
    obj2.pageBounds = { 200.0f, 200.0f, 300.0f, 250.0f };
    sel.AddSelect(obj2);
    EXPECT_EQ(sel.GetSelectedCount(), 2);
    RectF bounds = sel.GetSelectionBounds();
    EXPECT_EQ(bounds.left, 10.0f);
    EXPECT_EQ(bounds.top, 10.0f);
    EXPECT_EQ(bounds.right, 300.0f);
    EXPECT_EQ(bounds.bottom, 250.0f);

    // 4. Toggle object selection
    sel.ToggleSelect(obj);
    EXPECT_EQ(sel.GetSelectedCount(), 1);
    EXPECT_FALSE(sel.IsSelected("annot_1"));
    EXPECT_TRUE(sel.IsSelected("annot_2"));

    // 5. Clear all
    sel.Clear();
    EXPECT_FALSE(sel.HasSelection());
    EXPECT_EQ(sel.GetSelectionMode(), SelectionMode::None);
}

// ============================================================================
// SUITE 4: CURSOR RESOLUTION & ANGLE QUANTIZATION ADVERSARIAL VERIFICATION
// ============================================================================

TEST(Challenger1_CursorResolver_AngleToResizeCursor_Exhaustive_360_Degrees) {
    // Test full 360 degrees in 0.5 degree increments
    for (float deg = -360.0f; deg <= 720.0f; deg += 0.5f) {
        HCURSOR cur = CursorResolver::AngleToResizeCursor(deg);
        EXPECT_TRUE(cur != nullptr);

        float a = std::fmod(deg, 180.0f);
        if (a < 0.0f) a += 180.0f;

        if (a < 22.5f || a >= 157.5f) {
            EXPECT_EQ(cur, CursorResolver::GetSystemCursor(IDC_SIZEWE));
        } else if (a >= 22.5f && a < 67.5f) {
            EXPECT_EQ(cur, CursorResolver::GetSystemCursor(IDC_SIZENWSE));
        } else if (a >= 67.5f && a < 112.5f) {
            EXPECT_EQ(cur, CursorResolver::GetSystemCursor(IDC_SIZENS));
        } else {
            EXPECT_EQ(cur, CursorResolver::GetSystemCursor(IDC_SIZENESW));
        }
    }
}

TEST(Challenger1_CursorResolver_Precedence_Priority_Hierarchy) {
    using namespace ui::tools;

    // 1. Handle hover highest priority
    HCURSOR cur = CursorResolver::ResolveCursor(
        ToolType::Pan, ToolState::Dragging, HandleType::N, 0.0f, true, true, true);
    EXPECT_EQ(cur, CursorResolver::GetSystemCursor(IDC_SIZENS));

    // 2. InPlaceEditing priority over link/object
    cur = CursorResolver::ResolveCursor(
        ToolType::Select, ToolState::InPlaceEditing, HandleType::None, 0.0f, false, true, true);
    EXPECT_EQ(cur, CursorResolver::GetSystemCursor(IDC_IBEAM));

    // 3. Link hover priority over object/text
    cur = CursorResolver::ResolveCursor(
        ToolType::Select, ToolState::Idle, HandleType::None, 0.0f, true, true, true);
    EXPECT_EQ(cur, CursorResolver::GetSystemCursor(IDC_HAND));

    // 4. Object hover in Select tool
    cur = CursorResolver::ResolveCursor(
        ToolType::Select, ToolState::Idle, HandleType::None, 0.0f, true, false, true);
    EXPECT_EQ(cur, CursorResolver::GetSystemCursor(IDC_SIZEALL));

    // 5. Text hover in Select tool
    cur = CursorResolver::ResolveCursor(
        ToolType::Select, ToolState::Idle, HandleType::None, 0.0f, true, false, false);
    EXPECT_EQ(cur, CursorResolver::GetSystemCursor(IDC_IBEAM));

    // 6. Tool fallbacks
    cur = CursorResolver::ResolveCursor(
        ToolType::Pan, ToolState::Idle, HandleType::None, 0.0f, false, false, false);
    EXPECT_EQ(cur, CursorResolver::GetSystemCursor(IDC_HAND));

    cur = CursorResolver::ResolveCursor(
        ToolType::Rectangle, ToolState::Idle, HandleType::None, 0.0f, false, false, false);
    EXPECT_EQ(cur, CursorResolver::GetSystemCursor(IDC_CROSS));
}

// ============================================================================
// MAIN RUNNER
// ============================================================================
int main() {
    std::cout << "Starting Milestone 4 Challenger 1 Adversarial Interaction Stress Suite...\n";
    return TestRunner::Instance().RunAll();
}
