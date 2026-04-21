#include "World.h"
#include "world/Noise.h"
#include "world/TileRegistry.h"
#include "world/StructureGen.h"
#include <cmath>
#include <algorithm>

static constexpr int SURFACE_Y   = 42;
static constexpr int BEDROCK_Y   = 200;

World::World(unsigned seed) : m_seed(seed) {}

// ---- Chunk management -------------------------------------------------------

Chunk* World::getOrCreateChunk(int cx, int cy) {
    auto key = std::make_pair(cx, cy);
    auto it  = m_chunks.find(key);
    if (it != m_chunks.end()) return it->second.get();
    auto& slot = m_chunks[key];
    slot = std::make_unique<Chunk>(cx, cy);
    generateChunk(cx, cy);
    return slot.get();
}

Chunk* World::getChunk(int cx, int cy) {
    auto it = m_chunks.find({cx, cy});
    return it != m_chunks.end() ? it->second.get() : nullptr;
}

const Chunk* World::getChunk(int cx, int cy) const {
    auto it = m_chunks.find({cx, cy});
    return it != m_chunks.end() ? it->second.get() : nullptr;
}

// ---- Tile access ------------------------------------------------------------

TileData* World::getTile(int tx, int ty) {
    Chunk* c = getChunk(tileToChunkCoord(tx), tileToChunkCoord(ty));
    if (!c) return nullptr;
    return &c->at(tileLocalCoord(tx), tileLocalCoord(ty));
}

const TileData* World::getTile(int tx, int ty) const {
    const Chunk* c = getChunk(tileToChunkCoord(tx), tileToChunkCoord(ty));
    if (!c) return nullptr;
    return &c->at(tileLocalCoord(tx), tileLocalCoord(ty));
}

TileType World::getTileType(int tx, int ty) const {
    const TileData* d = getTile(tx, ty);
    return d ? d->type : TileType::Air;
}

void World::setTile(int tx, int ty, TileType t) {
    Chunk* c = getChunk(tileToChunkCoord(tx), tileToChunkCoord(ty));
    if (c) c->setType(tileLocalCoord(tx), tileLocalCoord(ty), t);
}

TileType World::getWallType(int tx, int ty) const {
    const Chunk* c = getChunk(tileToChunkCoord(tx), tileToChunkCoord(ty));
    if (!c) return TileType::Air;
    return c->getWall(tileLocalCoord(tx), tileLocalCoord(ty));
}

void World::setWallTile(int tx, int ty, TileType t) {
    Chunk* c = getChunk(tileToChunkCoord(tx), tileToChunkCoord(ty));
    if (c) c->setWall(tileLocalCoord(tx), tileLocalCoord(ty), t);
}

void World::setDecoTile(int tx, int ty, TileType t) {
    Chunk* c = getChunk(tileToChunkCoord(tx), tileToChunkCoord(ty));
    if (c) c->setDeco(tileLocalCoord(tx), tileLocalCoord(ty), t);
}

bool World::isSolid(int tx, int ty) const {
    const TileData* d = getTile(tx, ty);
    if (!d) return ty > SURFACE_Y + 10;
    return TileRegistry::instance().get(d->type).solid;
}

bool World::isAir(int tx, int ty) const { return getTileType(tx, ty) == TileType::Air; }

bool World::isTransparent(int tx, int ty) const {
    const TileData* d = getTile(tx, ty);
    if (!d) return ty <= SURFACE_Y;
    return TileRegistry::instance().get(d->type).transparent;
}

// ---- World queries ----------------------------------------------------------

int World::surfaceY(int tx) const {
    Noise n(m_seed);
    float h = n.fbm2(tx * 0.04f, 0.0f, 5, 2.0f, 0.5f);
    return SURFACE_Y + (int)(h * 18.f);
}

BiomeType World::biomeAt(int tx, int ty) const {
    return BiomeSystem::evaluate(tx, ty, surfaceY(tx), m_seed);
}

