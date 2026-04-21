#pragma once
#include "ecs/ECS.h"
#include "renderer/LightSystem.h"
#include "renderer/ParticleSystem.h"
#include "game/Item.h"
#include <glm/glm.hpp>
#include <string>
#include <functional>

// ---- CTransform --------------------------------------------------------------
// World-space position, rotation, scale for any entity
struct CTransform {
    glm::vec2 pos      = {0.f, 0.f};   // world pixels
    float     rotation = 0.f;          // radians
    glm::vec2 scale    = {1.f, 1.f};
    int       zOrder   = 0;            // render sort order
};

// ---- CSprite -----------------------------------------------------------------
// References a texture atlas region; rendered via SpriteBatch
struct CSprite {
    uint32_t   texID    = 0;           // OpenGL texture handle
    uint32_t   normTexID = 0;          // normal map texture (0 = none)
    glm::vec2  uvMin    = {0.f, 0.f};
    glm::vec2  uvMax    = {1.f, 1.f};
    glm::vec2  size     = {16.f, 16.f}; // pixels
    glm::vec4  tint     = {1,1,1,1};
    float      emissive = 0.f;
    bool       flipX    = false;
    bool       visible  = true;
};

// ---- CAnimation --------------------------------------------------------------
// Drives a CSprite through a spritesheet animation
struct AnimClip {
    std::string name;
    int   frameStart = 0;   // first frame index in sheet (row*cols + col)
    int   frameCount = 1;
    float fps        = 12.f;
    bool  loop       = true;
};

struct CAnimation {
    std::vector<AnimClip> clips;
    std::string  currentClip;          // name of playing clip
    int          frame      = 0;       // current frame index within clip
    float        timer      = 0.f;     // time since last frame advance
    bool         paused     = false;

    // Sheet layout
    int sheetCols = 8;   // columns in the spritesheet atlas
    int frameW    = 16;  // pixels per frame
    int frameH    = 16;

    // Event callback: called when clip finishes (non-looping) or loops
    std::function<void(const std::string&)> onFinish;

    // Convenience
    const AnimClip* current() const {
        for (auto& c : clips) if (c.name == currentClip) return &c;
        return clips.empty() ? nullptr : &clips[0];
    }

    void play(const std::string& name) {
        if (currentClip == name) return;
        currentClip = name;
        frame = 0;
        timer = 0.f;
        paused = false;
    }
};

// ---- CPhysicsBody ------------------------------------------------------------
// AABB-based rigid body driven by Physics system
struct CPhysicsBody {
    glm::vec2 vel          = {0.f, 0.f};  // pixels/second
    glm::vec2 halfExtents  = {7.f, 11.f}; // AABB half-size (from centre)
    float     gravityScale = 1.f;
    float     friction     = 0.85f;       // ground horizontal damping per tick
    float     airResist    = 0.98f;       // air horizontal damping per tick
    bool      onGround     = false;
    bool      onCeiling    = false;
    bool      onWallLeft   = false;
    bool      onWallRight  = false;
    bool      kinematic    = false;       // true = no gravity, no collision resolve
    bool      noClip       = false;       // ghost mode
};

// ---- CHealth -----------------------------------------------------------------
struct CHealth {
    float hp                  = 100.f;
    float maxHp               = 100.f;
    float mana                = 100.f;
    float maxMana             = 100.f;
    float manaRegen           = 5.f;     // per second
    float invincibilityTimer  = 0.f;     // seconds of i-frames remaining
    float invincibilityWindow = 0.5f;    // total i-frame duration on hit
    bool  dead                = false;

    float hpFraction()   const { return maxHp   > 0.f ? hp   / maxHp   : 0.f; }
    float manaFraction() const { return maxMana > 0.f ? mana / maxMana : 0.f; }

    // Returns true if damage was applied (not in i-frames, not dead)
    bool applyDamage(float dmg) {
        if (dead || invincibilityTimer > 0.f) return false;
        hp -= dmg;
        invincibilityTimer = invincibilityWindow;
        if (hp <= 0.f) { hp = 0.f; dead = true; }
        return true;
    }
};

// ---- CAI ---------------------------------------------------------------------
// Simple behaviour-tree state for NPC/enemy AI
enum class AIBehaviour { Passive, Wander, FollowTarget, FleeTarget, Patrol, Custom };

