#pragma once
#include <cmath>
#include <algorithm>

namespace ui::viewport {

class KineticScrollFilter {
public:
    // Default: 35% decay per 16ms tick (0.65 retention factor per tick)
    static constexpr float DEFAULT_DECAY_RATE_PER_TICK = 0.35f;
    static constexpr double TICK_DURATION_SECONDS = 0.016; // 16ms standard tick
    static constexpr float VELOCITY_EPSILON = 0.1f;        // Stop threshold (px/s)
    static constexpr float MAX_VELOCITY = 8000.0f;         // Velocity clamp (px/s)
    static constexpr float WHEEL_STEP_MULTIPLIER = 2.5f;   // Velocity impulse per wheel tick

    KineticScrollFilter();
    explicit KineticScrollFilter(float decayRatePerTick);
    ~KineticScrollFilter() = default;

    // Wheel Impulse & Gesture Input
    void AddWheelDelta(float deltaX, float deltaY);
    void AddVelocity(float vx, float vy);
    void SetVelocity(float vx, float vy);

    // Coalesce high-frequency / trackpad deltas into kinetic impulses
    float CoalesceWheelDelta(float rawDelta, bool isTrackpad = false);

    // Physics Update Step (called per frame / timer tick)
    // Returns true if movement occurred, false if motion is quiescent/stopped
    bool Update(double dtSeconds, float& outDeltaScrollX, float& outDeltaScrollY);

    // State Queries & Control
    bool IsActive() const;
    void Stop();
    void Reset();

    float GetVelocityX() const { return m_velocityX; }
    float GetVelocityY() const { return m_velocityY; }

    void SetDecayRate(float decayRatePerTick);
    float GetDecayRate() const { return m_decayRatePerTick; }

private:
    float m_velocityX = 0.0f;
    float m_velocityY = 0.0f;
    float m_subPixelAccumulatorX = 0.0f;
    float m_subPixelAccumulatorY = 0.0f;

    float m_decayRatePerTick = DEFAULT_DECAY_RATE_PER_TICK;
    double m_decayConstant = 26.92; // alpha in exp(-alpha * dt)
};

} // namespace ui::viewport
