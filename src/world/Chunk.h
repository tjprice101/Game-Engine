#pragma once
#include "world/Tile.h"
#include "renderer/TileRenderer.h"
#include <array>
#include <vector>
#include <glm/glm.hpp>

// ---- Multi-layer 32x32 tile chunk ------------------------------------------
// Layer 0 (Wall):       Background walls  – non-solid, rendered first (darker)
// Layer 1 (Main):       Primary tiles     – solid collision tiles
// Layer 2 (Decoration): Foreground decor  – non-solid, rendered last

class Chunk {
public:
    Chunk(int cx, int cy);

    // ---- Tile access (main layer shortcut) ----------------------------------
    TileData&       at       (int lx, int ly)       { return layer(TileLayer::Main, lx, ly); }
    const TileData& at       (int lx, int ly) const { return layer(TileLayer::Main, lx, ly); }

    TileData&       layer    (TileLayer l, int lx, int ly);
    const TileData& layer    (TileLayer l, int lx, int ly) const;

    TileType getType (int lx, int ly) const { return at(lx,ly).type; }
    void     setType (int lx, int ly, TileType t) { at(lx,ly).type = t; markDirty(); }

    TileType getWall (int lx, int ly) const { return layer(TileLayer::Wall,lx,ly).type; }
    void     setWall (int lx, int ly, TileType t) { layer(TileLayer::Wall,lx,ly).type=t; markDirty(); }

    TileType getDeco (int lx, int ly) const { return layer(TileLayer::Decoration,lx,ly).type; }
    void     setDeco (int lx, int ly, TileType t) { layer(TileLayer::Decoration,lx,ly).type=t; markDirty(); }

    // ---- Fluid --------------------------------------------------------------
    uint8_t  fluidLevel(int lx, int ly) const { return at(lx,ly).fluidLevel; }
    void     setFluid  (int lx, int ly, uint8_t lvl) { at(lx,ly).fluidLevel = lvl; markDirty(); }

    // ---- Rendering ----------------------------------------------------------
    void buildInstances(const glm::vec3 lightRGB[CHUNK_SIZE][CHUNK_SIZE]);

    const std::vector<TileInstance>& instances()     const { return m_mainInst; }
    const std::vector<TileInstance>& wallInstances() const { return m_wallInst; }
    const std::vector<TileInstance>& decoInstances() const { return m_decoInst; }

    bool dirty()    const { return m_dirty; }
    void markDirty()      { m_dirty = true; }
    void clearDirty()     { m_dirty = false; }

    int   chunkX()       const { return m_cx; }
    int   chunkY()       const { return m_cy; }
    float worldOriginX() const { return m_cx * CHUNK_SIZE * TILE_SIZE; }
    float worldOriginY() const { return m_cy * CHUNK_SIZE * TILE_SIZE; }

private:
    void buildLayerInstances(TileLayer l,
                              std::vector<TileInstance>& out,
                              const glm::vec3 light[CHUNK_SIZE][CHUNK_SIZE],
                              float dimFactor);

    int m_cx, m_cy;
    // 3 layers × CHUNK_SIZE² tiles
    std::array<TileData, CHUNK_SIZE*CHUNK_SIZE * (int)TileLayer::_Count> m_tiles;

    std::vector<TileInstance> m_mainInst;
    std::vector<TileInstance> m_wallInst;
    std::vector<TileInstance> m_decoInst;
    bool m_dirty = true;
};
