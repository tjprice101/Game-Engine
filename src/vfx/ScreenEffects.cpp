#include "vfx/ScreenEffects.h"
#include <cmath>
#include <algorithm>

// ---- Trigger effects --------------------------------------------------------

void ScreenEffects::shake(float intensity, float duration, float frequency) {
    // Only upgrade if more intense
    if (intensity > m_shakeIntensity) {
        m_shakeIntensity = intensity;
        m_shakeFreq      = frequency;
    }
    m_shakeDuration = std::max(m_shakeDuration, duration);
    m_shakeTimer    = 0.f;
}

void ScreenEffects::flash(const glm::vec4& color, float duration) {
    m_flashColor    = color;
    m_flashDuration = duration;
    m_flashTimer    = 0.f;
}

void ScreenEffects::distort(ScreenDistortion type, float strength, float duration,
                             glm::vec2 centerUV) {
    m_distortType   = type;
    m_distortPeak   = strength;
    m_distortDur    = duration;
    m_distortTimer  = 0.f;
    m_distortCenter = centerUV;
}

// ---- Convenience combos -----------------------------------------------------

void ScreenEffects::impactPop(const glm::vec4& color) {
    shake(3.f, 0.18f, 32.f);
    flash(color, 0.12f);
}

void ScreenEffects::epicExplosion(const glm::vec4& color) {
    shake(9.f, 0.5f, 26.f);
    flash(color, 0.25f);
    distort(ScreenDistortion::Ripple, 0.025f, 0.6f);
}

void ScreenEffects::bossPhaseTransition() {
    shake(6.f, 0.7f, 20.f);
    flash({1.f, 1.f, 1.f, 0.6f}, 0.35f);
    distort(ScreenDistortion::Pulse, 0.04f, 0.8f);
}

void ScreenEffects::ultimateHit() {
    shake(12.f, 0.45f, 35.f);
    flash({1.f, 1.f, 1.f, 0.85f}, 0.2f);
    distort(ScreenDistortion::Shatter, 0.03f, 0.55f);
}

// ---- Update -----------------------------------------------------------------

void ScreenEffects::update(float dt) {
    // Shake
    if (m_shakeDuration > 0.f) {
        m_shakeTimer    += dt;
        m_shakeDuration -= dt;
        if (m_shakeDuration <= 0.f) {
            m_shakeDuration  = 0.f;
            m_shakeIntensity = 0.f;
        }
    }

    // Flash
    if (m_flashDuration > 0.f) {
        m_flashTimer    += dt;
        m_flashDuration -= dt;
        if (m_flashDuration <= 0.f) {
            m_flashDuration = 0.f;
            m_flashColor    = {0.f, 0.f, 0.f, 0.f};
        }
    }

    // Distortion
    if (m_distortDur > 0.f) {
        m_distortTimer += dt;
        m_distortDur   -= dt;
        if (m_distortDur <= 0.f) {
            m_distortDur  = 0.f;
            m_distortType = ScreenDistortion::None;
        }
    }
}

// ---- Query ------------------------------------------------------------------

ScreenEffectParams ScreenEffects::params() const {
    ScreenEffectParams p;

    // Shake offset: decaying sine oscillation
    if (m_shakeDuration > 0.f && m_shakeIntensity > 0.f) {
        float envelope = m_shakeDuration / std::max(m_shakeDuration + m_shakeTimer, 0.001f);
        float angle    = m_shakeTimer * m_shakeFreq * 6.2832f;
        float amp      = m_shakeIntensity * envelope;
        p.shakeOffset  = { std::cos(angle) * amp,
                           std::sin(angle * 1.3f + 0.9f) * amp * 0.7f };
    }

    // Flash: linear fade-out
    if (m_flashDuration > 0.f && m_flashColor.a > 0.f) {
        float t      = m_flashTimer / std::max(m_flashTimer + m_flashDuration, 0.001f);
        p.flashColor = m_flashColor * (1.f - t);
    }

    // Distortion: ease in/out over duration
    if (m_distortType != ScreenDistortion::None && m_distortDur > 0.f) {
        float totalDur = m_distortTimer + m_distortDur;
        float progress = m_distortTimer / std::max(totalDur, 0.001f);
        float envelope = std::sin(progress * 3.14159f); // bell curve
        p.distortType   = m_distortType;
        p.distortStr    = m_distortPeak * envelope;
        p.distortCenter = m_distortCenter;
        p.distortTime   = m_distortTimer;
    }

    return p;
}

bool ScreenEffects::isActive() const {
    return m_shakeDuration > 0.f || m_flashDuration > 0.f || m_distortDur > 0.f;
}
