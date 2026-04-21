#pragma once
#include "Tile.h"
#include "renderer/Texture.h"
#include <array>

class TileRegistry {
public:
    static TileRegistry& instance();

    TileRegistry(const TileRegistry&) = delete;
    TileRegistry& operator=(const TileRegistry&) = delete;

    const TileProperties& get(TileType t) const;

    // Build the atlas texture procedurally (pixel art colored tiles).
    // Call once after OpenGL is ready.
    void buildAtlas();

    Texture& atlas() { return m_atlas; }

private:
    TileRegistry();
    void registerTiles();

    // Simple pixel painter helpers
    void paintSolid   (uint8_t* pixels, int atlasX, int atlasY, uint8_t r, uint8_t g, uint8_t b);
    void paintChecked (uint8_t* pixels, int atlasX, int atlasY,
                       uint8_t r1, uint8_t g1, uint8_t b1,
                       uint8_t r2, uint8_t g2, uint8_t b2);
    void paintGrass   (uint8_t* pixels, int atlasX, int atlasY);
    void paintVariance(uint8_t* pixels, int atlasX, int atlasY,
                       uint8_t r, uint8_t g, uint8_t b, int var);
    void setPixel     (uint8_t* pixels, int px, int py, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    std::array<TileProperties, (int)TileType::_Count> m_props;
    Texture m_atlas;
};
