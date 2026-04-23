#pragma once
#include <functional>
#include <vector>
#include <glm/glm.hpp>

class UnifiedVFX;

// ---- VFXEvent ---------------------------------------------------------------
struct VFXEvent {
    float delay;  // seconds from combo start
    std::function<void(UnifiedVFX&, glm::vec2)> action;
};

// ---- VFXCombo ---------------------------------------------------------------
// Sequences a series of timed VFX events into a choreographed moment.
// Inspired by MagnumOpus's combo system for boss phase transitions.
//
// Usage:
//   VFXCombo::epicDeath(theme).play(m_vfx, bossPos);
//   m_activeCombo.update(dt);

class VFXCombo {
public:
    // ---- Builder API --------------------------------------------------------
    VFXCombo& then(float delay, std::function<void(UnifiedVFX&, glm::vec2)> action);
    VFXCombo& thenRepeat(float startDelay, float interval, int times,
                         std::function<void(UnifiedVFX&, glm::vec2)> action);

    // ---- Predefined combos --------------------------------------------------
    static VFXCombo bossPhaseTransition();
    static VFXCombo epicDeath();
    static VFXCombo ultimateAttack();
    static VFXCombo impactCrescent();

    // ---- Playback -----------------------------------------------------------
    void  play(UnifiedVFX& vfx, glm::vec2 origin);
    void  update(float dt);
    void  stop();
    bool  isPlaying() const { return m_playing; }

private:
    std::vector<VFXEvent> m_events;
    float                 m_elapsed = 0.f;
    glm::vec2             m_origin  = {};
    UnifiedVFX*           m_vfx     = nullptr;
    bool                  m_playing = false;
    size_t                m_nextIdx = 0;
};
