#include "StrokeStabilizer.h"
#include <algorithm>

namespace ui {
namespace interaction {

StrokeStabilizer::StrokeStabilizer() {}

void StrokeStabilizer::StartStroke(const RawStrokePoint& p) {
    m_rawBuffer.clear();
    m_smoothedPoints.clear();
    
    m_curX = p.x;
    m_curY = p.y;
    m_vx = 0;
    m_vy = 0;

    m_smoothedPoints.push_back({ static_cast<float>(p.x), static_cast<float>(p.y) });
    m_rawBuffer.push_back(p);
}

void StrokeStabilizer::ProcessPoint(const RawStrokePoint& p) {
    // Pipeline: Preprocess -> Average
    ProcessInertia(p);
    ProcessVelocityGaussian();
}

void StrokeStabilizer::ProcessInertia(const RawStrokePoint& p) {
    // Simple physical spring-mass-damper inspired by Xournal++ Inertia
    double dx = p.x - m_curX;
    double dy = p.y - m_curY;

    double ax = dx / m_inertiaMass;
    double ay = dy / m_inertiaMass;

    m_vx = (m_vx + ax) * (1.0 - m_inertiaDrag);
    m_vy = (m_vy + ay) * (1.0 - m_inertiaDrag);

    m_curX += m_vx;
    m_curY += m_vy;
    
    RawStrokePoint smoothedP = p;
    smoothedP.x = m_curX;
    smoothedP.y = m_curY;
    
    m_rawBuffer.push_back(smoothedP);
    if (m_rawBuffer.size() > m_bufferSize) {
        m_rawBuffer.erase(m_rawBuffer.begin());
    }
}

void StrokeStabilizer::ProcessVelocityGaussian() {
    if (m_rawBuffer.empty()) return;
    
    // Simple arithmetic moving average representing Velocity Gaussian core logic
    double sumX = 0, sumY = 0;
    for (const auto& pt : m_rawBuffer) {
        sumX += pt.x;
        sumY += pt.y;
    }
    
    double avgX = sumX / m_rawBuffer.size();
    double avgY = sumY / m_rawBuffer.size();

    m_smoothedPoints.push_back({ static_cast<float>(avgX), static_cast<float>(avgY) });
}

void StrokeStabilizer::EndStroke() {
    // Flush any remaining buffer calculations
}

} // namespace interaction
} // namespace ui
