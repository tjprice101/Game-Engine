#include "Player.h"
#include "world/World.h"
#include "world/TileRegistry.h"
#include "core/Input.h"
#include <cmath>
#include <algorithm>

Player::Player(glm::vec2 spawnPos) {
    m_aabb.pos  = spawnPos;
    m_aabb.size = {PLAYER_W, PLAYER_H};

    // Give starter items
    m_inventory.addItem(ItemType::Dirt,          20);
    m_inventory.addItem(ItemType::Torch,          5);
    m_inventory.addItem(ItemType::WoodPickaxe,    1);
    m_inventory.addItem(ItemType::WoodAxe,        1);
    m_inventory.addItem(ItemType::WoodShovel,     1);
}

void Player::update(float dt, World& world, glm::vec2 mouseWorldPos,
                    bool clickMine, bool clickPlace,
                    bool jumpPressed, int hotbarKey)
{
    m_justBroke  = false;
    m_justPlaced = false;

    if (hotbarKey >= 0 && hotbarKey < Inventory::HOTBAR_SIZE)
        m_inventory.setHotbarSelected(hotbarKey);

    // Scroll wheel hotbar selection
    float scroll = Input::scrollDelta();
    if (scroll != 0.f) {
        int sel = m_inventory.hotbarSelected() - (int)scroll;
        sel = ((sel % Inventory::HOTBAR_SIZE) + Inventory::HOTBAR_SIZE)
              % Inventory::HOTBAR_SIZE;
        m_inventory.setHotbarSelected(sel);
    }

    handleMovement(dt, world, Input::keyDown(Key::A), Input::keyDown(Key::D), jumpPressed);
    handleMining  (dt, world, mouseWorldPos, clickMine);
    handlePlacing (world, mouseWorldPos, clickPlace);

    m_placeCooldown = std::max(0.f, m_placeCooldown - dt);
}

void Player::handleMovement(float dt, World& world,
                             bool left, bool right, bool jumpPressed)
{
    using namespace PhysicsConst;

    // Horizontal input
    float targetVX = 0.f;
    if (left)  { targetVX -= WALK_SPEED; m_facingRight = false; }
    if (right) { targetVX += WALK_SPEED; m_facingRight = true;  }

    // Accelerate/decelerate toward target
    if (targetVX != 0.f) {
        float diff = targetVX - m_vel.x;
        float accel = WALK_ACCEL * dt;
        m_vel.x += std::clamp(diff, -accel, accel);
    } else {
        // Friction
        float decel = FRICTION * dt;
        if (m_vel.x > 0.f) m_vel.x = std::max(0.f, m_vel.x - decel);
        else                m_vel.x = std::min(0.f, m_vel.x + decel);
    }
    m_vel.x = std::clamp(m_vel.x, -MAX_WALK_SPEED, MAX_WALK_SPEED);

    // Gravity
    m_vel.y += GRAVITY * dt;
    m_vel.y  = std::min(m_vel.y, MAX_FALL_SPEED);

    // Jump
    if (jumpPressed && m_onGround) {
        m_vel.y  = JUMP_VELOCITY;
        m_onGround = false;
    }

    // Resolve X
    bool hitX = false;
    float newX = Physics::resolveX(m_aabb, m_aabb.pos.x + m_vel.x * dt, hitX, world);
    if (hitX) m_vel.x = 0.f;
    m_aabb.pos.x = newX;

    // Resolve Y
    bool hitG = false, hitC = false;
    float newY = Physics::resolveY(m_aabb, m_aabb.pos.y + m_vel.y * dt, hitG, hitC, world);
    if (hitG) { m_vel.y = 0.f; m_onGround = true; }
    else      { m_onGround = false; }
    if (hitC) m_vel.y = std::max(0.f, m_vel.y);
    m_aabb.pos.y = newY;
}

void Player::handleMining(float dt, World& world,
                           glm::vec2 mouseWorld, bool clicking)
{
    if (!clicking) {
        m_miningTile   = {-1, -1};
        m_mineProgress = 0.f;
        return;
    }

    // Compute target tile
    glm::vec2 diff = mouseWorld - center();
    if (glm::length(diff) > MINE_RANGE_PX) {
        m_miningTile   = {-1, -1};
        m_mineProgress = 0.f;
        return;
    }

    int tx = World::toTileX(mouseWorld.x);
    int ty = World::toTileY(mouseWorld.y);

    TileType tt = world.getTileType(tx, ty);
    if (tt == TileType::Air || tt == TileType::Bedrock) {
        m_miningTile   = {-1, -1};
        m_mineProgress = 0.f;
        return;
    }

    // Different tile → reset progress
    if (m_miningTile.x != tx || m_miningTile.y != ty) {
        m_miningTile   = {tx, ty};
        m_mineProgress = 0.f;
    }

    // Mining speed based on held tool
    const ItemStack&   held  = m_inventory.selectedStack();
    const ItemProperties& ip = getItemProperties(held.type);
    float mineSpeed = held.empty() ? 1.0f : ip.mineSpeed;

    const TileProperties& tp = TileRegistry::instance().get(tt);
    float timeNeeded = tp.hardness / mineSpeed;  // seconds
    if (timeNeeded < 0.05f) timeNeeded = 0.05f;

    m_mineProgress += dt / timeNeeded;
    if (m_mineProgress >= 1.0f) {
        // Break tile
        ItemType drop = tileToItem(tt);
        if (drop != ItemType::None)
            m_inventory.addItem(drop, 1);

        world.setTile(tx, ty, TileType::Air);
        m_miningTile   = {-1, -1};
        m_mineProgress = 0.f;
        m_justBroke    = true;
    }
}

void Player::handlePlacing(World& world, glm::vec2 mouseWorld, bool clicking)
{
    if (!clicking || m_placeCooldown > 0.f) return;

    glm::vec2 diff = mouseWorld - center();
    if (glm::length(diff) > MINE_RANGE_PX) return;

    int tx = World::toTileX(mouseWorld.x);
    int ty = World::toTileY(mouseWorld.y);

    // Target must be Air
    if (!world.isAir(tx, ty)) return;

    // Must be adjacent to a solid tile
    bool adjacent = world.isSolid(tx-1,ty) || world.isSolid(tx+1,ty)
                 || world.isSolid(tx, ty-1)|| world.isSolid(tx, ty+1);
    if (!adjacent) return;

    // Must not overlap player AABB
    AABB tilebox;
    tilebox.pos  = { tx * TILE_SIZE, ty * TILE_SIZE };
    tilebox.size = { TILE_SIZE, TILE_SIZE };
    if (m_aabb.overlaps(tilebox)) return;

    const ItemStack& sel = m_inventory.selectedStack();
    if (sel.empty()) return;

    const ItemProperties& ip = getItemProperties(sel.type);
    if (!ip.isBlock) return;

    world.setTile(tx, ty, ip.placesAs);
    m_inventory.consumeSelected(1);
    m_placeCooldown = 0.18f;
    m_justPlaced    = true;
}
