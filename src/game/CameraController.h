#pragma once
#include "renderer/Camera.h"
#include "ecs/ECS.h"
#include "ecs/Components.h"
#include <glm/glm.hpp>
#include <vector>

// ---- Parallax layer ---------------------------------------------------------
struct ParallaxLayer {
    uint32_t  texID      = 0;    // texture to render (0 = solid colour)
    glm::vec4 tint       = {1,1,1,1};
    float     scrollX    = 0.3f; // 0=pinned to world, 1=locked to camera
    float     scrollY    = 0.1f;
    float     scaleX     = 1.f;
    float     scaleY     = 1.f;
    int       zOrder     = -100; // negative = behind world
};

// ---- CameraController -------------------------------------------------------
// Manages camera follow, screen shake, parallax, letterbox, zoom transitions.
// Designed to operate on the engine Camera object (Camera2D / CameraUBO).

class CameraController {
public:
    // ---- Configuration -------------------------------------------------------
    float minZoom    = 0.5f;
    float maxZoom    = 4.0f;
    float targetZoom = 1.0f;   // desired zoom (animated toward with spring)
    float zoomSpeed  = 5.0f;   // spring constant for zoom

    // Cinematic letterbox (0=off, 1=full bars)
    float targetLetterbox = 0.f;
    float letterbox       = 0.f;
    float letterboxSpeed  = 3.f;

    // World bounds (camera won't show outside)
    glm::vec2 worldMin = {-1e9f, -1e9f};
    glm::vec2 worldMax = { 1e9f,  1e9f};

    // ---- Public API ----------------------------------------------------------

    // Called once per frame. `target` can be NULL_ENTITY for free-cam.
    void update(float dt, EntityManager& em, EntityID target, Camera& camera);

    // Trauma-based screen shake (addTrauma(0..1); decays automatically)
    void addTrauma(float amount);

    // Instant camera warp (no spring follow for one frame)
    void snapTo(glm::vec2 worldPos, Camera& camera);

    // Register a parallax layer (rendered by RenderPipeline)
    void addParallaxLayer(ParallaxLayer layer);
    const std::vector<ParallaxLayer>& parallaxLayers() const { return m_parallax; }

    // Zoom to a point with spring interpolation
    void zoomTo(float zoom) { targetZoom = glm::clamp(zoom, minZoom, maxZoom); }

    // Get the current shake offset for rendering
    glm::vec2 shakeOffset() const { return m_shakeOffset; }

    // Letterbox fraction (0..1), used by RenderPipeline to draw black bars
    float letterboxFraction() const { return letterbox * 0.12f; } // 12% of height max

private:
    // Spring-follow state
    glm::vec2 m_vel        = {0.f, 0.f};  // camera velocity (spring)
    glm::vec2 m_currentPos = {0.f, 0.f};  // actual camera centre in world

    // Screen shake
    float     m_trauma     = 0.f;         // 0..1; squared = shake amplitude
    float     m_shakeTimer = 0.f;
    glm::vec2 m_shakeOffset = {0.f, 0.f};

    // Zoom spring
    float m_currentZoom = 1.f;

    std::vector<ParallaxLayer> m_parallax;

    glm::vec2 computeShake(float dt);
    glm::vec2 clampToWorld(glm::vec2 pos, glm::vec2 viewHalfSize) const;
};
