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
#include "vfx/ThemeRegistry.h"
#include "vfx/UnifiedVFX.h"
#include "vfx/VFXCombo.h"
#include "vfx/ScreenEffects.h"

static constexpr int LOAD_RADIUS_X = 6;
static constexpr int LOAD_RADIUS_Y = 5;

class Game {
public:
    explicit Game(const WindowConfig& cfg = {});

    void run();

    // ---- Accessors ----------------------------------------------------------
    EntityManager&  entities()    { return m_em;       }
    World&          world()       { return m_world;    }
    RenderPipeline& pipeline()    { return m_pipeline; }
    UnifiedVFX&     vfx()         { return m_vfx;      }
    ThemeRegistry&  themes()      { return ThemeRegistry::instance(); }
    ScreenEffects&  screen()      { return m_screenFX; }

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

    // ---- VFX layer ----------------------------------------------------------
    ScreenEffects   m_screenFX;
    UnifiedVFX      m_vfx;

    // ---- ECS entity IDs -----------------------------------------------------
    EntityID m_playerEntity = NULL_ENTITY;

    // ---- Game state ---------------------------------------------------------
    bool  m_running        = true;
    bool  m_showInventory  = false;
    float m_dayTime        = 0.5f;
    float m_dayDuration    = 600.f;
    float m_sunlight       = 1.0f;
    float m_totalTime      = 0.f;

    // ---- Per-frame ----------------------------------------------------------
    void processInput();
    void update(float dt);
    void renderFrame();
    void rebuildDirtyChunks();
    void loadChunksAroundCamera();

    // ---- Helpers ------------------------------------------------------------
    float     computeSunlight() const;
    glm::vec2 cameraTarget()    const;

    // ---- Apply theme to post-process settings --------------------------------
    void applyThemeToPostProcess(const Theme* theme);
};
