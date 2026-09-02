#include "TestFramework.h"
#include "../src/core/CoordinateConverter.h"
#include "../src/core/Geometry.h"
#include "../src/ui/src/PdfViewer.h"
#include <cmath>
#include <vector>
#include <random>
#include <iostream>
#include <algorithm>

// ============================================================================
// ADVERSARIAL TEST SUITE 1: Extreme Zoom Scales & Mathematical Invariance (0.01x - 100.0x)
// ============================================================================

// ============================================================================
// ADVERSARIAL TEST SUITE 1: Extreme Zoom Scales & Mathematical Invariance
// ============================================================================

// ============================================================================
// ADVERSARIAL TEST SUITE 1: Extreme Zoom Scales & Mathematical Invariance
// ============================================================================

TEST(Challenger1_StandardToMaxZooms_Invariance_0_1x_to_100x) {
    const std::vector<double> zooms = {
        0.1, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 2.0,
        3.0, 5.0, 8.0, 10.0, 20.0, 50.0, 75.0, 100.0
    };
    const std::vector<int> rotations = { 0, 90, 180, 270 };

    struct PageSpec { double w, h; };
    const std::vector<PageSpec> pageSizes = {
        { 612.0, 792.0 },      // Standard US Letter
        { 595.276, 841.89 },   // Standard A4
        { 10.0, 10.0 },        // Micro page
        { 14400.0, 14400.0 },  // Max PDF specification size (200x200 inches)
        { 50.0, 5000.0 },      // Ultra tall aspect ratio (1:100)
        { 5000.0, 50.0 }       // Ultra wide aspect ratio (100:1)
    };

    const std::vector<PointF> samplePoints = {
        { 0.0f, 0.0f },
        { 10.0f, 10.0f },
        { 100.5f, 250.75f },
        { 306.0f, 396.0f },
        { 612.0f, 792.0f }
    };

    float maxObservedError = 0.0f;

    for (double zoom : zooms) {
        for (int rot : rotations) {
            for (const auto& size : pageSizes) {
                CoordinateConverter::PageContext pageCtx{ size.w, size.h, rot };
                CoordinateConverter::ViewContext viewCtx{ zoom, 50.0, 100.0, 200.0, 150.0 };

                for (const auto& origPt : samplePoints) {
                    PointF screen = CoordinateConverter::PdfToScreen(pageCtx, viewCtx, origPt.x, origPt.y);
                    PointF recoveredPdf = CoordinateConverter::ScreenToPdf(pageCtx, viewCtx, screen.x, screen.y);

                    float diffX = std::abs(recoveredPdf.x - origPt.x);
                    float diffY = std::abs(recoveredPdf.y - origPt.y);
                    float err = std::max(diffX, diffY);
                    if (err > maxObservedError) maxObservedError = err;

                    float tolerance = (size.w > 2000.0 || size.h > 2000.0) ? 1e-3f : 1e-4f;
                    if (diffX >= tolerance || diffY >= tolerance) {
                        std::cout << "\n[ZOOM FAIL] zoom=" << zoom << " rot=" << rot << " page=(" << size.w << "x" << size.h << ") pt=(" << origPt.x << "," << origPt.y << ") recovered=(" << recoveredPdf.x << "," << recoveredPdf.y << ") diffX=" << diffX << " diffY=" << diffY << "\n";
                    }

                    EXPECT_TRUE(diffX < tolerance);
                    EXPECT_TRUE(diffY < tolerance);
                }
            }
        }
    }

    std::cout << "[Zoom Invariance 0.1x - 100.0x] Max observed error: " << maxObservedError << "\n";
}

TEST(Challenger1_MicroZoom_0_01x_to_0_09x_IEEE754_PrecisionBound) {
    const std::vector<double> microZooms = { 0.01, 0.02, 0.03, 0.05, 0.07, 0.09 };
    const std::vector<int> rotations = { 0, 90, 180, 270 };
    CoordinateConverter::PageContext pageCtx{ 612.0, 792.0, 0 };
    CoordinateConverter::ViewContext viewCtx{ 0.01, 50.0, 100.0, 200.0, 150.0 };

    float maxMicroError = 0.0f;

    for (double zoom : microZooms) {
        viewCtx.zoom = zoom;
        for (int rot : rotations) {
            pageCtx.rotation = rot;
            PointF testPts[] = { { 0.0f, 0.0f }, { 100.0f, 200.0f }, { 612.0f, 792.0f } };
            for (const auto& pt : testPts) {
                PointF screen = CoordinateConverter::PdfToScreen(pageCtx, viewCtx, pt.x, pt.y);
                PointF recovered = CoordinateConverter::ScreenToPdf(pageCtx, viewCtx, screen.x, screen.y);

                float diffX = std::abs(recovered.x - pt.x);
                float diffY = std::abs(recovered.y - pt.y);
                float err = std::max(diffX, diffY);
                if (err > maxMicroError) maxMicroError = err;

                // At 0.01x zoom with 32-bit float PointF, theoretical ULP bound is < 5e-4
                EXPECT_TRUE(diffX < 5e-4f);
                EXPECT_TRUE(diffY < 5e-4f);
            }
        }
    }
    std::cout << "[Micro Zoom Invariance 0.01x - 0.09x] Max observed error: " << maxMicroError << "\n";
}