// ---- World generation -------------------------------------------------------

void World::generateChunk(int cx, int cy) {
    Chunk* chunk = getChunk(cx, cy);
    if (!chunk) return;

    Noise terrain (m_seed);
    Noise caves   (m_seed ^ 0xDEADBEEF);
    Noise ores    (m_seed ^ 0xCAFEBABE);
    Noise caveDetail(m_seed ^ 0x12345678);

    for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
        int tx = cx * CHUNK_SIZE + lx;
        float hn = terrain.fbm2(tx * 0.04f, 0.0f, 5, 2.0f, 0.5f);
        int surface = SURFACE_Y + (int)(hn * 18.f);

        BiomeType biome = BiomeSystem::evaluate(tx, surface, surface, m_seed);
        const BiomeProperties& bp = BiomeSystem::get(biome);

        for (int ly = 0; ly < CHUNK_SIZE; ++ly) {
            int ty = cy * CHUNK_SIZE + ly;
            TileType tile = TileType::Air;
            TileType wall = TileType::Air;

            if (ty >= BEDROCK_Y) {
                tile = TileType::Bedrock;
                wall = TileType::StoneWall;
            } else if (ty >= surface) {
                // Cave detection (two overlapping noise fields for organic shapes)
                float cv1 = caves.noise2(tx * 0.07f, ty * 0.07f);
                float cv2 = caveDetail.noise2(tx * 0.14f + 50.f, ty * 0.14f + 50.f);
                bool inCave = (cv1 > 0.25f || cv2 > 0.35f) && ty > surface + 5;

                int depth = ty - surface;

                // Wall behind solid terrain
                wall = (depth < 2) ? TileType::DirtWall : TileType::StoneWall;
                if (biome == BiomeType::Desert || biome == BiomeType::Jungle)
                    wall = TileType::DirtWall;

                if (!inCave) {
                    if (depth == 0) {
                        switch (biome) {
                            case BiomeType::Tundra:  tile = TileType::Snow; break;
                            case BiomeType::Desert:  tile = TileType::Sand; break;
                            default:                 tile = TileType::Grass; break;
                        }
                    } else if (depth < bp.dirtDepth) {
                        tile = (biome == BiomeType::Desert) ? TileType::Sand : TileType::Dirt;
                        if (biome == BiomeType::Tundra && depth < 3) tile = TileType::Snow;
                    } else {
                        tile = TileType::Stone;

                        // Ore generation by depth
                        float ov = ores.noise2(tx * 0.28f, ty * 0.28f) * bp.oreDensity;
                        if (ov > 0.60f) {
                            if      (depth > 100) tile = TileType::Diamond;
                            else if (depth > 70)  tile = TileType::Gold;
                            else if (depth > 45)  tile = TileType::Silver;
                            else if (depth > 30)  tile = TileType::Iron;
                            else if (depth > 15)  tile = TileType::Copper;
                            else                  tile = TileType::Coal;
                        }

                        // Gravel pockets
                        float gv = ores.noise2(tx*0.12f+77.f, ty*0.12f+33.f);
                        if (gv > 0.70f && depth > 10 && depth < 40)
                            tile = TileType::Gravel;

                        // Glowstone deep underground
                        if (depth > 90) {
                            float glow = ores.noise2(tx*0.25f+200.f, ty*0.25f+200.f);
                            if (glow > 0.78f) tile = TileType::Glowstone;
                        }
                    }
                } else {
                    // Cave interior: no main tile, but keep wall
                    tile = TileType::Air;
                }
            }

            chunk->at(lx, ly).type       = tile;
            chunk->layer(TileLayer::Wall, lx, ly).type = wall;
        }

        // Tree generation
        {
            int ly_surf = surface - cy * CHUNK_SIZE;
            bool surfInChunk = (ly_surf >= 0 && ly_surf < CHUNK_SIZE);
            if (surfInChunk && biome != BiomeType::Desert) {
                float treeProb = bp.treeProbability;
                if (biome == BiomeType::Jungle) treeProb = 0.35f;
                unsigned hash = (unsigned)(tx * 2654435761u ^ m_seed);
                if ((float)(hash & 0xFF) / 255.f < treeProb) {
                    int treeH = 5 + (int)((hash >> 8) & 3);
                    for (int th = 1; th <= treeH; ++th) {
                        int tly = ly_surf - th;
                        if (tly >= 0 && tly < CHUNK_SIZE)
                            chunk->at(lx, tly).type = TileType::Wood;
                    }
                    int canopyR = (biome == BiomeType::Jungle) ? 3 : 2;
                    for (int dy = -treeH; dy <= -treeH + 3; ++dy) {
                        int r = (dy == -treeH || dy == -treeH+1) ? canopyR : canopyR-1;
                        for (int dx = -r; dx <= r; ++dx) {
                            int llx = lx + dx, lly = ly_surf + dy;
                            if (llx >= 0 && llx < CHUNK_SIZE && lly >= 0 && lly < CHUNK_SIZE)
                                if (chunk->at(llx, lly).type == TileType::Air)
                                    chunk->at(llx, lly).type = TileType::Leaves;
                        }
                    }
                }
            }
        }
    }

    // Dungeon / structure pass (via StructureGen)
    StructureGen::decorate(*this, cx, cy, m_seed);

    chunk->markDirty();
}

