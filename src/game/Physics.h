#pragma once
#include <glm/glm.hpp>
#include "world/World.h"
#include "world/Tile.h"

// Simple AABB (axis-aligned bounding box) physics helpers.
// All positions are in world-pixel coordinates. +Y is DOWN (matches camera).

struct AABB {
    glm::vec2 pos;   // top-left corner in world pixels
    glm::vec2 size;  // width x height in pixels

    float left()   const { return pos.x; }
    float right()  const { return pos.x + size.x; }
    float top()    const { return pos.y; }
    float bottom() const { return pos.y + size.y; }

    glm::vec2 center() const { return pos + size * 0.5f; }

    bool overlaps(const AABB& o) const {
        return left() < o.right()  && right()  > o.left() &&
               top()  < o.bottom() && bottom() > o.top();
    }
};

// Physics constants (pixel units)
namespace PhysicsConst {
    constexpr float GRAVITY        = 900.f;  // px/s²  (downward = positive)
    constexpr float JUMP_VELOCITY  = -430.f; // px/s   (upward = negative)
    constexpr float WALK_SPEED     = 190.f;  // px/s   horizontal target speed
    constexpr float WALK_ACCEL     = 1200.f; // px/s²  horizontal acceleration
    constexpr float FRICTION       = 800.f;  // px/s²  horizontal deceleration
    constexpr float MAX_FALL_SPEED = 600.f;  // px/s   terminal velocity
    constexpr float MAX_WALK_SPEED = 220.f;  // px/s   hard cap
}

// Resolve an AABB against world tiles for one axis at a time.
// Returns the corrected position component.
struct Physics {
    // Move box along X, stop on solid tiles. Returns adjusted X and sets hitX.
    static float resolveX(const AABB& box, float newX, bool& hitX,
                          const World& world);
    // Move box along Y, stop on solid tiles. Returns adjusted Y and sets hitGround/hitCeil.
    static float resolveY(const AABB& box, float newY, bool& hitGround, bool& hitCeil,
                          const World& world);
};

inline float Physics::resolveX(const AABB& box, float newX, bool& hitX,
                                const World& world)
{
    hitX = false;
    AABB moved = box;
    moved.pos.x = newX;

    // Tile range to check
    int tileLeft  = World::toTileX(moved.left());
    int tileRight = World::toTileX(moved.right() - 0.001f);
    int tileTop   = World::toTileY(moved.top()   + 1.f);
    int tileBot   = World::toTileY(moved.bottom() - 1.f);

    for (int ty = tileTop; ty <= tileBot; ++ty) {
        for (int tx = tileLeft; tx <= tileRight; ++tx) {
            if (!world.isSolid(tx, ty)) continue;

            float tileL = tx * TILE_SIZE;
            float tileR = tileL + TILE_SIZE;

            if (moved.right() > tileL && moved.left() < tileR) {
                hitX = true;
                if (newX > box.pos.x) // Moving right
                    newX = tileL - box.size.x;
                else                  // Moving left
                    newX = tileR;
                moved.pos.x = newX;
                break;
            }
        }
    }
    return newX;
}

inline float Physics::resolveY(const AABB& box, float newY, bool& hitGround, bool& hitCeil,
                                const World& world)
{
    hitGround = false;
    hitCeil   = false;
    AABB moved = box;
    moved.pos.y = newY;

    int tileLeft  = World::toTileX(moved.left()   + 1.f);
    int tileRight = World::toTileX(moved.right()  - 1.f);
    int tileTop   = World::toTileY(moved.top());
    int tileBot   = World::toTileY(moved.bottom() - 0.001f);

    for (int ty = tileTop; ty <= tileBot; ++ty) {
        for (int tx = tileLeft; tx <= tileRight; ++tx) {
            if (!world.isSolid(tx, ty)) continue;

            float tileT = ty * TILE_SIZE;
            float tileB = tileT + TILE_SIZE;

            if (moved.bottom() > tileT && moved.top() < tileB) {
                if (newY > box.pos.y) { // Falling down
                    hitGround = true;
                    newY = tileT - box.size.y;
                } else {               // Moving up
                    hitCeil = true;
                    newY = tileB;
                }
                moved.pos.y = newY;
                break;
            }
        }
    }
    return newY;
}
