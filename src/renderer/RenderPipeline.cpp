#include "RenderPipeline.h"
#include "world/Chunk.h"
#include "world/TileRegistry.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>

// Atlas is ATLAS_GRID × ATLAS_GRID cells of TILE_PIXEL×TILE_PIXEL pixels
static constexpr int   ATLAS_COLS = ATLAS_GRID;
static constexpr int   ATLAS_ROWS = ATLAS_GRID;

// ---- Destructor -------------------------------------------------------------
RenderPipeline::~RenderPipeline() {
    if (m_fsVAO)  { glDeleteVertexArrays(1, &m_fsVAO);  m_fsVAO  = 0; }
    if (m_fsVBO)  { glDeleteBuffers(1, &m_fsVBO);       m_fsVBO  = 0; }
    if (m_fsEBO)  { glDeleteBuffers(1, &m_fsEBO);       m_fsEBO  = 0; }
    if (m_skyVAO) { glDeleteVertexArrays(1, &m_skyVAO); m_skyVAO = 0; }
    if (m_skyVBO) { glDeleteBuffers(1, &m_skyVBO);      m_skyVBO = 0; }
}

// ---- Init -------------------------------------------------------------------
void RenderPipeline::init(int viewW, int viewH) {
    m_vpW = viewW; m_vpH = viewH;

    // ---- UBOs ---------------------------------------------------------------
    m_cameraUBO.create(sizeof(CameraUBO), 0);
    m_frameUBO .create(sizeof(FrameDataUBO), 1);

    // ---- GBuffer: albedo/spec (RGBA8) + normal/emissive (RGB16F) + depth -----
    m_gBuffer.addColorAttachment(viewW, viewH, GL_RGBA8,   GL_RGBA,           GL_UNSIGNED_BYTE, GL_NEAREST);
    m_gBuffer.addColorAttachment(viewW, viewH, GL_RGB16F,  GL_RGB,            GL_FLOAT,         GL_NEAREST);
    m_gBuffer.addDepthAttachment(viewW, viewH);
    m_gBuffer.build();

    // ---- HDR composite FBO: RGBA16F -----------------------------------------
    m_hdrFBO.addColorAttachment(viewW, viewH, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR);
    m_hdrFBO.build();

    // ---- Sub-systems --------------------------------------------------------
    m_lightSys.init(viewW, viewH);
    m_postProc.init(viewW, viewH);
    // SpriteBatch initializes its GPU buffers in its constructor; no separate init needed.

    // ---- Fullscreen quad for composite passes --------------------------------
    buildFullscreenQuad();

    // ---- Sky background quad (simple gradient quad = fullscreen) -------------
    m_skyVAO = m_fsVAO;  // reuse FS quad
    m_skyVBO = m_fsVBO;

    // ---- Load shaders via ShaderLibrary -------------------------------------
    auto& sl = ShaderLibrary::instance();
    sl.load("sky",             "assets/shaders/bg.vert",         "assets/shaders/bg.frag");
    sl.load("tile_lit",        "assets/shaders/tile_lit.vert",   "assets/shaders/tile_lit.frag");
    sl.load("sprite",          "assets/shaders/sprite.vert",     "assets/shaders/sprite.frag");
    sl.load("occluder",        "assets/shaders/occluder.vert",   "assets/shaders/occluder.frag");
    sl.load("shadow_1d",       "assets/shaders/fullscreen.vert", "assets/shaders/shadow_1d.frag");
    sl.load("light_point",     "assets/shaders/fullscreen.vert", "assets/shaders/light_point.frag");
    sl.load("bloom_threshold", "assets/shaders/fullscreen.vert", "assets/shaders/bloom_threshold.frag");
    sl.load("bloom_blur",      "assets/shaders/fullscreen.vert", "assets/shaders/bloom_blur.frag");
    sl.load("composite",       "assets/shaders/fullscreen.vert", "assets/shaders/composite.frag");
    sl.load("tonemap",         "assets/shaders/fullscreen.vert", "assets/shaders/tonemap.frag");
    sl.load("particles",       "assets/shaders/particles.vert",  "assets/shaders/particles.frag");
    sl.load("ui",              "assets/shaders/ui.vert",         "assets/shaders/ui.frag");

    m_skyShader       = &sl.get("sky");
    m_tileLitShader   = &sl.get("tile_lit");
    m_spriteShader    = &sl.get("sprite");
    m_compositeShader = &sl.get("composite");
    m_uiShader        = &sl.get("ui");
}

