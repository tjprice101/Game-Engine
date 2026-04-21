#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <functional>
#include <vector>

#include "renderer/Camera.h"
#include "renderer/RenderTarget.h"
#include "renderer/LightSystem.h"
#include "renderer/PostProcess.h"
#include "renderer/SpriteBatch.h"
#include "renderer/ParticleSystem.h"
#include "renderer/TileRenderer.h"
#include "renderer/UniformBuffer.h"
#include "renderer/ShaderLibrary.h"
#include "game/CameraController.h"
#include "game/UISystem.h"
#include "world/World.h"
#include "ecs/ECS.h"
#include "ecs/Components.h"

// ---- RenderPipeline ---------------------------------------------------------
// Orchestrates all rendering passes in order:
//   [1] Sky/BG pass
//   [2] Occluder pass (into LightSystem)
//   [3] Shadow pass (inside LightSystem::compute)
//   [4] Geometry pass → GBuffer
//   [5] Light accumulation pass (inside LightSystem::compute)
//   [6] Particle pass
//   [7] Composite pass → HDR FBO
//   [8] Post-process (bloom, tonemap, grade, CRT, vignette, FXAA)
//   [9] UI pass

class RenderPipeline {
public:
    RenderPipeline() = default;
    ~RenderPipeline();

    RenderPipeline(const RenderPipeline&)            = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    // ---- Init / resize ------------------------------------------------------
    void init(int viewW, int viewH);
    void onResize(int viewW, int viewH);

    // ---- Per-frame rendering ------------------------------------------------
    // Call this once per frame with all required scene data.
    // `drawOccluders` is a callback that renders solid geometry into the
    //  currently bound FBO (used by LightSystem for shadow generation).
    void render(
        const Camera&          camera,
        const CameraController& camCtrl,
        World&                 world,
        EntityManager&         em,
        ParticleManager&       particles,
        UISystem&              ui,
        float                  dayTime,
        float                  sunlight,
        float                  time
    );

    // ---- Sub-system access --------------------------------------------------
    LightSystem&          lightSystem()   { return m_lightSys;  }
    PostProcess&          postProcess()   { return m_postProc;  }
    PostProcessSettings&  ppSettings()    { return m_postProc.settings(); }
    SpriteBatch&          spriteBatch()   { return m_spriteBatch; }

    // ---- Debug --------------------------------------------------------------
    bool wireframe      = false;
    bool showLightDebug = false;

private:
    // ---- Passes -------------------------------------------------------------
    void passSky   (float dayTime, float sunlight, float time);
    void passGBuffer(const Camera& cam, World& world);
    void passSprites(const Camera& cam, EntityManager& em);
    void passComposite(float sunlight);
    void passParticles(const Camera& cam, ParticleManager& particles);
    void passUI(UISystem& ui, int w, int h);

    // ---- Helpers ------------------------------------------------------------
    void buildFullscreenQuad();
    void drawFullscreenQuad();
    void uploadCameraUBO(const Camera& cam);
    void uploadFrameDataUBO(float time, float dayTime, float sunlight, float wind);

    // ---- Sub-systems --------------------------------------------------------
    LightSystem    m_lightSys;
    PostProcess    m_postProc;
    SpriteBatch    m_spriteBatch;
    TileRenderer   m_tileRenderer;

    // ---- Render targets -----------------------------------------------------
    RenderTarget   m_gBuffer;   // attachment 0=albedo/spec (RGBA8), 1=normal/emissive (RGB16F)
    RenderTarget   m_hdrFBO;    // RGBA16F HDR composite (before post-process)

    // ---- UBOs ---------------------------------------------------------------
    UniformBuffer  m_cameraUBO;
    UniformBuffer  m_frameUBO;

    // ---- Fullscreen quad ----------------------------------------------------
    GLuint m_fsVAO = 0, m_fsVBO = 0, m_fsEBO = 0;

    // ---- Shaders (cached after ShaderLibrary load) --------------------------
    Shader* m_skyShader       = nullptr;
    Shader* m_tileLitShader   = nullptr;
    Shader* m_spriteShader    = nullptr;
    Shader* m_compositeShader = nullptr;
    Shader* m_uiShader        = nullptr;

    // ---- Viewport -----------------------------------------------------------
    int m_vpW = 0, m_vpH = 0;

    // ---- Sky background VBO (simple gradient quad) --------------------------
    GLuint m_skyVAO = 0, m_skyVBO = 0;
};
