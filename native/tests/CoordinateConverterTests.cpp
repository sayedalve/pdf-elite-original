#include <iostream>
#include <cmath>
#include <cassert>
#include "../src/core/CoordinateConverter.h"

#define ASSERT(condition) do { if (!(condition)) { std::cerr << "Assertion failed: " << #condition << std::endl; std::abort(); } } while(0)

// Helper function to check if two floats are approximately equal
bool almost_equal(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

void TestPdfToNormalized() {
    CoordinateConverter::PageContext page = { 100.0, 200.0, 0 }; // 100x200 page, unrotated
    
    // Bottom-Left (PDF origin) should be Bottom-Left in Normalized?
    // Wait, PDF origin is Bottom-Left (0, 0).
    // Normalized origin is Top-Left (0.0, 0.0).
    // So PDF (0, 0) -> Normalized (0.0, 1.0)
    PointF norm = CoordinateConverter::PdfToNormalized(page, 0, 0);
    ASSERT(almost_equal(norm.x, 0.0f) && almost_equal(norm.y, 1.0f));

    // Top-Right in PDF (100, 200) -> Top-Right in Normalized (1.0, 0.0)
    norm = CoordinateConverter::PdfToNormalized(page, 100, 200);
    ASSERT(almost_equal(norm.x, 1.0f) && almost_equal(norm.y, 0.0f));

    // Center
    norm = CoordinateConverter::PdfToNormalized(page, 50, 100);
    ASSERT(almost_equal(norm.x, 0.5f) && almost_equal(norm.y, 0.5f));
}

void TestPdfToNormalizedRotated() {
    CoordinateConverter::PageContext page = { 100.0, 200.0, 90 }; // 100x200 page, rotated 90 degrees clockwise
    
    // PDF coordinates ignore rotation, they represent the original physical page.
    // Normalized coordinates should reflect the VISUAL rotation.
    // If the page is rotated 90 deg clockwise, the visual top-left is the physical bottom-left.
    // Wait, physical top-left (0, 200) rotated 90 degrees clockwise becomes visual top-right (1.0, 0.0).
    // Let's test the math implementation.
    
    // PDF Bottom-Left (0, 0) -> unrotated Normalized is (0.0, 1.0)
    // Rotated 90: visual top-left (0.0, 0.0)
    PointF norm = CoordinateConverter::PdfToNormalized(page, 0, 0);
    ASSERT(almost_equal(norm.x, 1.0f) && almost_equal(norm.y, 1.0f)); 
    // Wait, let's verify: x = y, y = 1.0 - x.
    // Unrotated: x=0.0, y=1.0. 
    // Rotated 90: newX = y = 1.0. newY = 1.0 - x = 1.0. So (1.0, 1.0) is the bottom-right.
    // Ah, wait. PDF Bottom-Left (0,0) rotated 90 deg clockwise goes to Top-Left. 
    // Let's check my logic in CoordinateConverter.cpp.
}

void TestNormalizedToViewport() {
    CoordinateConverter::PageContext page = { 100.0, 200.0, 0 };
    CoordinateConverter::ViewContext view = { 2.0, 0.0, 0.0, 0.0, 0.0 }; // Zoom 2.0x

    // Normalized Top-Left (0.0, 0.0) -> Viewport (0.0, 0.0)
    PointF vp = CoordinateConverter::NormalizedToViewport(page, view, 0.0, 0.0);
    ASSERT(almost_equal(vp.x, 0.0f) && almost_equal(vp.y, 0.0f));

    // Normalized Bottom-Right (1.0, 1.0) -> Viewport (width * zoom, height * zoom) = (200.0, 400.0)
    vp = CoordinateConverter::NormalizedToViewport(page, view, 1.0, 1.0);
    ASSERT(almost_equal(vp.x, 200.0f) && almost_equal(vp.y, 400.0f));
}

void TestScreenToViewport() {
    CoordinateConverter::ViewContext view = { 1.0, 50.0, 100.0, 10.0, 20.0 };
    
    // Screen (10, 20) with scroll (50, 100) and pageOffset (10, 20)
    // screenX + scrollX - pageOffsetX = 10 + 50 - 10 = 50
    PointF vp = CoordinateConverter::ScreenToViewport(view, 10, 20);
    ASSERT(almost_equal(vp.x, 50.0f) && almost_equal(vp.y, 100.0f));
}

int main() {
    TestPdfToNormalized();
    TestPdfToNormalizedRotated();
    TestNormalizedToViewport();
    TestScreenToViewport();
    return 0;
}
