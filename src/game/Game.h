#pragma once
#include "core/Window.h"
#include "core/Timer.h"
#include "renderer/RenderPipeline.h"
#include "renderer/Camera.h"
#include "renderer/ParticleSystem.h"
#include "world/World.h"
#include "game/Player.h"
#include "game/CameraController.h"
#include "game/UISystem.h"
#include "ecs/ECS.h"
#include "ecs/Components.h"

static constexpr int LOAD_RADIUS_X = 6;
static constexpr int LOAD_RADIUS_Y = 5;

class Game {
public:
    explicit Game(const WindowConfig& cfg = {});

    void run();

    // ---- Accessors (for subclasses / scripting) -----------------------------
    EntityManager&  entities()  { return m_em; }
    World&          world()     { return m_world; }
    RenderPipeline& pipeline()  { return m_pipeline; }

private:
    // ---- Subsystems ---------------------------------------------------------
    Window          m_window;
    Timer           m_timer;
    RenderPipeline  m_pipeline;
    Camera          m_camera;
    CameraController m_camCtrl;
    World           m_world;
    Player          m_player;
    EntityManager   m_em;
    ParticleManager m_particles;
    UISystem        m_ui;

    // ---- ECS entity IDs -----------------------------------------------------
    EntityID m_playerEntity = NULL_ENTITY;

    // ---- Game state ---------------------------------------------------------
    bool  m_running       = true;
    bool  m_showInventory = false;
    float m_dayTime       = 0.5f;   // 0=midnight, 0.5=noon
    float m_dayDuration   = 600.f;  // seconds per full day
    float m_sunlight      = 1.0f;
    float m_totalTime     = 0.f;    // cumulative seconds (for shaders)

    // ---- Per-frame ----------------------------------------------------------
    void processInput();
    void update(float dt);
    void renderFrame();
    void rebuildDirtyChunks();
    void loadChunksAroundCamera();

    // ---- Helpers ------------------------------------------------------------
    float     computeSunlight() const;
    glm::vec2 cameraTarget()    const;
};
