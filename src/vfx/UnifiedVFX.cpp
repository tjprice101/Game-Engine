#include "vfx/UnifiedVFX.h"
#include "renderer/TrailRenderer.h"
#include "renderer/BeamRenderer.h"
#include <cmath>
#include <cstdlib>

static float rng()                  { return (float)rand() / (float)RAND_MAX; }
static float rngR(float lo, float hi) { return lo + rng() * (hi - lo); }

// ---- Lifecycle --------------------------------------------------------------

void UnifiedVFX::init(ParticleManager* particles,
                       TrailRenderer*   trails,
                       BeamRenderer*    beams,
                       ScreenEffects*   screen) {
    m_particles = particles;
    m_trails    = trails;
    m_beams     = beams;
    m_screen    = screen;
}

void UnifiedVFX::setTheme(const Theme* theme) {
    m_theme = theme;
}

void UnifiedVFX::update(float /*dt*/) {
    // Currently a no-op — future home for timed ambient VFX per-theme
}

// ---- Helpers ----------------------------------------------------------------

const Theme* UnifiedVFX::resolveTheme(const Theme* override) const {
    if (override) return override;
    return m_theme;
}

glm::vec3 UnifiedVFX::primaryColor(const Theme* t) const {
    return t ? t->palette.primary : glm::vec3{0.8f, 0.6f, 1.0f};
}

glm::vec3 UnifiedVFX::glowColor(const Theme* t) const {
    return t ? t->palette.glow : glm::vec3{1.0f, 0.9f, 1.0f};
}

// ---- Generic effects --------------------------------------------------------

void UnifiedVFX::impact(glm::vec2 pos, const Theme* override) {
    if (!m_particles) return;
    const Theme* t    = resolveTheme(override);
    glm::vec3    col  = glowColor(t);

    // Spark burst
    m_particles->burst(pos, ParticlePreset::Spark, 12);

    // Small bloom ring
    m_particles->burst(pos, ParticlePreset::BloomRing, 6);

    if (t && t->vfx.musicNotes)
        musicNotes(pos, col, 3);
}

void UnifiedVFX::explosion(glm::vec2 pos, float /*radius*/, const Theme* override) {
    if (!m_particles) return;
    const Theme* t    = resolveTheme(override);
    glm::vec3    glow = glowColor(t);

    // Main burst
    m_particles->burst(pos, ParticlePreset::Explosion, 28);

    // Screen pop
    if (m_screen)
        m_screen->impactPop({glow.r, glow.g, glow.b, 0.35f});

    if (t && t->vfx.musicNotes) musicNotes(pos, glow, 5);
    if (t && t->vfx.glyphs)     m_particles->burst(pos, ParticlePreset::Glyph, 4);
}

void UnifiedVFX::aura(glm::vec2 pos, float /*radius*/, const Theme* override) {
    if (!m_particles) return;
    const Theme* t   = resolveTheme(override);
    glm::vec3    col = glowColor(t);

    ParticleEmitter e{};
    e.origin         = pos;
    e.originVariance = {20.f, 8.f};
    e.direction      = {0.f, -1.f};
    e.speed          = 25.f;   e.speedVariance = 15.f;
    e.spread         = 1.2f;
    e.emitRate       = 20.f;   e.loop          = true;
    e.lifeMin        = 0.6f;   e.lifeMax       = 1.4f;
    e.colorStart     = {col.r, col.g, col.b, 0.7f};
    e.colorEnd       = {col.r, col.g, col.b, 0.f};
    e.sizeStart      = 7.f;    e.sizeEnd       = 2.f;
    e.gravity        = -20.f;
    e.shape          = 0.f;
    m_particles->addEmitter(e);
}

void UnifiedVFX::swingAura(glm::vec2 center, float angleDeg, float arcDeg,
                            const Theme* override) {
    if (!m_particles) return;
    const Theme* t   = resolveTheme(override);
    glm::vec3    col = primaryColor(t);
    const float  PI  = 3.14159265f;

    float arcRad = arcDeg * PI / 180.f;
    float baseRad = angleDeg * PI / 180.f;
    int   n = 12;
    for (int i = 0; i < n; ++i) {
        float a = baseRad - arcRad * 0.5f + arcRad * (i / float(n - 1));
        float r = 20.f + rng() * 12.f;
        glm::vec2 p = center + glm::vec2{std::cos(a) * r, std::sin(a) * r};

        ParticleEmitter e{};
        e.origin         = p;
        e.originVariance = {2.f, 2.f};
        e.direction      = {std::cos(a), std::sin(a)};
        e.speed          = 60.f;  e.speedVariance = 20.f;
        e.spread         = 0.3f;
        e.emitRate       = 0.f;   e.loop          = false;
        e.lifeMin        = 0.2f;  e.lifeMax       = 0.4f;
        e.colorStart     = {col.r, col.g, col.b, 1.f};
        e.colorEnd       = {col.r, col.g, col.b, 0.f};
        e.sizeStart      = 5.f;   e.sizeEnd       = 1.f;
        e.gravity        = 0.f;
        e.shape          = 3.f; // sparkle
        m_particles->burst(p, ParticlePreset::Custom, 0);
    }
}

