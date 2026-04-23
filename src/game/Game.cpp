#include "Game.h"
#include "core/Input.h"
#include "world/TileRegistry.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

// ---- Construction -----------------------------------------------------------

Game::Game(const WindowConfig& cfg)
    : m_window(cfg)
    , m_world(42u)
    , m_player({0.f, 0.f})
{
    Input::init(m_window.handle());
    m_window.setResizeCallback([this](int w, int h){
        m_pipeline.onResize(w, h);
        m_camera.setViewport(float(w), float(h));
    });

    m_camera.setViewport(float(cfg.width), float(cfg.height));
    m_camera.setZoom(2.0f);

    // Build tile atlas before pipeline init (pipeline loads shaders that sample it)
    TileRegistry::instance().buildAtlas();

    // Init rendering pipeline (also inits trails and beams)
    m_pipeline.init(cfg.width, cfg.height);

    // ---- VFX system init ----------------------------------------------------
    ThemeRegistry::instance().loadDefaults();
    m_vfx.init(&m_particles,
               &m_pipeline.trailRenderer(),
               &m_pipeline.beamRenderer(),
               &m_screenFX);
    // Start with MoonlightSonata theme
    m_vfx.setTheme(ThemeRegistry::instance().active());
    applyThemeToPostProcess(ThemeRegistry::instance().active());

    // Camera controller defaults
    m_camCtrl.minZoom    = 0.5f;
    m_camCtrl.maxZoom    = 6.0f;
    m_camCtrl.targetZoom = 2.0f;

    // Generate spawn area
    for (int cx = -LOAD_RADIUS_X; cx <= LOAD_RADIUS_X; ++cx)
        for (int cy = -1; cy <= LOAD_RADIUS_Y; ++cy)
            m_world.getOrCreateChunk(cx, cy);

    // Spawn player on surface
    int   spawnSurface = m_world.surfaceY(0);
    float spawnPx      = (spawnSurface - 3) * TILE_SIZE - PLAYER_H;
    m_player = Player({-PLAYER_W * 0.5f, spawnPx});

    // Register player as ECS entity with relevant components
    m_playerEntity = m_em.create();
    auto& xf  = m_em.add<CTransform>(m_playerEntity);
    xf.pos    = m_player.center();
    auto& cf  = m_em.add<CCameraFollow>(m_playerEntity);
    cf.offset = {0.f, -20.f};
    m_em.add<CTag>(m_playerEntity).isPlayer = true;
    m_em.add<CHealth>(m_playerEntity);

    // Centre camera on player immediately
    m_camCtrl.snapTo(m_player.center(), m_camera);
}

// ---- Main loop --------------------------------------------------------------

void Game::run() {
    m_timer.setFixedStep(60.0);

    while (!m_window.shouldClose() && m_running) {
        m_window.pollEvents();
        Input::update();
        m_timer.tick();

        while (m_timer.consumeFixedStep())
            update(static_cast<float>(m_timer.fixedStep()));

        renderFrame();
        m_window.swapBuffers();
    }
}

// ---- Update -----------------------------------------------------------------

void Game::update(float dt) {
    m_totalTime += dt;

    // ---- Day/night cycle ------------------------------------------------
    m_dayTime += dt / m_dayDuration;
    if (m_dayTime > 1.f) m_dayTime -= 1.f;
    m_sunlight = computeSunlight();

    // ---- Global input ---------------------------------------------------
    if (Input::keyPressed(Key::Escape)) m_running = false;
    if (Input::keyPressed(Key::E))      m_showInventory = !m_showInventory;

    // Hotbar slots 1-9, 0
    int hotbarKey = -1;
    for (int k = 0; k < 9; ++k)
        if (Input::keyPressed(Key::Num1 + k)) hotbarKey = k;
    if (Input::keyPressed(Key::Num0)) hotbarKey = 9;

    // Zoom with scroll + Ctrl
    if (Input::keyDown(Key::LeftCtrl)) {
        float s = Input::scrollDelta();
        if (s != 0.f) {
            float z = m_camCtrl.targetZoom * (s > 0.f ? 1.1f : 0.9f);
            m_camCtrl.zoomTo(z);
        }
    }

    // ---- Player update --------------------------------------------------
    glm::vec2 mouseScreen = Input::mousePos();
    glm::vec2 mouseWorld  = m_camera.screenToWorld(mouseScreen);
    bool leftHeld         = Input::mouseDown(MouseButton::Left);
    bool rightPress       = Input::mousePressed(MouseButton::Right);
    bool jumpPress        = Input::keyDown(Key::Space);

    m_player.update(dt, m_world, mouseWorld, leftHeld, rightPress, jumpPress, hotbarKey);

    // Sync player position to ECS transform
    if (m_em.isAlive(m_playerEntity)) {
        m_em.get<CTransform>(m_playerEntity).pos = m_player.center();
    }

    // ---- ECS health update ----------------------------------------------
    m_em.view<CHealth>([&](EntityID, CHealth& hp) {
        hp.invincibilityTimer = std::max(0.f, hp.invincibilityTimer - dt);
        hp.mana = std::min(hp.maxMana, hp.mana + hp.manaRegen * dt);
    });

    // ---- Particle update -----------------------------------------------
    m_particles.update(dt);
    m_screenFX.update(dt);

    // Spawn particles from ECS emitters
    m_em.view<CTransform, CParticleEmitter>([&](EntityID, const CTransform& xf, const CParticleEmitter& pe) {
        if (!pe.active) return;
        m_particles.addEmitter(ParticleManager::makePreset(pe.preset, xf.pos + glm::vec2{pe.offsetX, pe.offsetY}));
    });

    // ---- Camera update --------------------------------------------------
    m_camCtrl.update(dt, m_em, m_playerEntity, m_camera);

    // ---- Chunk management -----------------------------------------------
    loadChunksAroundCamera();
    rebuildDirtyChunks();

    // ---- Fluid simulation (10 Hz) ----------------------------------------
    static float fluidAcc = 0.f;
    fluidAcc += dt;
    if (fluidAcc >= 0.1f) {
        fluidAcc -= 0.1f;
        m_world.tickFluids();
    }

    // ---- UI state -------------------------------------------------------
    m_ui.updateDamageNumbers(dt);
}

