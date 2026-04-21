#include "LightEngine.h"
#include "world/World.h"
#include "world/TileRegistry.h"
#include "world/Tile.h"
#include <algorithm>
#include <cstring>

static constexpr float SKY_DECAY    = 0.92f; // side spread from sky column
static constexpr float BLOCK_DECAY  = 0.82f; // generic spread through solid
static constexpr float AIR_DECAY    = 0.92f; // spread through air/transparent
static constexpr float MIN_LIGHT    = 0.02f; // below this, stop propagating

void LightEngine::resize(int ox, int oy, int w, int h) {
    m_ox = ox; m_oy = oy; m_w = w; m_h = h;
    m_cells.assign(w * h, Cell{});
}

void LightEngine::compute(const World& world, float sunlight) {
    // Reset
    for (auto& c : m_cells) { c.r = c.g = c.b = 0.f; }

    std::queue<LightNode> q;

    // ---- Sky light: scan each column top-to-bottom ----
    for (int lx = 0; lx < m_w; ++lx) {
        int tx = m_ox + lx;
        bool exposed = true; // starts exposed to sky at top of region

        for (int ly = 0; ly < m_h; ++ly) {
            int ty = m_oy + ly;

            if (!exposed) break;

            // Is this tile transparent?
            TileType tt = world.getTileType(tx, ty);
            const TileProperties& props = TileRegistry::instance().get(tt);

            if (exposed) {
                Cell& c = m_cells[index(lx, ly)];
                float sv = sunlight;
                if (c.r < sv || c.g < sv || c.b < sv) {
                    c.r = std::max(c.r, sv);
                    c.g = std::max(c.g, sv);
                    c.b = std::max(c.b, sv);
                    q.push({lx, ly, sv, sv, sv});
                }
                if (!props.transparent) exposed = false;
            }
        }
    }

    // ---- Point light sources ----
    for (int ly = 0; ly < m_h; ++ly) {
        for (int lx = 0; lx < m_w; ++lx) {
            int tx = m_ox + lx, ty = m_oy + ly;
            TileType tt = world.getTileType(tx, ty);
            const TileProperties& props = TileRegistry::instance().get(tt);

            if (props.emitsLight && props.lightRadius > 0.f) {
                float strength = std::min(props.lightRadius / 15.f, 1.f);
                Cell& c = m_cells[index(lx, ly)];
                float nr = props.lightColor.r * strength;
                float ng = props.lightColor.g * strength;
                float nb = props.lightColor.b * strength;
                if (nr > c.r || ng > c.g || nb > c.b) {
                    c.r = std::max(c.r, nr);
                    c.g = std::max(c.g, ng);
                    c.b = std::max(c.b, nb);
                    q.push({lx, ly, nr, ng, nb});
                }
            }
        }
    }

    bfsPropagate(q, world);
}

void LightEngine::bfsPropagate(std::queue<LightNode>& q, const World& world) {
    static const int dx[] = { 1, -1, 0,  0 };
    static const int dy[] = { 0,  0, 1, -1 };

    while (!q.empty()) {
        LightNode node = q.front();
        q.pop();

        for (int d = 0; d < 4; ++d) {
            int nx = node.lx + dx[d];
            int ny = node.ly + dy[d];
            if (!inBounds(nx, ny)) continue;

            int tx = m_ox + nx, ty = m_oy + ny;
            TileType tt = world.getTileType(tx, ty);
            const TileProperties& props = TileRegistry::instance().get(tt);

            float decay = props.solid ? BLOCK_DECAY * 0.5f : AIR_DECAY;

            float nr = node.r * decay;
            float ng = node.g * decay;
            float nb = node.b * decay;

            if (nr < MIN_LIGHT && ng < MIN_LIGHT && nb < MIN_LIGHT) continue;

            Cell& neighbor = m_cells[index(nx, ny)];
            bool updated = false;
            if (nr > neighbor.r) { neighbor.r = nr; updated = true; }
            if (ng > neighbor.g) { neighbor.g = ng; updated = true; }
            if (nb > neighbor.b) { neighbor.b = nb; updated = true; }

            if (updated) q.push({nx, ny, neighbor.r, neighbor.g, neighbor.b});
        }
    }
}

glm::vec3 LightEngine::getLightAt(int tx, int ty) const {
    int lx = tx - m_ox, ly = ty - m_oy;
    if (!inBounds(lx, ly)) return {0.f, 0.f, 0.f};
    const Cell& c = m_cells[index(lx, ly)];
    return {c.r, c.g, c.b};
}

void LightEngine::fillChunkLight(int chunkOriginTX, int chunkOriginTY,
                                  glm::vec3 out[32][32]) const
{
    for (int ly = 0; ly < CHUNK_SIZE; ++ly) {
        for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
            int tx = chunkOriginTX + lx;
            int ty = chunkOriginTY + ly;
            out[ly][lx] = getLightAt(tx, ty);
        }
    }
}
