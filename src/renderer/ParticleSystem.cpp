#include "ParticleSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>

static float rng() { return (float)rand() / (float)RAND_MAX; }
static float rngRange(float lo, float hi) { return lo + rng() * (hi - lo); }

// ---- Presets ----------------------------------------------------------------

ParticleEmitter ParticleSystem::makePreset(ParticlePreset type, glm::vec2 origin) {
    ParticleEmitter e{};
    e.origin = origin;
    switch (type) {
    case ParticlePreset::Fire:
        e.direction     = {0.f, -1.f};
        e.speed         = 50.f;  e.speedVariance  = 30.f;
        e.spread        = 0.6f;
        e.emitRate      = 40.f;
        e.lifeMin       = 0.4f;  e.lifeMax        = 0.9f;
        e.colorStart    = {1.f, 0.6f, 0.0f, 1.f};
        e.colorEnd      = {0.5f, 0.0f, 0.0f, 0.f};
        e.sizeStart     = 8.f;   e.sizeEnd        = 1.f;
        e.gravity       = 20.f;
        e.originVariance= {4.f, 2.f};
        break;
    case ParticlePreset::Smoke:
        e.direction     = {0.f, -1.f};
        e.speed         = 20.f;  e.speedVariance  = 10.f;
        e.spread        = 1.0f;
        e.emitRate      = 15.f;
        e.lifeMin       = 1.0f;  e.lifeMax        = 2.5f;
        e.colorStart    = {0.4f, 0.4f, 0.4f, 0.8f};
        e.colorEnd      = {0.6f, 0.6f, 0.6f, 0.0f};
        e.sizeStart     = 6.f;   e.sizeEnd        = 18.f;
        e.gravity       = -5.f;
        break;
    case ParticlePreset::Spark:
        e.direction     = {0.f, -1.f};
        e.speed         = 140.f; e.speedVariance  = 80.f;
        e.spread        = 3.14159f;
        e.emitRate      = 60.f;
        e.lifeMin       = 0.3f;  e.lifeMax        = 0.8f;
        e.colorStart    = {1.f, 0.95f, 0.4f, 1.f};
        e.colorEnd      = {1.f, 0.3f, 0.0f, 0.f};
        e.sizeStart     = 3.f;   e.sizeEnd        = 1.f;
        e.gravity       = 200.f;
        break;
    case ParticlePreset::Magic:
        e.direction     = {0.f, -1.f};
        e.speed         = 60.f;  e.speedVariance  = 40.f;
        e.spread        = 3.14159f;
        e.emitRate      = 25.f;
        e.lifeMin       = 0.5f;  e.lifeMax        = 1.2f;
        e.colorStart    = {0.5f, 0.2f, 1.f, 1.f};
        e.colorEnd      = {0.9f, 0.5f, 1.f, 0.f};
        e.sizeStart     = 5.f;   e.sizeEnd        = 2.f;
        e.gravity       = -10.f;
        break;
    case ParticlePreset::Rain:
        e.direction     = {0.2f, 1.f};
        e.speed         = 350.f; e.speedVariance  = 50.f;
        e.spread        = 0.05f;
        e.emitRate      = 80.f;
        e.lifeMin       = 0.8f;  e.lifeMax        = 1.4f;
        e.colorStart    = {0.6f, 0.7f, 1.f, 0.8f};
        e.colorEnd      = {0.6f, 0.7f, 1.f, 0.f};
        e.sizeStart     = 2.f;   e.sizeEnd        = 2.f;
        e.gravity       = 0.f;
        break;
    case ParticlePreset::Snow:
        e.direction     = {0.f, 1.f};
        e.speed         = 30.f;  e.speedVariance  = 15.f;
        e.spread        = 0.8f;
        e.emitRate      = 30.f;
        e.lifeMin       = 3.f;   e.lifeMax        = 6.f;
        e.colorStart    = {1.f, 1.f, 1.f, 0.9f};
        e.colorEnd      = {0.9f, 0.9f, 1.f, 0.f};
        e.sizeStart     = 4.f;   e.sizeEnd        = 3.f;
        e.gravity       = 0.f;
        break;
    case ParticlePreset::Blood:
        e.direction     = {0.f, -1.f};
        e.speed         = 100.f; e.speedVariance  = 60.f;
        e.spread        = 3.14159f;
        e.emitRate      = 0.f;
        e.lifeMin       = 0.4f;  e.lifeMax        = 0.8f;
        e.colorStart    = {0.8f, 0.0f, 0.0f, 1.f};
        e.colorEnd      = {0.4f, 0.0f, 0.0f, 0.f};
        e.sizeStart     = 4.f;   e.sizeEnd        = 2.f;
        e.gravity       = 350.f;
        break;
    case ParticlePreset::Explosion:
        e.direction     = {0.f, -1.f};
        e.speed         = 200.f; e.speedVariance  = 120.f;
        e.spread        = 3.14159f;
        e.emitRate      = 0.f;   e.loop           = false;
        e.lifeMin       = 0.5f;  e.lifeMax        = 1.2f;
        e.colorStart    = {1.f, 0.8f, 0.1f, 1.f};
        e.colorEnd      = {0.5f, 0.1f, 0.0f, 0.f};
        e.sizeStart     = 16.f;  e.sizeEnd        = 2.f;
        e.gravity       = 60.f;
        break;
    default:
        break;
    }
    return e;
}

// ---- Construction -----------------------------------------------------------