// ============================================================================
// ADVERSARIAL TEST SUITE 2: Extreme Scroll & Viewport Offsets
// ============================================================================

TEST(Challenger1_StandardScrollOffsets_Invariance) {
    const std::vector<double> scrollOffsets = {
        -2000.0, -1000.0, -123.45, 0.0, 123.45, 1000.0, 2000.0
    };
    const std::vector<double> pageOffsets = {
        -500.0, -100.0, 0.0, 50.5, 500.0, 1000.0
    };
    const std::vector<int> rotations = { 0, 90, 180, 270 };

    CoordinateConverter::PageContext pageCtx{ 612.0, 792.0, 0 };
    float maxScrollError = 0.0f;

    for (double scroll : scrollOffsets) {
        for (double pageOff : pageOffsets) {
            for (int rot : rotations) {
                pageCtx.rotation = rot;
                CoordinateConverter::ViewContext viewCtx{ 1.5, scroll, scroll * 0.75, pageOff, pageOff * 1.25 };

                PointF testPts[] = {
                    { 0.0f, 0.0f },
                    { 150.0f, 200.0f },
                    { 306.0f, 396.0f },
                    { 612.0f, 792.0f }
                };

                for (const auto& pt : testPts) {
                    PointF screen = CoordinateConverter::PdfToScreen(pageCtx, viewCtx, pt.x, pt.y);
                    PointF recovered = CoordinateConverter::ScreenToPdf(pageCtx, viewCtx, screen.x, screen.y);

                    float diffX = std::abs(recovered.x - pt.x);
                    float diffY = std::abs(recovered.y - pt.y);
                    float err = std::max(diffX, diffY);
                    if (err > maxScrollError) maxScrollError = err;

                    if (diffX >= 1e-4f || diffY >= 1e-4f) {
                        std::cout << "\n[SCROLL FAIL] scroll=" << scroll << " pageOff=" << pageOff << " rot=" << rot << " pt=(" << pt.x << "," << pt.y << ") recovered=(" << recovered.x << "," << recovered.y << ") diffX=" << diffX << " diffY=" << diffY << "\n";
                    }

                    EXPECT_TRUE(diffX < 1e-4f);
                    EXPECT_TRUE(diffY < 1e-4f);
                }
            }
        }
    }

    std::cout << "[Scroll Invariance (-2000 to +2000 DIPs)] Max observed error: " << maxScrollError << "\n";
}

TEST(Challenger1_ExtremeScrollOffsets_IEEE754_Bound) {
    const std::vector<double> scrollOffsets = {
        -50000.0, -10000.0, 10000.0, 50000.0
    };
    const std::vector<double> pageOffsets = {
        -10000.0, 10000.0
    };
    const std::vector<int> rotations = { 0, 90, 180, 270 };

    CoordinateConverter::PageContext pageCtx{ 612.0, 792.0, 0 };
    float maxScrollError = 0.0f;

    for (double scroll : scrollOffsets) {
        for (double pageOff : pageOffsets) {
            for (int rot : rotations) {
                pageCtx.rotation = rot;
                CoordinateConverter::ViewContext viewCtx{ 1.5, scroll, scroll * 0.75, pageOff, pageOff * 1.25 };

                PointF testPts[] = { { 0.0f, 0.0f }, { 306.0f, 396.0f }, { 612.0f, 792.0f } };

                for (const auto& pt : testPts) {
                    PointF screen = CoordinateConverter::PdfToScreen(pageCtx, viewCtx, pt.x, pt.y);
                    PointF recovered = CoordinateConverter::ScreenToPdf(pageCtx, viewCtx, screen.x, screen.y);

                    float diffX = std::abs(recovered.x - pt.x);
                    float diffY = std::abs(recovered.y - pt.y);
                    float err = std::max(diffX, diffY);
                    if (err > maxScrollError) maxScrollError = err;

                    // With 50,000 DIP offset, 32-bit float epsilon is 50000 * 2^-24 = ~0.003
                    EXPECT_TRUE(diffX < 5e-3f);
                    EXPECT_TRUE(diffY < 5e-3f);
                }
            }
        }
    }

    std::cout << "[Extreme Scroll Invariance (+-50000 DIPs)] Max observed error: " << maxScrollError << "\n";
}