struct CAI {
    AIBehaviour behaviour   = AIBehaviour::Wander;
    EntityID    target      = NULL_ENTITY;  // entity being followed/fled
    float       aggroRadius = 200.f;        // pixels, detect player in range
    float       attackRadius = 30.f;
    float       wanderTimer  = 0.f;
    glm::vec2   wanderTarget = {0.f, 0.f};
    float       attackCooldown = 0.f;
    float       attackDamage   = 10.f;

    // Hook for custom behaviour logic (set by game code)
    std::function<void(EntityID self, float dt)> customUpdate;
};

// ---- CCollider ---------------------------------------------------------------
// Logical collision shape; used for trigger detection (not physics resolve)
struct CCollider {
    glm::vec2 offset     = {0.f, 0.f};   // from CTransform.pos
    glm::vec2 halfExtents = {8.f, 8.f};
    uint32_t  layerMask  = 0xFFFFFFFF;   // bitmask: which layers collide
    bool      isTrigger  = false;        // trigger = no resolve, event only

    // Callback when another entity enters trigger
    std::function<void(EntityID self, EntityID other)> onTriggerEnter;
};

// ---- CLight ------------------------------------------------------------------
// Attaches a 2D point light to an entity (picked up by LightSystem)
struct CLight {
    glm::vec3 color     = {1.f, 0.9f, 0.6f};  // RGB
    float     radius    = 120.f;               // pixels
    float     intensity = 1.f;
    bool      castShadow = true;
    float     flickerAmp = 0.f;   // 0=steady, 0.1=subtle torch flicker
    float     flickerT   = 0.f;   // internal timer
};

// ---- CPickup -----------------------------------------------------------------
// Entity is a world drop that can be attracted to the player and collected
struct CPickup {
    ItemType  item        = ItemType::Block_Dirt;
    int       count       = 1;
    float     magnetRange = 80.f;   // pixels, auto-attract beyond this
    float     pickupRange = 16.f;   // pixels, collect on overlap
    float     lifetime    = 300.f;  // seconds before despawn
    float     bob         = 0.f;    // internal: bob phase
};

// ---- CClickable --------------------------------------------------------------
// For clicker/tycoon games: entity responds to mouse clicks
struct CClickable {
    float     cooldown     = 0.f;   // current cooldown timer (seconds)
    float     cooldownMax  = 0.1f;  // minimum time between clicks
    glm::vec2 halfExtents  = {8.f, 8.f};  // click AABB in world space

    // Return value passed to game logic (e.g. coins per click)
    float     valuePerClick   = 1.f;
    float     valueMultiplier = 1.f;

    bool hovered = false;
    bool pressed = false;

    // Callbacks
    std::function<void(EntityID self, float value)> onClick;
    std::function<void(EntityID self)>              onHover;
};

// ---- CParticleEmitter --------------------------------------------------------
// Binds a particle emitter preset to an entity; ParticleSystem picks these up
struct CParticleEmitter {
    ParticlePreset preset  = ParticlePreset::Smoke;
    bool           active  = true;
    float          offsetX = 0.f;
    float          offsetY = 0.f;

    // If non-zero, overrides preset's emitRate
    float emitRateOverride = 0.f;
};

// ---- CTag / CName ------------------------------------------------------------
// Lightweight identity tags (no overhead pool needed)
struct CTag {
    uint32_t tag = 0;  // bitmask of game-defined tags (Player=1, Enemy=2, Projectile=4...)
    bool     isPlayer = false;
    bool     isEnemy  = false;
};

struct CName {
    std::string name;
};

// ---- CInventoryRef -----------------------------------------------------------
// Marks entity as having an inventory (player/chest); index into game-owned store
struct CInventoryRef {
    int inventoryIndex = -1;  // index into Game's inventory array
    int hotbarSlot     = 0;   // currently selected hotbar slot
};

// ---- CCameraFollow -----------------------------------------------------------
// Marks entity as the camera target; CameraController reads this
struct CCameraFollow {
    float springStiffness = 8.f;    // higher = snappier
    float deadzoneW       = 32.f;   // pixels, horizontal deadzone half-width
    float deadzoneH       = 24.f;
    glm::vec2 offset      = {0.f, -16.f};  // camera offset from entity centre
};

// ---- CDamageNumber -----------------------------------------------------------
// Spawned temporarily to show floating damage text
struct CDamageNumber {
    float  value    = 0.f;
    float  lifetime = 1.2f;   // seconds remaining
    float  vy       = -60.f;  // pixels/second upward
    glm::vec4 color = {1.f, 0.2f, 0.2f, 1.f};
    bool   isCrit   = false;
};