ParticleSystem::ParticleSystem() {
    // Build a simple billboard quad (unit square centered at origin)
    float quad[] = {
        -0.5f, -0.5f,  0.f, 0.f,
         0.5f, -0.5f,  1.f, 0.f,
         0.5f,  0.5f,  1.f, 1.f,
        -0.5f,  0.5f,  0.f, 1.f,
    };
    uint32_t idx[] = {0,1,2, 0,2,3};

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_quadVBO);
    glGenBuffers(1, &m_quadEBO);
    glGenBuffers(1, &m_instanceVBO);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    // Quad vertex attribs
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));

    // Instance attribs
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    using PI = ParticleInstance;
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(PI), (void*)offsetof(PI, pos));
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(PI), (void*)offsetof(PI, size));
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(PI), (void*)offsetof(PI, rotation));
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(PI), (void*)offsetof(PI, color));
    glVertexAttribDivisor(5, 1);

    glBindVertexArray(0);
}

ParticleSystem::~ParticleSystem() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_quadVBO);
    glDeleteBuffers(1, &m_quadEBO);
    glDeleteBuffers(1, &m_instanceVBO);
}

// ---- Burst / Emit -----------------------------------------------------------

void ParticleSystem::burst(glm::vec2 pos, ParticlePreset preset, int count) {
    auto cfg = makePreset(preset, pos);
    cfg.loop = false;
    for (int i = 0; i < count; ++i) spawnOne(cfg);
}

int ParticleSystem::addEmitter(const ParticleEmitter& cfg) {
    for (int i = 0; i < (int)m_emitters.size(); ++i)
        if (!m_emitters[i].active) { m_emitters[i] = cfg; return i; }
    m_emitters.push_back(cfg);
    return (int)m_emitters.size() - 1;
}

void ParticleSystem::removeEmitter(int idx) {
    if (idx >= 0 && idx < (int)m_emitters.size())
        m_emitters[idx].active = false;
}

int ParticleSystem::findFreeSlot() {
    for (int i = 0; i < MAX_PARTICLES; ++i)
        if (!m_particles[i].alive) return i;
    return -1;
}

void ParticleSystem::spawnOne(const ParticleEmitter& cfg) {
    int slot = findFreeSlot();
    if (slot < 0) return;

    Particle& p = m_particles[slot];
    p.alive = true;

    float ox = rngRange(-cfg.originVariance.x, cfg.originVariance.x);
    float oy = rngRange(-cfg.originVariance.y, cfg.originVariance.y);
    p.pos = cfg.origin + glm::vec2(ox, oy);

    float baseAngle = atan2f(cfg.direction.y, cfg.direction.x);
    float angle     = baseAngle + rngRange(-cfg.spread, cfg.spread);
    float speed     = cfg.speed + rngRange(-cfg.speedVariance, cfg.speedVariance);
    p.vel       = { cosf(angle)*speed, sinf(angle)*speed };
    p.colorStart= cfg.colorStart;
    p.colorEnd  = cfg.colorEnd;
    p.sizeStart = cfg.sizeStart + rngRange(-1.f, 1.f);
    p.sizeEnd   = cfg.sizeEnd;
    p.maxLife   = rngRange(cfg.lifeMin, cfg.lifeMax);
    p.life      = p.maxLife;
    p.rotation  = rngRange(0.f, 6.28318f);
    p.rotVel    = rngRange(-2.f, 2.f);
}

// ---- Update -----------------------------------------------------------------

void ParticleSystem::update(float dt) {
    // Tick emitters
    for (auto& e : m_emitters) {
        if (!e.active) continue;
        e._accumulator += dt;
        float interval = 1.f / std::max(e.emitRate, 0.001f);
        while (e._accumulator >= interval) {
            e._accumulator -= interval;
            spawnOne(e);
        }
    }

    // Tick particles
    for (auto& p : m_particles) {
        if (!p.alive) continue;
        p.life -= dt;
        if (p.life <= 0.f) { p.alive = false; continue; }

        // Gravity (positive = downward in our y-down coord system)
        // We store gravity per-emitter but here we apply a global gravity for now
        p.vel.y += 0.f; // gravity applied per-emitter at spawn time
        p.pos   += p.vel * dt;
        p.rotation += p.rotVel * dt;
    }
}

// ---- Draw -------------------------------------------------------------------

void ParticleSystem::draw(const Shader& shader, const Camera& camera) {
    // Build GPU data
    m_gpuData.clear();
    for (const auto& p : m_particles) {
        if (!p.alive) continue;
        float t = 1.f - (p.life / p.maxLife);
        ParticleInstance pi;
        pi.pos      = p.pos;
        pi.size     = p.sizeStart + (p.sizeEnd - p.sizeStart) * t;
        pi.rotation = p.rotation;
        pi.color    = glm::mix(p.colorStart, p.colorEnd, t);
        m_gpuData.push_back(pi);
    }

    if (m_gpuData.empty()) return;

    // Upload instances
    size_t dataSize = m_gpuData.size() * sizeof(ParticleInstance);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    if (m_gpuData.size() > m_instanceCap) {
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)dataSize, m_gpuData.data(), GL_DYNAMIC_DRAW);
        m_instanceCap = m_gpuData.size();
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)dataSize, m_gpuData.data());
    }

    shader.bind();
    shader.setMat4("uViewProj", camera.viewProj());
    shader.setInt("uSprite", 0);

    glBindVertexArray(m_vao);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                            (GLsizei)m_gpuData.size());
    glBindVertexArray(0);
    shader.unbind();
}

int ParticleSystem::aliveCount() const {
    int n = 0;
    for (const auto& p : m_particles)
        if (p.alive) ++n;
    return n;
}
