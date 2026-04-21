#pragma once
#include "Physics.h"
#include "Inventory.h"
#include <glm/glm.hpp>

class World;

// Player size in pixels
static constexpr float PLAYER_W = 28.f;
static constexpr float PLAYER_H = 48.f;

// Mining range in pixels
static constexpr float MINE_RANGE_PX = 5.f * TILE_SIZE; // 5 tiles

class Player {
public:
    explicit Player(glm::vec2 spawnPos);

    // ---- Per-frame update ---------------------------------------------------
    void update(float dt, World& world, glm::vec2 mouseWorldPos,
                bool clickMine, bool clickPlace, bool jumpPressed,
                int hotbarKey); // hotbarKey = 0..9 or -1

    // ---- Queries ------------------------------------------------------------
    glm::vec2   position()   const { return m_aabb.pos; }
    glm::vec2   center()     const { return m_aabb.center(); }
    glm::vec2   velocity()   const { return m_vel; }
    AABB        aabb()       const { return m_aabb; }
    bool        onGround()   const { return m_onGround; }
    bool        facingRight()const { return m_facingRight; }
    int         health()     const { return m_health; }
    int         maxHealth()  const { return m_maxHealth; }
    Inventory&       inventory()       { return m_inventory; }
    const Inventory& inventory() const { return m_inventory; }

    // Tile being mined right now (-1,-1 if none)
    glm::ivec2  miningTile()    const { return m_miningTile; }
    float       mineProgress()  const { return m_mineProgress; }  // 0..1

    // Was a tile broken this frame?
    bool        justBroke()     const { return m_justBroke; }
    bool        justPlaced()    const { return m_justPlaced; }

private:
    void handleMovement(float dt, World& world,
                        bool left, bool right, bool jumpPressed);
    void handleMining  (float dt, World& world, glm::vec2 mouseWorld, bool clicking);
    void handlePlacing (World& world, glm::vec2 mouseWorld, bool clicking);

    AABB      m_aabb;
    glm::vec2 m_vel      = {0.f, 0.f};
    bool      m_onGround = false;
    bool      m_facingRight = true;

    int m_health    = 100;
    int m_maxHealth = 100;

    Inventory  m_inventory;

    // Mining state
    glm::ivec2 m_miningTile   = {-1, -1};
    float      m_mineProgress = 0.f;
    bool       m_justBroke    = false;
    bool       m_justPlaced   = false;

    // Place cooldown (prevent spam)
    float m_placeCooldown = 0.f;
};
