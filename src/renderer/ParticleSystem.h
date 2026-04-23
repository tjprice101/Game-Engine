#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include "renderer/Shader.h"
#include "renderer/Camera.h"

// ---- Particle types ---------------------------------------------------------
enum class ParticlePreset {
    // Original presets
    Fire, Smoke, Spark, Magic, Rain, Snow, Leaves, Blood, Dust, Explosion,
    Bubble, Leaf,
    // Music-engine presets (MagnumOpus-inspired)
    MusicNote,    // Gold/purple floating note sparkles      (shape=3 sparkle)
    Glyph,        // Arcane symbols, green/purple            (shape=3 sparkle)
    BloomRing,    // Expanding hollow luminous ring          (shape=2 ring)
    ElectricArc,  // Cyan/white fast velocity sparks         (shape=0 glow)
    SakuraPetal,  // Pink swirling soft petals               (shape=1 circle)
    Feather,      // White/sky-blue drifting feathers        (shape=1 circle)
    VoidTendril,  // Dark purple slow-rising void wisps      (shape=0 glow)
    Custom        // Caller fills in the emitter manually
};

// ---- Particle shape ---------------------------------------------------------
//  0 = soft glow (Gaussian) — default, HDR-bright core
//  1 = solid circle
//  2 = bloom ring (hollow)
//  3 = sparkle / cross
static constexpr float SHAPE_GLOW    = 0.f;
static constexpr float SHAPE_CIRCLE  = 1.f;
static constexpr float SHAPE_RING    = 2.f;
static constexpr float SHAPE_SPARKLE = 3.f;

// ---- Per-particle CPU state -------------------------------------------------
struct Particle {
    glm::vec2 pos;
    glm::vec2 vel;
    glm::vec4 colorStart;
    glm::vec4 colorEnd;
    float     sizeStart;
    float     sizeEnd;
    float     rotation;
    float     rotVel;
    float     life;      // remaining life in seconds
    float     maxLife;
    float     shape  = SHAPE_GLOW;
    bool      alive  = false;
};

// ---- Emitter config ---------------------------------------------------------
struct ParticleEmitter {
    glm::vec2     origin;
    glm::vec2     originVariance  = {8.f, 4.f};
    glm::vec2     direction       = {0.f, -1.f};
    float         speed           = 80.f;
    float         speedVariance   = 40.f;
    float         spread          = 0.8f;       // angular spread (radians)
    float         emitRate        = 20.f;       // particles per second
    float         lifeMin         = 0.5f;
    float         lifeMax         = 1.5f;
    glm::vec4     colorStart      = {1.f, .5f, 0.f, 1.f};
    glm::vec4     colorEnd        = {0.3f, 0.f, 0.f, 0.f};
    float         sizeStart       = 6.f;
    float         sizeEnd         = 1.f;
    float         gravity         = 0.f;
    float         shape           = SHAPE_GLOW;
    bool          loop            = true;
    bool          active          = true;
    float         _accumulator    = 0.f;
};

// ---- Per-instance GPU data --------------------------------------------------
// Matches the instanced vertex attributes in particles.vert (locations 1-5).
struct ParticleInstance {
    glm::vec2 pos;      // loc 1 — offset 0,  8 bytes
    float     size;     // loc 2 — offset 8,  4 bytes
    float     rotation; // loc 3 — offset 12, 4 bytes
    glm::vec4 color;    // loc 4 — offset 16, 16 bytes
    float     shape;    // loc 5 — offset 32, 4 bytes
    float     _pad[3];  //         offset 36, 12 bytes (16-byte alignment)
};
static_assert(sizeof(ParticleInstance) == 48);

// ---- ParticleSystem ---------------------------------------------------------
class ParticleSystem {
public:
    static constexpr int MAX_PARTICLES = 16384;

    ParticleSystem();
    ~ParticleSystem();

    ParticleSystem(const ParticleSystem&)            = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    // Emit a burst from a preset (count particles spawned immediately)
    void burst(glm::vec2 pos, ParticlePreset preset, int count = 20);

    // Add a continuous emitter (returns emitter index)
    int  addEmitter(const ParticleEmitter& cfg);
    void removeEmitter(int idx);
    ParticleEmitter& emitter(int idx) { return m_emitters[idx]; }

    // Build an emitter config from a preset template
    static ParticleEmitter makePreset(ParticlePreset type, glm::vec2 origin);

    void update(float dt);
    void draw(const Shader& shader, const Camera& camera);

    int aliveCount() const;

private:
    void spawnOne(const ParticleEmitter& cfg);
    int  findFreeSlot();

    Particle         m_particles[MAX_PARTICLES];
    std::vector<ParticleEmitter> m_emitters;

    GLuint m_vao           = 0;
    GLuint m_quadVBO       = 0;
    GLuint m_quadEBO       = 0;
    GLuint m_instanceVBO   = 0;
    size_t m_instanceCap   = 0;

    std::vector<ParticleInstance> m_gpuData;
};

using ParticleManager = ParticleSystem;
