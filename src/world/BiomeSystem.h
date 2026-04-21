#pragma once
#include "world/Noise.h"
#include <glm/glm.hpp>
#include <string>

// ---- Biome definitions ------------------------------------------------------
enum class BiomeType : uint8_t {
    Plains      = 0,
    Desert      = 1,
    Tundra      = 2,
    Forest      = 3,
    Jungle      = 4,
    Underground = 5,
    DeepCave    = 6,
    Hell        = 7,
    _Count      = 8
};

struct BiomeProperties {
    const char* name;
    // Surface tile
    int surfaceTileAtlasX, surfaceTileAtlasY;
    // Sub-surface depth before stone
    int dirtDepth;
    // Tree probability (0..1)
    float treeProbability;
    // Ore density multiplier
    float oreDensity;
    // Ambient light color (underground base)
    glm::vec3 ambientColor;
    // Sky color at noon
    glm::vec3 skyColor;
    // Background wall type
    int wallAtlasX, wallAtlasY;
};

class BiomeSystem {
public:
    static const BiomeProperties& get(BiomeType t) {
        static const BiomeProperties props[(int)BiomeType::_Count] = {
          // name,       surfX,Y, dirtD,  tree,   ore,   ambient,            sky,              wallX,Y
          { "Plains",     1, 0,   5,      0.10f,  1.0f, {0.05f,0.05f,0.08f},{0.38f,0.65f,0.92f}, 25, 0 },
          { "Desert",     4, 0,   8,      0.0f,   0.8f, {0.06f,0.05f,0.04f},{0.82f,0.72f,0.45f}, 21, 0 },
          { "Tundra",    19, 0,   4,      0.04f,  1.1f, {0.05f,0.06f,0.08f},{0.70f,0.82f,0.95f}, 25, 0 },
          { "Forest",     1, 0,   5,      0.22f,  0.9f, {0.04f,0.06f,0.04f},{0.30f,0.60f,0.30f}, 25, 0 },
          { "Jungle",     1, 0,   3,      0.30f,  1.2f, {0.03f,0.06f,0.03f},{0.20f,0.55f,0.25f}, 27, 0 },
          { "Underground",3, 0,   0,      0.0f,   1.0f, {0.02f,0.02f,0.02f},{0.02f,0.02f,0.06f}, 26, 0 },
          { "DeepCave",   3, 0,   0,      0.0f,   1.5f, {0.01f,0.01f,0.01f},{0.01f,0.01f,0.03f}, 26, 0 },
          { "Hell",      14, 0,   0,      0.0f,   0.5f, {0.08f,0.02f,0.01f},{0.20f,0.05f,0.02f}, 23, 0 },
        };
        int i = (int)t;
        if (i < 0 || i >= (int)BiomeType::_Count) i = 0;
        return props[i];
    }

    // Determine biome at world tile coordinates
    static BiomeType evaluate(int tx, int ty, int surfaceY, unsigned seed) {
        Noise n(seed ^ 0xBEEF);
        float moisture = n.noise2(tx * 0.015f, 0.3f);
        float temp     = n.noise2(tx * 0.012f + 100.f, 0.3f);

        int depth = ty - surfaceY;

        if (depth > 120) return BiomeType::Hell;
        if (depth > 60)  return BiomeType::DeepCave;
        if (depth > 15)  return BiomeType::Underground;

        if (temp   < -0.30f) return BiomeType::Tundra;
        if (moisture > 0.30f && temp > 0.20f) return BiomeType::Jungle;
        if (moisture > 0.15f) return BiomeType::Forest;
        if (moisture < -0.25f) return BiomeType::Desert;
        return BiomeType::Plains;
    }

    // Blend weights for smooth biome transitions (returns weights for each biome)
    static void blendWeights(int tx, int ty, int surfaceY, unsigned seed,
                              float weights[(int)BiomeType::_Count])
    {
        for (int i = 0; i < (int)BiomeType::_Count; ++i) weights[i] = 0.f;
        BiomeType center = evaluate(tx, ty, surfaceY, seed);
        weights[(int)center] = 1.f;
        // TODO: sample neighbors for smooth blending
    }
};