// ============================================================================
// ADVERSARIAL TEST SUITE 3: Rect Transformation Invariance & Bounding Normalization
// ============================================================================

TEST(Challenger1_RectTransform_AllRotations_Normalization) {
    int rotations[] = { 0, 90, 180, 270 };
    CoordinateConverter::ViewContext viewCtx{ 2.5, 45.0, 90.0, 150.0, 200.0 };

    for (int rot : rotations) {
        CoordinateConverter::PageContext pageCtx{ 612.0, 792.0, rot };

        // Test standard PDF rect (x1 < x2, y1 < y2)
        double left = 100.0, bottom = 150.0, right = 400.0, top = 550.0;
        RectF screenRect = CoordinateConverter::PdfToScreenRect(pageCtx, viewCtx, left, top, right, bottom);

        // Screen rect MUST have left <= right and top <= bottom
        EXPECT_TRUE(screenRect.left <= screenRect.right);
        EXPECT_TRUE(screenRect.top <= screenRect.bottom);
        EXPECT_TRUE(screenRect.Width() > 0);
        EXPECT_TRUE(screenRect.Height() > 0);

        // Converting back to PDF space
        RectF recoveredPdf = CoordinateConverter::ScreenToPdfRect(pageCtx, viewCtx, screenRect.left, screenRect.top, screenRect.right, screenRect.bottom);

        // Recovered PDF rect MUST have left <= right and top <= bottom
        EXPECT_TRUE(recoveredPdf.left <= recoveredPdf.right);
        EXPECT_TRUE(recoveredPdf.top <= recoveredPdf.bottom);

        // Verify recovered coordinates match original bounds
        float diffLeft = std::abs(recoveredPdf.left - static_cast<float>(left));
        float diffRight = std::abs(recoveredPdf.right - static_cast<float>(right));
        float diffBottom = std::abs(recoveredPdf.top - static_cast<float>(bottom)); // Note: in PDF space, Y is inverted
        float diffTop = std::abs(recoveredPdf.bottom - static_cast<float>(top));

        EXPECT_TRUE(diffLeft < 1e-4f);
        EXPECT_TRUE(diffRight < 1e-4f);
        EXPECT_TRUE(diffBottom < 1e-4f || std::abs(recoveredPdf.top - static_cast<float>(top)) < 1e-4f);
        EXPECT_TRUE(diffTop < 1e-4f || std::abs(recoveredPdf.bottom - static_cast<float>(bottom)) < 1e-4f);
    }
}

TEST(Challenger1_DegenerateRects_PointsAndLines) {
    CoordinateConverter::PageContext pageCtx{ 612.0, 792.0, 90 };
    CoordinateConverter::ViewContext viewCtx{ 1.0, 0.0, 0.0, 0.0, 0.0 };

    // 1. Single point rect (zero area)
    RectF ptScreen = CoordinateConverter::PdfToScreenRect(pageCtx, viewCtx, 100.0, 100.0, 100.0, 100.0);
    EXPECT_EQ(ptScreen.left, ptScreen.right);
    EXPECT_EQ(ptScreen.top, ptScreen.bottom);
    EXPECT_EQ(ptScreen.Width(), 0.0f);
    EXPECT_EQ(ptScreen.Height(), 0.0f);

    RectF ptPdf = CoordinateConverter::ScreenToPdfRect(pageCtx, viewCtx, ptScreen.left, ptScreen.top, ptScreen.right, ptScreen.bottom);
    EXPECT_EQ(ptPdf.left, ptPdf.right);
    EXPECT_EQ(ptPdf.top, ptPdf.bottom);
    EXPECT_TRUE(std::abs(ptPdf.left - 100.0f) < 1e-4f);
    EXPECT_TRUE(std::abs(ptPdf.top - 100.0f) < 1e-4f);

    // 2. Horizontal line (zero height)
    RectF hScreen = CoordinateConverter::PdfToScreenRect(pageCtx, viewCtx, 50.0, 200.0, 350.0, 200.0);
    EXPECT_TRUE(hScreen.left <= hScreen.right);
    EXPECT_TRUE(hScreen.top <= hScreen.bottom);

    // 3. Vertical line (zero width)
    RectF vScreen = CoordinateConverter::PdfToScreenRect(pageCtx, viewCtx, 200.0, 50.0, 200.0, 350.0);
    EXPECT_TRUE(vScreen.left <= vScreen.right);
    EXPECT_TRUE(vScreen.top <= vScreen.bottom);
}