void RenderPipeline::onResize(int w, int h) {
    m_vpW = w; m_vpH = h;
    m_gBuffer.resize(w, h);
    m_hdrFBO.resize(w, h);
    m_lightSys.onResize(w, h);
    m_postProc.onResize(w, h);
}

// ---- UBO uploads ------------------------------------------------------------
void RenderPipeline::uploadCameraUBO(const Camera& cam) {
    CameraUBO data;
    const glm::mat4& vp = cam.viewProj();
    for (int i = 0; i < 16; ++i) data.viewProj[i] = glm::value_ptr(vp)[i];
    data.position[0]  = cam.position().x;
    data.position[1]  = cam.position().y;
    data.zoom         = cam.zoom();
    data._pad0        = 0.f;
    data.viewport[0]  = cam.viewportWidth();
    data.viewport[1]  = cam.viewportHeight();
    m_cameraUBO.update(&data);
}

void RenderPipeline::uploadFrameDataUBO(float time, float dayTime, float sunlight, float /*wind*/) {
    FrameDataUBO data;
    data.time      = time;
    data.deltaTime = 0.f;
    data.dayTime   = dayTime;
    data.sunlight  = sunlight;
    data.screenW   = (float)m_vpW;
    data.screenH   = (float)m_vpH;
    m_frameUBO.update(&data);
}

