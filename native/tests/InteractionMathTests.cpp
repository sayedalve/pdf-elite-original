#include <iostream>
#include <cassert>
#include <vector>
#include "../src/ui/src/interaction/StrokeStabilizer.h"
#include "../src/ui/src/interaction/ShapeRecognizer.h"

#define ASSERT(condition) do { if (!(condition)) { std::cerr << "Assertion failed: " << #condition << std::endl; std::abort(); } } while(0)

void TestStrokeStabilization() {
    ui::interaction::StrokeStabilizer stabilizer;
    
    // Simulate a fast, jittery stroke
    stabilizer.StartStroke({0.0, 0.0, 1.0, 0.0});
    stabilizer.ProcessPoint({10.0, 5.0, 1.0, 0.1});
    stabilizer.ProcessPoint({20.0, -2.0, 1.0, 0.2}); // Jitter
    stabilizer.ProcessPoint({30.0, 6.0, 1.0, 0.3}); // Jitter
    stabilizer.ProcessPoint({40.0, 0.0, 1.0, 0.4});
    stabilizer.EndStroke();
    
    auto points = stabilizer.GetSmoothedPoints();
    ASSERT(!points.empty());
    
    // Verify smoothing applied (points are pushed to smoothed buffer)
    ASSERT(points.size() > 0);
}

void TestShapeRecognizerLine() {
    std::vector<PointF> stroke;
    // Perfect line with a slight jitter in the middle
    stroke.push_back({0.0f, 0.0f});
    stroke.push_back({10.0f, 10.0f});
    stroke.push_back({20.0f, 21.0f}); // Slight jitter
    stroke.push_back({30.0f, 30.0f});
    stroke.push_back({40.0f, 40.0f});
    
    auto result = ui::interaction::ShapeRecognizer::Recognize(stroke, 0.15);
    ASSERT(result.shape == ui::interaction::RecognizedShape::Line);
    ASSERT(result.start.x == 0.0f && result.end.x == 40.0f);
}

void TestShapeRecognizerEllipse() {
    std::vector<PointF> stroke;
    // Rough circle
    stroke.push_back({10.0f, 0.0f});
    stroke.push_back({20.0f, 10.0f});
    stroke.push_back({10.0f, 20.0f});
    stroke.push_back({0.0f, 10.0f});
    stroke.push_back({10.5f, 0.5f}); // Closed loop
    
    auto result = ui::interaction::ShapeRecognizer::Recognize(stroke, 0.2);
    ASSERT(result.shape == ui::interaction::RecognizedShape::Ellipse);
}

int main() {
    TestStrokeStabilization();
    TestShapeRecognizerLine();
    TestShapeRecognizerEllipse();
    std::cout << "Interaction Math tests passed!\n";
    return 0;
}
