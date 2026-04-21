#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include "renderer/Shader.h"
#include "renderer/Texture.h"
#include "renderer/RenderTarget.h"
#include "renderer/Camera.h"

// ---- Particle types ---------------------------------------------------------
enum class ParticlePreset {
    Fire, Smoke, Spark, Magic, Rain, Snow, Leaves, Blood, Dust, Explosion,
    Bubble, Leaf, Custom
};

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
    bool      alive = false;
};

// ---- Emitter config ---------------------------------------------------------
struct ParticleEmitter {
    glm::vec2     origin;
    glm::vec2     originVariance  = {8.f, 4.f}; // position randomness
    glm::vec2     direction       = {0.f, -1.f};// base velocity direction
    float         speed           = 80.f;
    float         speedVariance   = 40.f;
    float         spread          = 0.8f;        // angular spread in radians
    float         emitRate        = 20.f;        // particles per second
    float         lifeMin         = 0.5f;
    float         lifeMax         = 1.5f;
    glm::vec4     colorStart      = {1.f,.5f,0.f,1.f};
    glm::vec4     colorEnd        = {0.3f,0.f,0.f,0.f};
    float         sizeStart       = 6.f;
    float         sizeEnd         = 1.f;
    float         gravity         = 0.f;         // pixels/s² downward
    bool          loop            = true;
    bool          active          = true;
    float         _accumulator    = 0.f;         // internal
};

// ---- Per-instance GPU data --------------------------------------------------
struct ParticleInstance {
    glm::vec2 pos;
    float     size;
    float     rotation;
    glm::vec4 color;
};
static_assert(sizeof(ParticleInstance) == 32);

// ---- ParticleSystem ---------------------------------------------------------
class ParticleSystem {
public:
    static constexpr int MAX_PARTICLES = 16384;

    ParticleSystem();
    ~ParticleSystem();

    ParticleSystem(const ParticleSystem&)            = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    // Emit a burst from a preset
    void burst(glm::vec2 pos, ParticlePreset preset, int count = 20);

    // Add a continuous emitter (returns emitter index)
    int  addEmitter(const ParticleEmitter& cfg);
    void removeEmitter(int idx);
    ParticleEmitter& emitter(int idx) { return m_emitters[idx]; }

    // Apply a named preset to an emitter config
    static ParticleEmitter makePreset(ParticlePreset type, glm::vec2 origin);

    // Update all alive particles
    void update(float dt);

    // Draw all alive particles using instanced rendering
    void draw(const Shader& shader, const Camera& camera);

    int aliveCount() const;

private:
    void spawnOne(const ParticleEmitter& cfg);
    int  findFreeSlot();

    Particle         m_particles[MAX_PARTICLES] = {};
    std::vector<ParticleEmitter> m_emitters;

    GLuint m_vao           = 0;
    GLuint m_quadVBO       = 0;
    GLuint m_quadEBO       = 0;
    GLuint m_instanceVBO   = 0;
    size_t m_instanceCap   = 0;

    std::vector<ParticleInstance> m_gpuData;
};

using ParticleManager = ParticleSystem;
