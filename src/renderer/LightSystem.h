#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include "renderer/Shader.h"
#include "renderer/RenderTarget.h"
#include "renderer/Camera.h"

// ---- 2D Deferred Lighting System -------------------------------------------
//
// Pipeline:
//   1. Occluder pass: render solid tiles as white → occluderFBO
//   2. Shadow pass:   for each light → 512x1 polar shadow map
//   3. Light pass:    for each light → accumulate into HDR lightFBO (additive)
//
// The result (lightFBO) is later combined with the geometry GBuffer in the
// composite pass: finalColor = albedo * lighting + emissive
//
// Supports up to MAX_LIGHTS dynamic point lights per frame.

static constexpr int MAX_LIGHTS         = 64;
static constexpr int SHADOW_MAP_RES     = 512;  // columns in the 1D shadow map

struct PointLight2D {
    glm::vec2 worldPos;           // centre in world pixels
    glm::vec3 color     = {1,1,1};
    float     radius    = 160.f;  // pixels
    float     intensity = 1.f;
    bool      castShadow= true;
    bool      active    = true;
};

class LightSystem {
public:
    LightSystem()  = default;
    ~LightSystem() = default;

    LightSystem(const LightSystem&)            = delete;
    LightSystem& operator=(const LightSystem&) = delete;

    void init(int viewW, int viewH);
    void onResize(int w, int h);

    // ---- Per-frame API ------------------------------------------------------

    // Call with the SAME camera used to render geometry
    // sunlight: 0..1 ambient contribution
    void beginFrame(const Camera& cam, float sunlight);

    // Add a dynamic light this frame (cleared next frame)
    void addLight(const PointLight2D& light);

    // Execute occluder + shadow + light accumulation passes
    // Requires:
    //   tileVAO / solidInstances → rendered into occluder FBO
    // You pass a lambda that draws scene geometry into the currently-bound FBO
    void compute(std::function<void()> drawOccluders);

    // Bind the light accumulation texture to a slot for the composite shader
    void bindLightTex(int slot = 1) const;

    // ---- Accessors ----------------------------------------------------------
    RenderTarget& lightFBO()    { return m_lightFBO;    }
    RenderTarget& occluderFBO() { return m_occluderFBO; }

    int lightCount() const { return (int)m_lights.size(); }

private:
    void buildFullscreenQuad();
    void occluderPass(std::function<void()> drawOccluders);
    void shadowPass  (const PointLight2D& light, int idx);
    void lightPass   (const PointLight2D& light, int idx);

    // Shaders (loaded via ShaderLibrary, cached here)
    Shader* m_shadowShader = nullptr;
    Shader* m_lightShader  = nullptr;

    // Render targets
    RenderTarget m_occluderFBO;  // RGBA8 – white=solid, black=air
    RenderTarget m_lightFBO;     // RGBA16F HDR light accumulation

    // Per-light 1D shadow maps (pooled)
    std::array<GLuint, MAX_LIGHTS> m_shadowTex{};   // 1D shadow map textures
    std::array<GLuint, MAX_LIGHTS> m_shadowFBO{};

    // Active lights this frame
    std::vector<PointLight2D> m_lights;

    // Fullscreen quad for post-process passes
    GLuint m_fsVAO = 0, m_fsVBO = 0, m_fsEBO = 0;

    // Light influence quad
    GLuint m_lightVAO = 0, m_lightVBO = 0, m_lightEBO = 0;

    const Camera* m_camera   = nullptr;
    float         m_sunlight = 1.f;
    int           m_vpW = 0, m_vpH = 0;
};
