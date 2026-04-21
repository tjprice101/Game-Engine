#include "CameraController.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

// Simple pseudo-random hash for shake direction variation
static float hashf(float t) {
    return std::sin(t * 127.1f + 311.7f) * 0.5f + 0.5f;
}

glm::vec2 CameraController::computeShake(float dt) {
    if (m_trauma <= 0.f) {
        m_shakeOffset = {0.f, 0.f};
        return m_shakeOffset;
    }

    m_trauma = std::max(0.f, m_trauma - dt * 1.5f); // decay rate
    m_shakeTimer += dt;

    float amplitude = m_trauma * m_trauma; // squared gives better feel
    float maxShake  = 12.f; // max pixel offset

    m_shakeOffset = {
        (hashf(m_shakeTimer * 20.f)       * 2.f - 1.f) * amplitude * maxShake,
        (hashf(m_shakeTimer * 20.f + 50.f)* 2.f - 1.f) * amplitude * maxShake
    };
    return m_shakeOffset;
}

glm::vec2 CameraController::clampToWorld(glm::vec2 pos, glm::vec2 half) const {
    pos.x = std::clamp(pos.x, worldMin.x + half.x, worldMax.x - half.x);
    pos.y = std::clamp(pos.y, worldMin.y + half.y, worldMax.y - half.y);
    return pos;
}

void CameraController::update(float dt, EntityManager& em, EntityID target, Camera& camera) {
    // ---- Zoom spring --------------------------------------------------------
    float zoomErr    = targetZoom - m_currentZoom;
    m_currentZoom   += zoomErr * zoomSpeed * dt;

    // ---- Letterbox spring ---------------------------------------------------
    float lbErr = targetLetterbox - letterbox;
    letterbox  += lbErr * letterboxSpeed * dt;

    // ---- Follow target ------------------------------------------------------
    glm::vec2 desired = m_currentPos;

    if (target != NULL_ENTITY && em.isAlive(target)) {
        if (em.has<CTransform>(target)) {
            const CTransform& xf  = em.get<CTransform>(target);
            glm::vec2 followOffset = {0.f, -16.f};

            if (em.has<CCameraFollow>(target)) {
                const CCameraFollow& cf = em.get<CCameraFollow>(target);
                followOffset = cf.offset;

                glm::vec2 diff = (xf.pos + followOffset) - m_currentPos;

                // Deadzone: only spring-follow if outside deadzone
                bool inDZ = std::abs(diff.x) < cf.deadzoneW
                         && std::abs(diff.y) < cf.deadzoneH;

                if (!inDZ) {
                    // Spring constant proportional to distance for snappier feel
                    float stiffness = cf.springStiffness;
                    m_vel += (diff * stiffness - m_vel * 3.f) * dt;
                }
            } else {
                // Default: direct spring follow
                glm::vec2 diff = (xf.pos + followOffset) - m_currentPos;
                m_vel += (diff * 8.f - m_vel * 4.f) * dt;
            }
        }
    }

    m_currentPos += m_vel * dt;

    // ---- World bounds clamp -------------------------------------------------
    float w = camera.viewportWidth()  / m_currentZoom;
    float h = camera.viewportHeight() / m_currentZoom;
    m_currentPos = clampToWorld(m_currentPos, {w * 0.5f, h * 0.5f});

    // ---- Screen shake -------------------------------------------------------
    glm::vec2 shake = computeShake(dt);
    glm::vec2 finalPos = m_currentPos + shake;

    // ---- Apply to Camera ----------------------------------------------------
    camera.setPosition(finalPos);
    camera.setZoom(m_currentZoom);
}

void CameraController::addTrauma(float amount) {
    m_trauma = std::min(1.f, m_trauma + amount);
}

void CameraController::snapTo(glm::vec2 worldPos, Camera& camera) {
    m_currentPos  = worldPos;
    m_vel         = {0.f, 0.f};
    camera.setPosition(worldPos);
}

void CameraController::addParallaxLayer(ParallaxLayer layer) {
    m_parallax.push_back(layer);
    // Keep sorted by zOrder
    std::sort(m_parallax.begin(), m_parallax.end(),
        [](const ParallaxLayer& a, const ParallaxLayer& b){ return a.zOrder < b.zOrder; });
}
