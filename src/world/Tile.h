#pragma once
#include <cstdint>
#include <glm/glm.hpp>

// ---- Tile layers ------------------------------------------------------------
enum class TileLayer : uint8_t {
    Wall        = 0,  // background wall (non-solid, darker, decorative)
    Main        = 1,  // primary solid gameplay tiles
    Decoration  = 2,  // foreground cosmetics (grass blades, mushrooms, moss)
    _Count      = 3
};

// ---- Tile type identifiers --------------------------------------------------
enum class TileType : uint16_t {
    Air           = 0,
    // --- Terrain ---
    Grass         = 1,
    Dirt          = 2,
    Stone         = 3,
    Sand          = 4,
    Gravel        = 5,
    Wood          = 6,
    Leaves        = 7,
    // --- Ores ---
    Coal          = 8,
    Iron          = 9,
    Gold          = 10,
    Diamond       = 11,
    Copper        = 12,
    Tin           = 13,
    Silver        = 14,
    // --- Special ---
    Torch         = 15,
    Water         = 16,
    Lava          = 17,
    Bedrock       = 18,
    Snow          = 19,
    Ice           = 20,
    Sandstone     = 21,
    Cobblestone   = 22,
    Obsidian      = 23,
    Glowstone     = 24,   // emits bright light
    // --- Wall variants ---
    DirtWall      = 25,
    StoneWall     = 26,
    WoodWall      = 27,
    GraniteWall   = 28,
    // --- Vegetation ---
    Cactus        = 29,
    Mushroom      = 30,
    GrassDecor    = 31,   // Decoration layer: waving grass blades
    // --- Crafted ---
    Plank         = 32,
    Brick         = 33,
    Glass         = 34,
    Chest         = 35,
    Workbench     = 36,
    Furnace       = 37,
    // --- Environment ---
    Moss          = 38,
    Vine          = 39,
    _Count        = 40
};

// ---- Per-tile stored data ---------------------------------------------------
struct TileData {
    TileType  type      = TileType::Air;
    uint8_t   meta      = 0;     // orientation/variant/growth
    uint8_t   fluidLevel= 0;     // 0=dry, 1..8=fluid depth
};

// ---- Static properties for a tile type -------------------------------------
struct TileProperties {
    const char* name          = "Air";
    bool        solid         = false;
    bool        transparent   = true;
    bool        liquid        = false;
    bool        emitsLight    = false;
    bool        isWallTile    = false;  // belongs in wall layer
    glm::vec3   lightColor    = {0.f, 0.f, 0.f};
    float       lightRadius   = 0.f;     // tiles
    float       hardness      = 1.0f;
    int         atlasX        = 0;
    int         atlasY        = 0;
    float       friction      = 0.8f;
    uint8_t     maxStack      = 64;
    float       emissiveStr   = 0.f;     // 0=none, 1=full emissive
    float       specular      = 0.f;     // specular reflectivity 0..1
    float       normalStr     = 1.0f;    // normal-map strength multiplier
};

// ---- Constants --------------------------------------------------------------
constexpr int   CHUNK_SIZE       = 32;
constexpr float TILE_SIZE        = 16.0f;
constexpr int   ATLAS_GRID       = 8;
constexpr float ATLAS_CELL_SIZE  = 1.0f / ATLAS_GRID;
constexpr int   ATLAS_PIXEL_SIZE = 128;
constexpr int   TILE_PIXEL       = ATLAS_PIXEL_SIZE / ATLAS_GRID;