void UnifiedVFX::deathExplosion(glm::vec2 pos, const Theme* override) {
    if (!m_particles) return;
    const Theme* t   = resolveTheme(override);
    glm::vec3    col = primaryColor(t);
    glm::vec3    glow = glowColor(t);

    // Three concentric bursts with staggered sizes
    for (int wave = 0; wave < 3; ++wave) {
        float scale = 1.f + wave * 0.6f;
        m_particles->burst(pos, ParticlePreset::Explosion, int(40 * scale));
        m_particles->burst(pos, ParticlePreset::BloomRing, int(8 * scale));
    }

    if (m_screen) m_screen->epicExplosion({glow.r, glow.g, glow.b, 0.7f});

    musicNotes(pos, glow, 10);
    if (t && t->vfx.glyphs) m_particles->burst(pos, ParticlePreset::Glyph, 8);
}

void UnifiedVFX::particleTrail(glm::vec2 from, glm::vec2 to, const Theme* override) {
    if (!m_particles) return;
    const Theme* t   = resolveTheme(override);
    glm::vec3    col = primaryColor(t);

    glm::vec2 dir  = to - from;
    float     len  = glm::length(dir);
    if (len < 1.f) return;
    glm::vec2 step = dir / (len / 12.f);
    int       n    = (int)(len / 12.f);

    for (int i = 0; i < n; ++i) {
        glm::vec2 p = from + step * float(i);
        m_particles->burst(p, ParticlePreset::Magic, 1);
    }
}

// ---- Theme-specific spectacles ----------------------------------------------

void UnifiedVFX::bellChime(glm::vec2 pos) {
    if (!m_particles) return;
    // Gold sparkles in a bell-curve arc upward
    for (int i = 0; i < 12; ++i) {
        float angle = -1.2f + rngR(0.f, 2.4f);
        glm::vec2 p = pos + glm::vec2{std::cos(angle) * 10.f, std::sin(angle) * 10.f};
        m_particles->burst(p, ParticlePreset::Spark, 2);
    }
    musicNotes(pos, {1.f, 0.85f, 0.1f}, 5);
}

void UnifiedVFX::sakuraBurst(glm::vec2 pos) {
    if (!m_particles) return;
    // Pink petals spiral outward
    m_particles->burst(pos, ParticlePreset::SakuraPetal, 20);
    if (m_screen) m_screen->impactPop({1.f, 0.6f, 0.7f, 0.3f});
}

void UnifiedVFX::featherBurst(glm::vec2 pos) {
    if (!m_particles) return;
    m_particles->burst(pos, ParticlePreset::Feather, 16);
}

void UnifiedVFX::lunarGlow(glm::vec2 pos) {
    if (!m_particles) return;
    // Soft blue-white bloom rings expanding outward
    for (int i = 0; i < 3; ++i)
        m_particles->burst(pos + glm::vec2{rngR(-8.f, 8.f), rngR(-8.f, 8.f)},
                           ParticlePreset::BloomRing, 3);
    musicNotes(pos, {0.6f, 0.5f, 1.f}, 4);
}

void UnifiedVFX::voidTendril(glm::vec2 pos) {
    if (!m_particles) return;
    m_particles->burst(pos, ParticlePreset::VoidTendril, 14);
    m_particles->burst(pos, ParticlePreset::Glyph, 4);
}

void UnifiedVFX::cosmicDestiny(glm::vec2 pos) {
    if (!m_particles) return;
    // Double spiral — pink then white
    constexpr int N = 24;
    const float PI  = 3.14159265f;
    for (int i = 0; i < N; ++i) {
        float t = float(i) / N;
        float r = t * 40.f;
        float a1 = t * 4 * 2 * PI;
        float a2 = a1 + PI;
        glm::vec2 p1 = pos + glm::vec2{std::cos(a1) * r, std::sin(a1) * r};
        glm::vec2 p2 = pos + glm::vec2{std::cos(a2) * r, std::sin(a2) * r};
        m_particles->burst(p1, ParticlePreset::Spark, 1);
        m_particles->burst(p2, ParticlePreset::BloomRing, 1);
    }
    if (m_screen) m_screen->epicExplosion({1.f, 0.3f, 0.5f, 0.6f});
    musicNotes(pos, {1.f, 0.3f, 0.6f}, 12);
    m_particles->burst(pos, ParticlePreset::Glyph, 6);
}

void UnifiedVFX::heroicFanfare(glm::vec2 pos) {
    if (!m_particles) return;
    // Gold rays + music notes shooting upward
    for (int i = 0; i < 8; ++i) {
        float a = -1.5708f + rngR(-0.5f, 0.5f); // upward fan
        glm::vec2 p = pos + glm::vec2{std::cos(a) * rng() * 20.f,
                                       std::sin(a) * rng() * 20.f};
        m_particles->burst(p, ParticlePreset::Spark, 3);
    }
    musicNotes(pos, {1.f, 0.85f, 0.2f}, 8);
    if (m_screen) m_screen->flash({1.f, 0.9f, 0.6f, 0.4f}, 0.25f);
}

// ---- Music motif ------------------------------------------------------------

void UnifiedVFX::musicNotes(glm::vec2 pos, const glm::vec3& color, int count) {
    if (!m_particles) return;
    m_particles->burst(pos, ParticlePreset::MusicNote, count);
}