// ---- Chunk management -------------------------------------------------------

void Game::loadChunksAroundCamera() {
    int camCX = World::tileToChunkCoord(World::toTileX(m_camera.position().x));
    int camCY = World::tileToChunkCoord(World::toTileY(m_camera.position().y));

    for (int dx = -LOAD_RADIUS_X; dx <= LOAD_RADIUS_X; ++dx)
        for (int dy = -1; dy <= LOAD_RADIUS_Y; ++dy)
            m_world.getOrCreateChunk(camCX + dx, camCY + dy);
}

void Game::rebuildDirtyChunks() {
    // Build instanced render data for any chunk flagged dirty.
    // Light values: use a simple flat ambient grid (no BFS) since LightSystem
    // handles per-pixel dynamic lighting in the deferred pipeline.
    glm::vec3 lightBuf[CHUNK_SIZE][CHUNK_SIZE];

    float ambient = m_sunlight;
    glm::vec3 ambientRGB = glm::vec3(ambient);

    for (auto& [coord, chunk] : m_world.chunks()) {
        if (!chunk->dirty()) continue;

        // Fill light buffer with uniform ambient; dynamic lights handled by GPU
        for (int y = 0; y < CHUNK_SIZE; ++y)
            for (int x = 0; x < CHUNK_SIZE; ++x)
                lightBuf[y][x] = ambientRGB;

        chunk->buildInstances(lightBuf);
    }
}

// ---- Render -----------------------------------------------------------------

void Game::renderFrame() {
    // Prepare UI for this frame
    int w = (int)m_camera.viewportWidth();
    int h = (int)m_camera.viewportHeight();
    m_ui.beginFrame(w, h);

    // Draw HUD elements
    m_ui.drawHotbar(m_player.inventory(), w, h, TileRegistry::instance().atlas().id());

    // Health / mana bars
    if (m_em.isAlive(m_playerEntity) && m_em.has<CHealth>(m_playerEntity)) {
        const CHealth& hp = m_em.get<CHealth>(m_playerEntity);
        m_ui.drawHealthMana(hp.hpFraction(), hp.manaFraction(), w, h);
    }

    m_ui.drawCrosshair(w, h);
    m_ui.drawDebugStats(
        m_timer.delta() > 0.0 ? (float)(1.0 / m_timer.delta()) : 0.f,
        (int)m_em.entityCount(),
        0,
        w
    );

    if (m_showInventory) {
        m_ui.drawInventoryGrid(m_player.inventory(), w, h,
                               TileRegistry::instance().atlas().id());
    }

    // ---- Apply screen effects to camera (shake offset) ----------------------
    ScreenEffectParams fx = m_screenFX.params();
    glm::vec2 origCamPos  = m_camera.position();
    if (glm::length(fx.shakeOffset) > 0.001f)
        m_camera.setPosition(origCamPos + fx.shakeOffset);

    // ---- Apply screen effects to post-process (flash + distortion) ----------
    auto& ppCfg             = m_pipeline.ppSettings();
    ppCfg.flash.color       = fx.flashColor;
    ppCfg.distort.type      = static_cast<int>(fx.distortType);
    ppCfg.distort.strength  = fx.distortStr;
    ppCfg.distort.center    = fx.distortCenter;
    ppCfg.distort.time      = fx.distortTime;

    // Run all render passes
    m_pipeline.render(
        m_camera, m_camCtrl,
        m_world, m_em, m_particles,
        m_ui,
        m_dayTime, m_sunlight, m_totalTime
    );

    // Restore camera position after shake
    if (glm::length(fx.shakeOffset) > 0.001f)
        m_camera.setPosition(origCamPos);
}

// ---- Helpers ----------------------------------------------------------------

float Game::computeSunlight() const {
    float angle = m_dayTime * 2.f * 3.14159265f;
    float raw   = (std::cos(angle) + 1.f) * 0.5f;
    return 0.04f + raw * 0.96f;
}

glm::vec2 Game::cameraTarget() const {
    glm::vec2 c = m_player.center();
    c.y -= 20.f;
    return c;
}

// ---- Theme → PostProcess ----------------------------------------------------
// Maps a Theme's visual config to the PostProcess settings.
// Call when switching themes (or once at startup with the default theme).
void Game::applyThemeToPostProcess(const Theme* theme) {
    if (!theme) return;
    auto& pp = m_pipeline.ppSettings();

    // Bloom
    pp.bloom.intensity  = theme->vfx.bloomIntensity;
    pp.bloom.threshold  = theme->vfx.bloomThreshold;
    pp.bloom.tint       = theme->vfx.bloomTint;

    // Chromatic aberration
    pp.chromAb.enabled  = theme->vfx.chromaticAb;
    pp.chromAb.strength = theme->vfx.chromaticStr;

    // Color grade: lift shadows, tint highlights
    pp.colorGrade.lift  = theme->palette.liftGrade;
    pp.colorGrade.gain  = theme->palette.gainGrade;
}