// ---- Fullscreen quad --------------------------------------------------------
void RenderPipeline::buildFullscreenQuad() {
    float verts[] = { -1,-1, 1,-1, 1,1, -1,1 };
    uint32_t idx[] = {0,1,2, 0,2,3};

    glGenVertexArrays(1, &m_fsVAO);
    glGenBuffers(1, &m_fsVBO);
    glGenBuffers(1, &m_fsEBO);

    glBindVertexArray(m_fsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_fsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_fsEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
    glBindVertexArray(0);
}

void RenderPipeline::drawFullscreenQuad() {
    glBindVertexArray(m_fsVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

// ---- Main render ------------------------------------------------------------
void RenderPipeline::render(
    const Camera&           camera,
    const CameraController& camCtrl,
    World&                  world,
    EntityManager&          em,
    ParticleManager&        particles,
    UISystem&               ui,
    float                   dayTime,
    float                   sunlight,
    float                   time)
{
    // ---- Hot-reload shaders (polls file mtimes) ------------------------------
    ShaderLibrary::instance().pollHotReload();

    // ---- Update UBOs --------------------------------------------------------
    uploadCameraUBO(camera);
    uploadFrameDataUBO(time, dayTime, sunlight, 0.f);

    // ---- [1] Sky pass -------------------------------------------------------
    passSky(dayTime, sunlight, time);

    // ---- [2+3+5] Light system (occluder → shadow → light accumulation) ------
    m_lightSys.beginFrame(camera, sunlight);

    // Collect ECS lights
    em.view<CTransform, CLight>([&](EntityID, const CTransform& xf, const CLight& cl) {
        PointLight2D pl;
        pl.worldPos  = xf.pos;
        pl.color     = cl.color;
        pl.radius    = cl.radius;
        pl.intensity = cl.intensity;
        pl.castShadow= cl.castShadow;
        m_lightSys.addLight(pl);
    });

    // Occluder pass callback: render tiles with occluder shader
    m_lightSys.compute([&]() {
        auto& slInner = ShaderLibrary::instance();
        if (!slInner.has("occluder")) return;
        Shader& occ = slInner.get("occluder");

        occ.bind();
        const Texture& atlas = TileRegistry::instance().atlas();
        atlas.bind(0);
        occ.setInt("uAtlas", 0);
        occ.setVec2("uTileSize", {TILE_SIZE, TILE_SIZE});
        occ.setVec2("uAtlasCellSize", {ATLAS_CELL_SIZE, ATLAS_CELL_SIZE});

        // Draw only solid main-layer tiles
        for (auto& [coord, chunk] : world.chunks()) {
            const auto& insts = chunk->instances();
            if (insts.empty()) continue;
            m_tileRenderer.draw(insts, atlas, occ, camera, TILE_SIZE, ATLAS_CELL_SIZE);
        }
    });

    // ---- [4] GBuffer geometry pass ------------------------------------------
    passGBuffer(camera, world);

    // ---- Sprite entities pass (into GBuffer) --------------------------------
    passSprites(camera, em);

    // ---- [6] Particle pass (over GBuffer/HDR) --------------------------------
    passParticles(camera, particles);

    // ---- [7] Composite: GBuffer albedo × lights + emissive → HDR FBO --------
    passComposite(sunlight);

    // ---- [8] Post-process: bloom → tonemap → grade → effects → screen --------
    m_postProc.apply(m_hdrFBO.colorTexture(0).id(), m_postProc.settings());

    // ---- [9] UI pass (on top of everything) ---------------------------------
    passUI(ui, m_vpW, m_vpH);
}

// ---- Pass implementations ---------------------------------------------------

void RenderPipeline::passSky(float dayTime, float sunlight, float time) {
    if (!m_skyShader) return;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_vpW, m_vpH);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_skyShader->bind();
    m_skyShader->setFloat("uDayTime", dayTime);
    m_skyShader->setFloat("uSunlight", sunlight);
    m_skyShader->setFloat("uTime", time);

    drawFullscreenQuad();
    glEnable(GL_DEPTH_TEST);
}

void RenderPipeline::passGBuffer(const Camera& cam, World& world) {
    if (!m_tileLitShader) return;

    m_gBuffer.bind();
    glViewport(0, 0, m_vpW, m_vpH);
    GLenum drawBufs[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, drawBufs);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    m_tileLitShader->bind();
    const Texture& atlas = TileRegistry::instance().atlas();
    atlas.bind(0);
    m_tileLitShader->setInt("uAtlas", 0);
    m_tileLitShader->setVec2("uTileSize", {TILE_SIZE, TILE_SIZE});
    m_tileLitShader->setVec2("uAtlasCellSize", {ATLAS_CELL_SIZE, ATLAS_CELL_SIZE});

    // Draw 3 layers: wall (dim), main, deco
    for (auto& [coord, chunk] : world.chunks()) {
        // Wall layer (dim multiplier handled by instance light values)
        const auto& walls = chunk->wallInstances();
        if (!walls.empty())
            m_tileRenderer.draw(walls, atlas, *m_tileLitShader, cam, TILE_SIZE, ATLAS_CELL_SIZE);

        // Main layer
        const auto& mains = chunk->instances();
        if (!mains.empty())
            m_tileRenderer.draw(mains, atlas, *m_tileLitShader, cam, TILE_SIZE, ATLAS_CELL_SIZE);

        // Decoration layer
        const auto& decos = chunk->decoInstances();
        if (!decos.empty())
            m_tileRenderer.draw(decos, atlas, *m_tileLitShader, cam, TILE_SIZE, ATLAS_CELL_SIZE);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPipeline::passSprites(const Camera& cam, EntityManager& em) {
    if (!m_spriteShader) return;

    m_gBuffer.bind();
    GLenum drawBufs[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, drawBufs);

    m_spriteBatch.begin(cam, *m_spriteShader);

    const Texture& atlas = TileRegistry::instance().atlas();

    em.view<CTransform, CSprite>([&](EntityID, const CTransform& xf, const CSprite& sp) {
        if (!sp.visible) return;
        // Use the tile atlas for sprite rendering when texID matches it,
        // otherwise pass nullptr (solid colour tint only)
        const Texture* tex  = (sp.texID != 0) ? &atlas : nullptr;
        const Texture* norm = nullptr;
        m_spriteBatch.submit(
            tex, norm,
            xf.pos, sp.size,
            sp.uvMin, sp.uvMax,
            sp.tint, sp.emissive,
            sp.flipX
        );
    });

    m_spriteBatch.end();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPipeline::passComposite(float sunlight) {
    if (!m_compositeShader) return;

    m_hdrFBO.bind();
    glViewport(0, 0, m_vpW, m_vpH);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_compositeShader->bind();

    // GBuffer textures
    m_gBuffer.bindColorTex(0, 0);   // albedo/spec → slot 0
    m_gBuffer.bindColorTex(1, 1);   // normal/emissive → slot 1
    m_compositeShader->setInt("uGAlbedo",       0);
    m_compositeShader->setInt("uGNormEmissive", 1);

    // Light accumulation buffer
    m_lightSys.bindLightTex(2);
    m_compositeShader->setInt("uLightBuffer", 2);

    // Ambient sunlight
    m_compositeShader->setFloat("uSunlight", sunlight);

    drawFullscreenQuad();
    m_compositeShader->unbind();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
}

void RenderPipeline::passParticles(const Camera& cam, ParticleManager& particles) {
    auto& sl = ShaderLibrary::instance();
    if (!sl.has("particles")) return;
    Shader& ps = sl.get("particles");

    // Particles rendered additively over the current back-buffer
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive
    glDisable(GL_DEPTH_TEST);
    particles.draw(ps, cam);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void RenderPipeline::passUI(UISystem& ui, int w, int h) {
    if (!m_uiShader) return;
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ui.endFrame(m_uiShader->id());

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
