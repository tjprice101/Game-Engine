#pragma once
#include <glm/glm.hpp>
#include "vfx/Theme.h"

// ---- ScreenEffects ----------------------------------------------------------
// Screen-space dramatic effects: shake, flash, and distortion.
// Inspired by MagnumOpus's EroicaScreenShake and ScreenDistortionManager.
//
// Usage:
//   m_screenEffects.shake(8.f, 0.4f);
//   m_screenEffects.flash({1,0.8f,0,1}, 0.3f);
//   m_screenEffects.distort(ScreenDistortion::Ripple, 0.04f, 0.6f);
//
// Apply results:
//   glm::vec2 offset = m_screenEffects.shakeOffset(time);
//   PostProcess uniforms: flash/distort via params()

struct ScreenEffectParams {
    // Shake
    glm::vec2        shakeOffset   = {0.f, 0.f};  // pixels, applied to camera
    // Flash
    glm::vec4        flashColor    = {0.f, 0.f, 0.f, 0.f}; // RGBA overlay
    // Distortion
    ScreenDistortion distortType   = ScreenDistortion::None;
    float            distortStr    = 0.f;
    glm::vec2        distortCenter = {0.5f, 0.5f}; // UV space
    float            distortTime   = 0.f;           // elapsed time for animated distort
};

class ScreenEffects {
public:
    // ---- Trigger effects ----------------------------------------------------

    // Camera shake. intensity = max pixel offset, frequency = oscillations/sec.
    void shake(float intensity, float duration, float frequency = 28.f);

    // Full-screen color flash (additive overlay that fades out).
    void flash(const glm::vec4& color, float duration);

    // Full-screen warp distortion with a given type and peak strength.
    void distort(ScreenDistortion type, float strength, float duration,
                 glm::vec2 centerUV = {0.5f, 0.5f});

    // ---- Convenience combos -------------------------------------------------
    void impactPop(const glm::vec4& color);        // small shake + flash
    void epicExplosion(const glm::vec4& color);    // hard shake + flash + ripple
    void bossPhaseTransition();                     // heavy shake + pulse distort
    void ultimateHit();                             // shatter + white flash

    // ---- Update (call once per frame) ---------------------------------------
    void update(float dt);

    // ---- Query --------------------------------------------------------------
    // Returns current combined state for the render system to consume.
    ScreenEffectParams params() const;

    bool isActive() const;

private:
    // Shake state
    float     m_shakeIntensity = 0.f;
    float     m_shakeDuration  = 0.f;
    float     m_shakeFreq      = 28.f;
    float     m_shakeTimer     = 0.f;

    // Flash state
    glm::vec4 m_flashColor    = {0.f, 0.f, 0.f, 0.f};
    float     m_flashDuration = 0.f;
    float     m_flashTimer    = 0.f;

    // Distort state
    ScreenDistortion m_distortType   = ScreenDistortion::None;
    float            m_distortPeak   = 0.f;
    float            m_distortDur    = 0.f;
    float            m_distortTimer  = 0.f;
    glm::vec2        m_distortCenter = {0.5f, 0.5f};
};