// ============================================================================
// ADVERSARIAL TEST SUITE 4: Monte Carlo 100,000 Vector Invariance Stress Test
// ============================================================================

TEST(Challenger1_MonteCarlo_100k_Vector_StressTest) {
    std::mt19937_64 rng(1337);
    std::uniform_real_distribution<double> zoomDist(0.01, 100.0);
    std::uniform_real_distribution<double> scrollDist(-10000.0, 10000.0);
    std::uniform_real_distribution<double> pageOffsetDist(-5000.0, 5000.0);
    std::uniform_real_distribution<double> pageSizeDist(10.0, 5000.0);
    std::uniform_int_distribution<int> rotDist(0, 3);
    const int rots[] = { 0, 90, 180, 270 };

    int maxDiscrepancyCount = 0;
    float maxObservedError = 0.0f;

    for (int i = 0; i < 100000; ++i) {
        double width = pageSizeDist(rng);
        double height = pageSizeDist(rng);
        int rot = rots[rotDist(rng)];

        CoordinateConverter::PageContext pageCtx{ width, height, rot };
        CoordinateConverter::ViewContext viewCtx{
            zoomDist(rng),
            scrollDist(rng),
            scrollDist(rng),
            pageOffsetDist(rng),
            pageOffsetDist(rng)
        };

        std::uniform_real_distribution<double> xDist(-1000.0, width + 1000.0);
        std::uniform_real_distribution<double> yDist(-1000.0, height + 1000.0);

        PointF origPdf{ static_cast<float>(xDist(rng)), static_cast<float>(yDist(rng)) };

        PointF screen = CoordinateConverter::PdfToScreen(pageCtx, viewCtx, origPdf.x, origPdf.y);
        PointF recovered = CoordinateConverter::ScreenToPdf(pageCtx, viewCtx, screen.x, screen.y);

        float errX = std::abs(recovered.x - origPdf.x);
        float errY = std::abs(recovered.y - origPdf.y);
        float maxErr = std::max(errX, errY);

        if (maxErr > maxObservedError) {
            maxObservedError = maxErr;
        }

        // Float32 roundtrip precision allows small error scaling with magnitude
        float tolerance = 1e-4f * std::max(1.0f, static_cast<float>(std::abs(origPdf.x) + std::abs(origPdf.y)));
        if (tolerance < 1e-4f) tolerance = 1e-4f;

        if (errX > tolerance || errY > tolerance) {
            maxDiscrepancyCount++;
        }
    }

    std::cout << "[Monte Carlo 100k Vectors] Max observed error: " << maxObservedError 
              << ", Failures: " << maxDiscrepancyCount << "\n";
    EXPECT_EQ(maxDiscrepancyCount, 0);
}

// ============================================================================
// ADVERSARIAL TEST SUITE 5: Tool State Machine & Pointer Lifecycle Stress
// ============================================================================

TEST(Challenger1_ToolStateMachine_RapidTransitions_And_Interruption) {
    PdfViewer viewer;

    ToolMode modes[] = { ToolMode::Pan, ToolMode::Select, ToolMode::AddText, ToolMode::Highlight, ToolMode::EditText };

    for (int cycle = 0; cycle < 1000; ++cycle) {
        ToolMode targetMode = modes[cycle % 5];
        viewer.SetToolMode(targetMode);
        EXPECT_TRUE(viewer.GetToolMode() == targetMode);

        // Simulate mouse interactions
        viewer.OnMouseMove(100.0f + (cycle % 50), 100.0f + (cycle % 50));
        viewer.OnLButtonDown(100.0f, 100.0f);
        viewer.OnMouseMove(200.0f, 250.0f);

        if (cycle % 3 == 0) {
            // Abrupt cancel during active drag
            viewer.CancelActiveInteractions();
        } else {
            // Normal button up
            viewer.OnLButtonUp(200.0f, 250.0f);
        }

        // Mode must remain intact
        EXPECT_TRUE(viewer.GetToolMode() == targetMode);
    }
}

int main() {
    std::cout << "Starting Milestone 1 Challenger 1 Empirical Adversarial Suite...\n";
    return TestRunner::Instance().RunAll();
}
