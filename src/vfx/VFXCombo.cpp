#include "vfx/VFXCombo.h"
#include "vfx/UnifiedVFX.h"
#include <algorithm>

// ---- Builder API ------------------------------------------------------------

VFXCombo& VFXCombo::then(float delay,
                           std::function<void(UnifiedVFX&, glm::vec2)> action) {
    m_events.push_back({delay, std::move(action)});
    return *this;
}

VFXCombo& VFXCombo::thenRepeat(float startDelay, float interval, int times,
                                 std::function<void(UnifiedVFX&, glm::vec2)> action) {
    for (int i = 0; i < times; ++i)
        m_events.push_back({startDelay + interval * i, action});
    return *this;
}

// ---- Predefined combos ------------------------------------------------------

VFXCombo VFXCombo::bossPhaseTransition() {
    VFXCombo c;
    c.then(0.00f, [](UnifiedVFX& v, glm::vec2 p) {
        v.screenEffects().bossPhaseTransition();
        v.explosion(p, 40.f);
    });
    c.then(0.15f, [](UnifiedVFX& v, glm::vec2 p) { v.explosion(p, 28.f); });
    c.then(0.30f, [](UnifiedVFX& v, glm::vec2 p) { v.musicNotes(p, {1,1,1}, 10); });
    c.then(0.50f, [](UnifiedVFX& v, glm::vec2 p) { v.explosion(p, 35.f); });
    return c;
}

VFXCombo VFXCombo::epicDeath() {
    VFXCombo c;
    c.then(0.00f, [](UnifiedVFX& v, glm::vec2 p) {
        v.deathExplosion(p);
    });
    c.thenRepeat(0.05f, 0.08f, 5, [](UnifiedVFX& v, glm::vec2 p) {
        v.explosion(p + glm::vec2{(float)(rand()%80-40), (float)(rand()%80-40)}, 20.f);
    });
    c.then(0.55f, [](UnifiedVFX& v, glm::vec2 p) {
        v.cosmicDestiny(p);
    });
    c.then(0.90f, [](UnifiedVFX& v, glm::vec2 p) {
        v.musicNotes(p, {1,1,1}, 16);
    });
    return c;
}

VFXCombo VFXCombo::ultimateAttack() {
    VFXCombo c;
    c.then(0.00f, [](UnifiedVFX& v, glm::vec2 p) {
        v.screenEffects().ultimateHit();
        v.explosion(p, 50.f);
    });
    c.then(0.08f, [](UnifiedVFX& v, glm::vec2 p) { v.heroicFanfare(p); });
    c.thenRepeat(0.12f, 0.06f, 4, [](UnifiedVFX& v, glm::vec2 p) {
        v.swingAura(p, (float)(rand()%360), 120.f);
    });
    c.then(0.40f, [](UnifiedVFX& v, glm::vec2 p) { v.musicNotes(p, {1,1,1}, 12); });
    return c;
}

VFXCombo VFXCombo::impactCrescent() {
    VFXCombo c;
    c.then(0.00f, [](UnifiedVFX& v, glm::vec2 p) { v.impact(p); });
    c.then(0.05f, [](UnifiedVFX& v, glm::vec2 p) { v.swingAura(p, 0.f, 180.f); });
    c.then(0.10f, [](UnifiedVFX& v, glm::vec2 p) { v.musicNotes(p, {1,.9f,.7f}, 4); });
    return c;
}

// ---- Playback ---------------------------------------------------------------

void VFXCombo::play(UnifiedVFX& vfx, glm::vec2 origin) {
    m_vfx     = &vfx;
    m_origin  = origin;
    m_elapsed = 0.f;
    m_nextIdx = 0;
    m_playing = true;

    // Sort events by delay for safe sequential firing
    std::stable_sort(m_events.begin(), m_events.end(),
                     [](const VFXEvent& a, const VFXEvent& b){
                         return a.delay < b.delay;
                     });
}

void VFXCombo::update(float dt) {
    if (!m_playing || !m_vfx) return;

    m_elapsed += dt;

    while (m_nextIdx < m_events.size() &&
           m_events[m_nextIdx].delay <= m_elapsed) {
        m_events[m_nextIdx].action(*m_vfx, m_origin);
        ++m_nextIdx;
    }

    if (m_nextIdx >= m_events.size())
        m_playing = false;
}

void VFXCombo::stop() {
    m_playing = false;
    m_nextIdx = 0;
    m_elapsed = 0.f;
}
