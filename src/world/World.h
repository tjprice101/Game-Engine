#pragma once
#include "world/Chunk.h"
#include "world/BiomeSystem.h"
#include <unordered_map>
#include <memory>
#include <cmath>

struct ChunkCoordHash {
    std::size_t operator()(std::pair<int,int> p) const noexcept {
        return std::hash<int64_t>()((int64_t)(uint32_t)p.first << 32 | (uint32_t)p.second);
    }
};
using ChunkMap = std::unordered_map<std::pair<int,int>, std::unique_ptr<Chunk>, ChunkCoordHash>;

class World {
public:
    explicit World(unsigned seed = 12345);

    // ---- Tile access (Main layer) -------------------------------------------
    TileData*       getTile    (int tx, int ty);
    const TileData* getTile    (int tx, int ty) const;
    TileType        getTileType(int tx, int ty) const;
    void            setTile    (int tx, int ty, TileType t);

    // ---- Wall layer access --------------------------------------------------
    TileType getWallType(int tx, int ty) const;
    void     setWallTile(int tx, int ty, TileType t);

    // ---- Decoration layer ---------------------------------------------------
    void setDecoTile(int tx, int ty, TileType t);

    // ---- Queries ------------------------------------------------------------
    bool isSolid      (int tx, int ty) const;
    bool isAir        (int tx, int ty) const;
    bool isTransparent(int tx, int ty) const;

    // ---- Coordinate helpers -------------------------------------------------
    static int  toTileX(float wx) { return (int)std::floor(wx / TILE_SIZE); }
    static int  toTileY(float wy) { return (int)std::floor(wy / TILE_SIZE); }

    static int tileToChunkCoord(int t) {
        return t < 0 ? (t - CHUNK_SIZE + 1) / CHUNK_SIZE : t / CHUNK_SIZE;
    }
    static int tileLocalCoord(int t) {
        int r = t % CHUNK_SIZE;
        return r < 0 ? r + CHUNK_SIZE : r;
    }

    // ---- Chunk management ---------------------------------------------------
    Chunk*       getOrCreateChunk(int cx, int cy);
    Chunk*       getChunk        (int cx, int cy);
    const Chunk* getChunk        (int cx, int cy) const;
    const ChunkMap& chunks()     const { return m_chunks; }

    // ---- Generation ---------------------------------------------------------
    void generateChunk(int cx, int cy);
    int  surfaceY     (int tx) const;
    BiomeType biomeAt (int tx, int ty) const;

    // ---- Simulation ---------------------------------------------------------
    // Call once per fluid-tick (10 Hz)
    void tickFluids();

    unsigned seed() const { return m_seed; }

private:
    Chunk* getOrCreate(int cx, int cy); // internal, no-gen version

    ChunkMap m_chunks;
    unsigned m_seed;
};
