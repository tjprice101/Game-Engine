#pragma once
#include <chrono>

class Timer {
public:
    using Clock    = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration  = std::chrono::duration<double>;

    Timer() : m_last(Clock::now()), m_accumulated(0.0), m_fixedStep(1.0 / 60.0) {}

    void setFixedStep(double hz) { m_fixedStep = 1.0 / hz; }

    // Call at the start of every frame. Returns true each fixed step that needs processing.
    void tick() {
        auto now  = Clock::now();
        m_delta   = Duration(now - m_last).count();
        // Cap delta so a lag spike doesn't cause a spiral of death
        if (m_delta > 0.25) m_delta = 0.25;
        m_last       = now;
        m_accumulated += m_delta;
        m_totalTime  += m_delta;
    }

    bool consumeFixedStep() {
        if (m_accumulated >= m_fixedStep) {
            m_accumulated -= m_fixedStep;
            return true;
        }
        return false;
    }

    double delta()      const { return m_delta; }
    double fixedStep()  const { return m_fixedStep; }
    double totalTime()  const { return m_totalTime; }
    float  deltaF()     const { return static_cast<float>(m_delta); }
    float  fixedStepF() const { return static_cast<float>(m_fixedStep); }

private:
    TimePoint m_last;
    double    m_delta      = 0.0;
    double    m_accumulated= 0.0;
    double    m_fixedStep  = 1.0 / 60.0;
    double    m_totalTime  = 0.0;
};
