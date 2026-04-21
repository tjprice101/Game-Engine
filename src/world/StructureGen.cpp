#include "StructureGen.h"
#include "world/World.h"
#include "world/Noise.h"
#include <cstdlib>
#include <algorithm>

static unsigned lcg(unsigned& s) { return s = s * 1664525u + 1013904223u; }
static int rngRange(unsigned& s, int lo, int hi) {
    lcg(s);
    return lo + (int)((s >> 8) % (hi - lo + 1));
}

void StructureGen::decorate(World& world, int cx, int cy, unsigned seed) {
    // Use chunk coords to create a deterministic per-chunk seed
    unsigned chunkSeed = seed ^ (unsigned)(cx * 2654435761u) ^ (unsigned)(cy * 1013904223u);

    int depth = cy * CHUNK_SIZE - world.surfaceY(cx * CHUNK_SIZE);

    // Dungeon: very rare, only deep underground
    if (depth > 50) {
        unsigned ds = chunkSeed ^ 0xABC;
        lcg(ds);
        if ((ds & 0xFF) < 6) // ~2% chance per chunk
            placeDungeon(world, cx, cy, chunkSeed);
    }

    // Underground cabin: near surface underground
    if (depth > 8 && depth < 40) {
        unsigned cs = chunkSeed ^ 0xCABB;
        lcg(cs);
        if ((cs & 0xFF) < 8) // ~3%
            placeCabin(world, cx, cy, chunkSeed);
    }

    // Mineshaft: medium depth
    if (depth > 20 && depth < 100) {
        unsigned ms = chunkSeed ^ 0x4D494Eu;
        lcg(ms);
        if ((ms & 0xFF) < 12) // ~5%
            placeMineshaft(world, cx, cy, chunkSeed);
    }

    // Surface features
    if (depth <= 0) {
        placeCactus  (world, cx, cy, chunkSeed);
        placeMushrooms(world, cx, cy, chunkSeed);
    }
}

void StructureGen::fillRect(World& world, int tx, int ty, int w, int h, TileType t) {
    for (int y = ty; y < ty + h; ++y)
        for (int x = tx; x < tx + w; ++x)
            world.setTile(x, y, t);
}

void StructureGen::hollowRect(World& world,
                               int tx, int ty, int w, int h,
                               TileType wall, TileType fill)
{
    for (int y = ty; y < ty + h; ++y) {
        for (int x = tx; x < tx + w; ++x) {
            bool isEdge = (x == tx || x == tx+w-1 || y == ty || y == ty+h-1);
            world.setTile(x, y, isEdge ? wall : fill);
        }
    }
}

void StructureGen::placeDungeon(World& world, int cx, int cy, unsigned seed) {
    unsigned s = seed ^ 0xD0D0D0u;
    int baseX = cx * CHUNK_SIZE + rngRange(s, 4, CHUNK_SIZE - 12);
    int baseY = cy * CHUNK_SIZE + rngRange(s, 4, CHUNK_SIZE - 12);
    int rooms = rngRange(s, 2, 5);

    // Dig out rooms connected by corridors
    int prevRX = baseX, prevRY = baseY;
    for (int r = 0; r < rooms; ++r) {
        int rw = rngRange(s, 5, 9);
        int rh = rngRange(s, 4, 7);
        int rx = baseX + rngRange(s, -6, 6);
        int ry = baseY + rngRange(s, -4, 4);

        hollowRect(world, rx, ry, rw, rh, TileType::Cobblestone, TileType::Air);

        // Corridor to previous room
        int cx2 = rx, cy2 = ry + rh/2;
        int ex = prevRX, ey = prevRY;
        while (cx2 != ex) {
            world.setTile(cx2, cy2, TileType::Air);
            cx2 += (cx2 < ex) ? 1 : -1;
        }
        while (cy2 != ey) {
            world.setTile(cx2, cy2, TileType::Air);
            cy2 += (cy2 < ey) ? 1 : -1;
        }

        // Place torch in room
        world.setTile(rx + rw/2, ry + 1, TileType::Torch);

        prevRX = rx + rw/2; prevRY = ry + rh/2;
    }
}