// ---- Fluid simulation -------------------------------------------------------

void World::tickFluids() {
    // Simple cellular automata: iterate all loaded chunks
    // Water: falls down, then spreads left/right
    // Lava: same but 3x slower (handled externally by calling less often)

    std::vector<std::tuple<int,int,TileType,uint8_t>> changes;

    for (auto& [coord, chunk] : m_chunks) {
        for (int ly = CHUNK_SIZE - 2; ly >= 0; --ly) {
            for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
                int tx = coord.first  * CHUNK_SIZE + lx;
                int ty = coord.second * CHUNK_SIZE + ly;

                TileData& td = chunk->at(lx, ly);
                bool isWater = (td.type == TileType::Water);
                bool isLava  = (td.type == TileType::Lava);
                if (!isWater && !isLava) continue;

                uint8_t level = td.fluidLevel;
                if (level == 0) continue;

                TileType below = getTileType(tx, ty + 1);

                // Fall down
                if (below == TileType::Air) {
                    changes.emplace_back(tx, ty,   TileType::Air,         0);
                    changes.emplace_back(tx, ty+1, td.type, std::min((uint8_t)8, level));
                    continue;
                }

                // Lava + water = obsidian/cobblestone
                if (isLava) {
                    bool waterLeft  = getTileType(tx-1, ty) == TileType::Water;
                    bool waterRight = getTileType(tx+1, ty) == TileType::Water;
                    bool waterBelow = below == TileType::Water;
                    if (waterLeft || waterRight || waterBelow) {
                        changes.emplace_back(tx, ty, TileType::Obsidian, 0);
                        continue;
                    }
                }

                // Spread sideways if not at level 1
                if (level > 1) {
                    bool leftAir  = getTileType(tx-1, ty) == TileType::Air;
                    bool rightAir = getTileType(tx+1, ty) == TileType::Air;
                    uint8_t newLevel = level - 1;
                    if (leftAir)  changes.emplace_back(tx-1, ty, td.type, newLevel);
                    if (rightAir) changes.emplace_back(tx+1, ty, td.type, newLevel);
                }
            }
        }
    }

    for (auto& [tx, ty, type, level] : changes) {
        setTile(tx, ty, type);
        TileData* td = getTile(tx, ty);
        if (td) { td->fluidLevel = level; }
        Chunk* c = getChunk(tileToChunkCoord(tx), tileToChunkCoord(ty));
        if (c) c->markDirty();
    }
}
