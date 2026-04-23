#pragma once
#include "renderer/ParticleSystem.h"
#include "vfx/Theme.h"
#include "vfx/ScreenEffects.h"
#include <glm/glm.hpp>
#include <functional>

// Forward declarations
class TrailRenderer;
class BeamRenderer;
class VFXCombo;

// ---- UnifiedVFX -------------------------------------------------------------
// Master VFX dispatcher inspired by MagnumOpus's UnifiedVFXManager.
// All visual effects flow through this class: particles, trails, beams, screen.
//
// Theme-aware: effects automatically pick colors/style from the active theme.
// Composable: individual effects combine into choreographed moments.
//
// Usage:
//   m_vfx.init(&m_particles, &m_trails, &m_beams, &m_screen);
//   m_vfx.setTheme(ThemeRegistry::instance().active());
//   m_vfx.impact({100, 200});
//   m_vfx.screenEffects().shake(6.f, 0.3f);

class UnifiedVFX {
public:
    // ---- Lifecycle ----------------------------------------------------------
    void init(ParticleManager* particles,
              TrailRenderer*   trails,
              BeamRenderer*    beams,
              ScreenEffects*   screen);

    void setTheme(const Theme* theme);
    const Theme* theme() const { return m_theme; }

    void update(float dt);

    // ---- Generic theme-aware effects ----------------------------------------

    // Hit/impact spark burst at a world position.
    void impact(glm::vec2 pos, const Theme* override = nullptr);

    // Larger explosion burst (boss hits, area damage).
    void explosion(glm::vec2 pos, float radius = 32.f, const Theme* override = nullptr);

    // Ambient aura emanating from a point (buffs, item glow).
    void aura(glm::vec2 pos, float radius = 24.f, const Theme* override = nullptr);

    // Quick weapon-swing arc of particles.
    void swingAura(glm::vec2 center, float angleDeg, float arcDeg = 90.f,
                   const Theme* override = nullptr);

    // Boss/enemy death: maximum spectacle explosion.
    void deathExplosion(glm::vec2 pos, const Theme* override = nullptr);

    // Trail of particles along a line (projectile wake).
    void particleTrail(glm::vec2 from, glm::vec2 to, const Theme* override = nullptr);

    // ---- Theme-specific spectacles (direct ports from MagnumOpus) -----------
    void bellChime(glm::vec2 pos);      // La Campanella — bell resonance
    void sakuraBurst(glm::vec2 pos);    // Eroica — heroic petal explosion
    void featherBurst(glm::vec2 pos);   // Swan Lake — graceful feather shower
    void lunarGlow(glm::vec2 pos);      // Moonlight Sonata — soft lunar halo
    void voidTendril(glm::vec2 pos);    // Enigma Variations — dark tendrils
    void cosmicDestiny(glm::vec2 pos);  // Fate — double spiral galaxy burst
    void heroicFanfare(glm::vec2 pos);  // Eroica — triumphant notes + rays

    // ---- Music motif (universal) --------------------------------------------
    // Spawn floating music-note particles — the engine's signature visual.
    void musicNotes(glm::vec2 pos, const glm::vec3& color, int count = 8);

    // ---- Screen effects (delegates) -----------------------------------------
    ScreenEffects& screenEffects() { return *m_screen; }

    // ---- Sub-system access --------------------------------------------------
    ParticleManager& particles() { return *m_particles; }
    TrailRenderer&   trails()    { return *m_trails;    }
    BeamRenderer&    beams()     { return *m_beams;     }

private:
    ParticleManager* m_particles = nullptr;
    TrailRenderer*   m_trails    = nullptr;
    BeamRenderer*    m_beams     = nullptr;
    ScreenEffects*   m_screen    = nullptr;
    const Theme*     m_theme     = nullptr;

    // Helper: get theme or fall back to override, then default colors
    const Theme* resolveTheme(const Theme* override) const;
    glm::vec3    primaryColor(const Theme* t) const;
    glm::vec3    glowColor(const Theme* t) const;
};