void StructureGen::placeCabin(World& world, int cx, int cy, unsigned seed) {
    unsigned s = seed ^ 0xCAB1u;
    int ox = cx * CHUNK_SIZE + rngRange(s, 2, CHUNK_SIZE - 12);
    int oy = cy * CHUNK_SIZE + rngRange(s, 2, CHUNK_SIZE - 8);
    int w  = rngRange(s, 7, 11);
    int h  = rngRange(s, 4,  6);

    hollowRect(world, ox, oy, w, h, TileType::Wood, TileType::Plank);

    // Door opening
    world.setTile(ox + w/2, oy + h - 1, TileType::Air);
    world.setTile(ox + w/2, oy + h - 2, TileType::Air);

    // Window
    world.setTile(ox + 1, oy + 1, TileType::Glass);
    world.setTile(ox + w - 2, oy + 1, TileType::Glass);

    // Interior: chest + workbench
    world.setTile(ox + 2, oy + h - 2, TileType::Chest);
    world.setTile(ox + w - 3, oy + h - 2, TileType::Workbench);

    // Torches
    world.setTile(ox + 1, oy + 1, TileType::Torch);
    world.setTile(ox + w - 2, oy + 1, TileType::Torch);
}

void StructureGen::placeMineshaft(World& world, int cx, int cy, unsigned seed) {
    unsigned s = seed ^ 0x4D494Eu;
    int ox = cx * CHUNK_SIZE;
    int oy = cy * CHUNK_SIZE + rngRange(s, 2, CHUNK_SIZE - 4);

    // Horizontal tunnel with wood supports every 5 tiles
    for (int dx = 0; dx < CHUNK_SIZE; ++dx) {
        world.setTile(ox + dx, oy,     TileType::Air);
        world.setTile(ox + dx, oy + 1, TileType::Air);

        if (dx % 5 == 0) {
            // Plank support
            world.setTile(ox + dx, oy - 1, TileType::Plank);
            world.setTile(ox + dx, oy + 2, TileType::Plank);
        }
    }

    // Random torches
    for (int dx = 3; dx < CHUNK_SIZE; dx += rngRange(s, 7, 12)) {
        world.setTile(ox + dx, oy, TileType::Torch);
    }
}

void StructureGen::placeCactus(World& world, int cx, int cy, unsigned seed) {
    unsigned s = seed ^ 0xCAC1u;
    // Only in desert biome
    for (int lx = 0; lx < CHUNK_SIZE; lx += 3) {
        int tx = cx * CHUNK_SIZE + lx;
        BiomeType biome = world.biomeAt(tx, cy * CHUNK_SIZE);
        if (biome != BiomeType::Desert) continue;

        lcg(s);
        if ((s & 0xFF) > 30) continue;

        int sy  = world.surfaceY(tx) - 1;
        int h   = 2 + (int)((s >> 8) & 3);
        for (int i = 0; i < h; ++i)
            world.setTile(tx, sy - i, TileType::Cactus);
    }
}

void StructureGen::placeMushrooms(World& world, int cx, int cy, unsigned seed) {
    unsigned s = seed ^ 0x4D555348u;
    for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
        int tx = cx * CHUNK_SIZE + lx;
        lcg(s);
        if ((s & 0xFF) > 8) continue; // rare

        // Must be inside a cave (air tile underground)
        for (int ly = 2; ly < CHUNK_SIZE - 1; ++ly) {
            int ty = cy * CHUNK_SIZE + ly;
            if (world.getTileType(tx, ty) == TileType::Air
             && world.isSolid(tx, ty + 1))
            {
                world.setDecoTile(tx, ty, TileType::Mushroom);
                break;
            }
        }
    }
}
