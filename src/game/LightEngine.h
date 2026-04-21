#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <queue>
#include <unordered_map>

class World;

// ---- BFS-based 2D lighting similar to Terraria/Minecraft -------------------
// Operates on a rectangular tile region each frame.
// Sky light flows downward from the top of loaded terrain.
// Point lights (torches, lava) spread radially with RGB tint.
// Results are stored as float RGB per tile for chunk::buildInstances().

class LightEngine {
public:
    // Region origin (tile coords) and size (tile units)
    void resize(int originTX, int originTY, int widthTiles, int heightTiles);

    // Full recompute over the stored region.
    // sunlight: 0..1 sky light strength (1 = noon, 0 = midnight)
    void compute(const World& world, float sunlight);

    // Query light at absolute tile coords (returns 0 if outside region)
    glm::vec3 getLightAt(int tx, int ty) const;

    // Convenience: fill a CHUNK_SIZE x CHUNK_SIZE array for Chunk::buildInstances
    // chunkOriginTX/TY = cx*CHUNK_SIZE, cy*CHUNK_SIZE
    void fillChunkLight(int chunkOriginTX, int chunkOriginTY,
                        glm::vec3 out[32][32]) const;

    int originX() const { return m_ox; }
    int originY() const { return m_oy; }
    int width()   const { return m_w;  }
    int height()  const { return m_h;  }

private:
    struct Cell { float r = 0, g = 0, b = 0; };

    // Flat 2D array indexed [y * m_w + x]  (local coords)
    std::vector<Cell> m_cells;

    int m_ox = 0, m_oy = 0; // top-left tile coord of region
    int m_w  = 0, m_h  = 0;

    int  index(int lx, int ly) const { return ly * m_w + lx; }
    bool inBounds(int lx, int ly) const {
        return lx >= 0 && lx < m_w && ly >= 0 && ly < m_h;
    }

    // BFS node: local coords + RGB source
    struct LightNode {
        int lx, ly;
        float r, g, b;
    };

    void bfsPropagate(std::queue<LightNode>& q, const World& world);
};
