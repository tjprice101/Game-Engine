#pragma once
#include "world/Tile.h"

class World;

// ---- StructureGen -----------------------------------------------------------
// Post-generation pass that places structures into already-filled chunks.
// Called from World::generateChunk after terrain fill.

class StructureGen {
public:
    // Main entry: dispatch all structure types for this chunk
    static void decorate(World& world, int cx, int cy, unsigned seed);

private:
    // ---- Individual generators -------------------------------------------
    static void placeDungeon  (World& world, int cx, int cy, unsigned seed);
    static void placeCabin    (World& world, int cx, int cy, unsigned seed);
    static void placeMineshaft(World& world, int cx, int cy, unsigned seed);
    static void placeCactus   (World& world, int cx, int cy, unsigned seed);
    static void placeMushrooms(World& world, int cx, int cy, unsigned seed);

    // ---- Helpers -----------------------------------------------------------
    // Fill a rectangle in the Main layer
    static void fillRect(World& world,
                          int tx, int ty, int w, int h, TileType t);
    // Draw a hollow box (walls only)
    static void hollowRect(World& world,
                            int tx, int ty, int w, int h,
                            TileType wall, TileType fill);
};
