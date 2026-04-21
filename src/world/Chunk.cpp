#include "Chunk.h"
#include "world/TileRegistry.h"

Chunk::Chunk(int cx, int cy) : m_cx(cx), m_cy(cy) {
    m_tiles.fill(TileData{});
}

TileData& Chunk::layer(TileLayer l, int lx, int ly) {
    int base = (int)l * CHUNK_SIZE * CHUNK_SIZE;
    return m_tiles[base + ly * CHUNK_SIZE + lx];
}

const TileData& Chunk::layer(TileLayer l, int lx, int ly) const {
    int base = (int)l * CHUNK_SIZE * CHUNK_SIZE;
    return m_tiles[base + ly * CHUNK_SIZE + lx];
}

void Chunk::buildLayerInstances(TileLayer l,
                                  std::vector<TileInstance>& out,
                                  const glm::vec3 light[CHUNK_SIZE][CHUNK_SIZE],
                                  float dimFactor)
{
    out.clear();
    out.reserve(CHUNK_SIZE * CHUNK_SIZE / 2);

    const float worldOX = worldOriginX();
    const float worldOY = worldOriginY();

    for (int ly = 0; ly < CHUNK_SIZE; ++ly) {
        for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
            const TileData& td = Chunk::layer(l, lx, ly);
            if (td.type == TileType::Air) continue;

            const TileProperties& props = TileRegistry::instance().get(td.type);

            TileInstance inst;
            inst.tilePos = { worldOX + lx * TILE_SIZE, worldOY + ly * TILE_SIZE };
            inst.atlasUV = { props.atlasX * ATLAS_CELL_SIZE, props.atlasY * ATLAS_CELL_SIZE };
            inst.light   = light[ly][lx] * dimFactor;
            inst._pad    = 0.f;
            out.push_back(inst);
        }
    }
}

void Chunk::buildInstances(const glm::vec3 lightRGB[CHUNK_SIZE][CHUNK_SIZE]) {
    buildLayerInstances(TileLayer::Wall,       m_wallInst, lightRGB, 0.35f);
    buildLayerInstances(TileLayer::Main,       m_mainInst, lightRGB, 1.00f);
    buildLayerInstances(TileLayer::Decoration, m_decoInst, lightRGB, 1.00f);
    m_dirty = false;
}
