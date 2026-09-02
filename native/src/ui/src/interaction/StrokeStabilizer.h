#pragma once
#include <vector>
#include <cmath>
#include "../../../core/Geometry.h"

namespace ui {
namespace interaction {

struct RawStrokePoint {
    double x, y;
    double pressure;
    double timestamp;
};

// Inspired by Xournal++: Preprocessors and Averaging Methods
class StrokeStabilizer {
public:
    StrokeStabilizer();

    // Configuration
    void SetDeadzoneRadius(double radius) { m_deadzoneRadius = radius; }
    void SetInertiaMass(double mass) { m_inertiaMass = mass; }
    void SetInertiaDrag(double drag) { m_inertiaDrag = drag; }
    void SetBufferSize(size_t size) { m_bufferSize = size; }

    void StartStroke(const RawStrokePoint& p);
    void ProcessPoint(const RawStrokePoint& p);
    void EndStroke();

    const std::vector<PointF>& GetSmoothedPoints() const { return m_smoothedPoints; }

private:
    std::vector<RawStrokePoint> m_rawBuffer;
    std::vector<PointF> m_smoothedPoints;
    
    // Config
    double m_deadzoneRadius = 2.0;
    double m_inertiaMass = 1.0;
    double m_inertiaDrag = 0.5;
    size_t m_bufferSize = 4;

    // State for Inertia
    double m_vx = 0, m_vy = 0;
    double m_curX = 0, m_curY = 0;

    void ProcessDeadzone(const RawStrokePoint& p);
    void ProcessInertia(const RawStrokePoint& p);
    void ProcessVelocityGaussian();
};

} // namespace interaction
} // namespace ui
