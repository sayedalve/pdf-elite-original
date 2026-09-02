#include "viewport/KineticScrollFilter.h"
#include <cmath>
#include <algorithm>

namespace ui::viewport {

KineticScrollFilter::KineticScrollFilter()
    : KineticScrollFilter(DEFAULT_DECAY_RATE_PER_TICK) {}

KineticScrollFilter::KineticScrollFilter(float decayRatePerTick) {
    SetDecayRate(decayRatePerTick);
}

void KineticScrollFilter::SetDecayRate(float decayRatePerTick) {
    m_decayRatePerTick = std::clamp(decayRatePerTick, 0.01f, 0.99f);
    double retention = 1.0 - m_decayRatePerTick;
    m_decayConstant = -std::log(retention) / TICK_DURATION_SECONDS;
}

void KineticScrollFilter::AddWheelDelta(float deltaX, float deltaY) {
    float impulseX = deltaX * WHEEL_STEP_MULTIPLIER;
    float impulseY = deltaY * WHEEL_STEP_MULTIPLIER;

    m_velocityX = std::clamp(m_velocityX + impulseX, -MAX_VELOCITY, MAX_VELOCITY);
    m_velocityY = std::clamp(m_velocityY + impulseY, -MAX_VELOCITY, MAX_VELOCITY);
}

void KineticScrollFilter::AddVelocity(float vx, float vy) {
    m_velocityX = std::clamp(m_velocityX + vx, -MAX_VELOCITY, MAX_VELOCITY);
    m_velocityY = std::clamp(m_velocityY + vy, -MAX_VELOCITY, MAX_VELOCITY);
}

void KineticScrollFilter::SetVelocity(float vx, float vy) {
    m_velocityX = std::clamp(vx, -MAX_VELOCITY, MAX_VELOCITY);
    m_velocityY = std::clamp(vy, -MAX_VELOCITY, MAX_VELOCITY);
}

float KineticScrollFilter::CoalesceWheelDelta(float rawDelta, bool isTrackpad) {
    if (isTrackpad) {
        return rawDelta * 1.2f;
    }
    // Windows standard WHEEL_DELTA is 120
    float normalizedTicks = rawDelta / 120.0f;
    return normalizedTicks * 40.0f;
}

bool KineticScrollFilter::Update(double dtSeconds, float& outDeltaScrollX, float& outDeltaScrollY) {
    if (!IsActive()) {
        outDeltaScrollX = 0.0f;
        outDeltaScrollY = 0.0f;
        return false;
    }

    if (dtSeconds <= 0.0001) {
        outDeltaScrollX = 0.0f;
        outDeltaScrollY = 0.0f;
        return true;
    }

    // Clamp delta time to prevent leap-forward on frame drops (max 100ms)
    double dt = std::min(dtSeconds, 0.1);

    // Apply exponential decay and analytical closed-form displacement over dt:
    // \Delta x = \int_0^{dt} v(t) dt = \frac{v_0}{\alpha}(1 - e^{-\alpha dt})
    double decayFactor = std::exp(-m_decayConstant * dt);
    float dx = 0.0f;
    float dy = 0.0f;
    if (m_decayConstant > 1e-6) {
        double factor = (1.0 - decayFactor) / m_decayConstant;
        dx = static_cast<float>(m_velocityX * factor);
        dy = static_cast<float>(m_velocityY * factor);
    } else {
        dx = static_cast<float>(m_velocityX * dt);
        dy = static_cast<float>(m_velocityY * dt);
    }

    m_velocityX = static_cast<float>(m_velocityX * decayFactor);
    m_velocityY = static_cast<float>(m_velocityY * decayFactor);

    // Check quiescence
    if (std::abs(m_velocityX) < VELOCITY_EPSILON) m_velocityX = 0.0f;
    if (std::abs(m_velocityY) < VELOCITY_EPSILON) m_velocityY = 0.0f;

    outDeltaScrollX = dx;
    outDeltaScrollY = dy;
    return true;
}

bool KineticScrollFilter::IsActive() const {
    return (std::abs(m_velocityX) >= VELOCITY_EPSILON || std::abs(m_velocityY) >= VELOCITY_EPSILON);
}

void KineticScrollFilter::Stop() {
    m_velocityX = 0.0f;
    m_velocityY = 0.0f;
    m_subPixelAccumulatorX = 0.0f;
    m_subPixelAccumulatorY = 0.0f;
}

void KineticScrollFilter::Reset() {
    Stop();
}

} // namespace ui::viewport
