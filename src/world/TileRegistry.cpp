#include "TileRegistry.h"
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

// Atlas layout: atlasX = (int)type % ATLAS_GRID, atlasY = (int)type / ATLAS_GRID
// Row 0 (0-7):  Air, Grass, Dirt, Stone, Sand, Gravel, Wood, Leaves
// Row 1 (8-15): Coal, Iron, Gold, Diamond, Copper, Tin, Silver, Torch
// Row 2(16-23): Water, Lava, Bedrock, Snow, Ice, Sandstone, Cobblestone, Obsidian
// Row 3(24-31): Glowstone, DirtWall, StoneWall, WoodWall, GraniteWall, Cactus, Mushroom, GrassDecor
// Row 4(32-39): Plank, Brick, Glass, Chest, Workbench, Furnace, Moss, Vine

TileRegistry& TileRegistry::instance() {
    static TileRegistry reg;
    return reg;
}

TileRegistry::TileRegistry() { registerTiles(); }

const TileProperties& TileRegistry::get(TileType t) const {
    int idx = (int)t;
    if (idx < 0 || idx >= (int)TileType::_Count) idx = 0;
    return m_props[idx];
}

// ---- Helper macro to compute atlas coords from tile ID ----------------------
#define AX(id) ((id) % ATLAS_GRID)
#define AY(id) ((id) / ATLAS_GRID)

void TileRegistry::registerTiles() {
    auto& p = m_props;
    // Fields: name, solid, transparent, liquid, emitsLight, isWallTile,
    //         lightColor, lightRadius, hardness, atlasX, atlasY,
    //         friction, maxStack, emissiveStr, specular, normalStr

    // ---- Row 0: Basic terrain -----------------------------------------------
    p[ 0] = { "Air",         false, true,  false, false, false, {0,0,0},        0.f,  0.f, AX( 0),AY( 0), 0.f,   0, 0.f, 0.f, 1.f };
    p[ 1] = { "Grass",       true,  false, false, false, false, {0,0,0},        0.f,  1.0f, AX( 1),AY( 1), 0.8f, 64, 0.f, 0.1f, 1.f };
    p[ 2] = { "Dirt",        true,  false, false, false, false, {0,0,0},        0.f,  0.5f, AX( 2),AY( 2), 0.8f, 64, 0.f, 0.0f, 1.f };
    p[ 3] = { "Stone",       true,  false, false, false, false, {0,0,0},        0.f,  1.5f, AX( 3),AY( 3), 0.8f, 64, 0.f, 0.2f, 1.f };
    p[ 4] = { "Sand",        true,  false, false, false, false, {0,0,0},        0.f,  0.5f, AX( 4),AY( 4), 0.5f, 64, 0.f, 0.0f, 0.5f };
    p[ 5] = { "Gravel",      true,  false, false, false, false, {0,0,0},        0.f,  0.6f, AX( 5),AY( 5), 0.5f, 64, 0.f, 0.0f, 0.5f };
    p[ 6] = { "Wood",        true,  false, false, false, false, {0,0,0},        0.f,  1.0f, AX( 6),AY( 6), 0.8f, 64, 0.f, 0.1f, 1.f };
    p[ 7] = { "Leaves",      true,  true,  false, false, false, {0,0,0},        0.f,  0.3f, AX( 7),AY( 7), 0.8f, 64, 0.f, 0.0f, 0.5f };

    // ---- Row 1: Ores + Torch + Fluids ---------------------------------------
    p[ 8] = { "Coal",        true,  false, false, false, false, {0,0,0},        0.f,  2.0f, AX( 8),AY( 8), 0.8f, 64, 0.f, 0.1f, 1.f };
    p[ 9] = { "Iron",        true,  false, false, false, false, {0,0,0},        0.f,  3.0f, AX( 9),AY( 9), 0.8f, 64, 0.f, 0.4f, 1.f };
    p[10] = { "Gold",        true,  false, false, false, false, {0,0,0},        0.f,  4.0f, AX(10),AY(10), 0.8f, 64, 0.f, 0.7f, 1.f };
    p[11] = { "Diamond",     true,  false, false, false, false, {0,0,0},        0.f,  5.0f, AX(11),AY(11), 0.8f, 64, 0.f, 0.9f, 1.f };
    p[12] = { "Copper",      true,  false, false, false, false, {0,0,0},        0.f,  2.5f, AX(12),AY(12), 0.8f, 64, 0.f, 0.5f, 1.f };
    p[13] = { "Tin",         true,  false, false, false, false, {0,0,0},        0.f,  2.0f, AX(13),AY(13), 0.8f, 64, 0.f, 0.4f, 1.f };
    p[14] = { "Silver",      true,  false, false, false, false, {0,0,0},        0.f,  3.5f, AX(14),AY(14), 0.8f, 64, 0.f, 0.8f, 1.f };
    p[15] = { "Torch",       false, true,  false, true,  false, {1.f,.7f,.3f}, 10.f,  0.1f, AX(15),AY(15), 0.f,  64, 0.9f, 0.f, 0.5f };

    // ---- Row 2: Fluids + Special blocks -------------------------------------
    p[16] = { "Water",       false, true,  true,  false, false, {0,0,0},        0.f,  0.0f, AX(16),AY(16), 0.f,  64, 0.f, 0.3f, 0.5f };
    p[17] = { "Lava",        false, true,  true,  true,  false, {1.f,.4f,.1f}, 14.f,  0.0f, AX(17),AY(17), 0.f,  64, 1.f, 0.2f, 0.5f };
    p[18] = { "Bedrock",     true,  false, false, false, false, {0,0,0},        0.f, 99.f,  AX(18),AY(18), 0.8f, 64, 0.f, 0.1f, 1.f };
    p[19] = { "Snow",        true,  false, false, false, false, {0,0,0},        0.f,  0.2f, AX(19),AY(19), 0.3f, 64, 0.f, 0.1f, 0.5f };
    p[20] = { "Ice",         true,  false, false, false, false, {0,0,0},        0.f,  0.5f, AX(20),AY(20), 0.05f,64, 0.f, 0.8f, 0.5f };
    p[21] = { "Sandstone",   true,  false, false, false, false, {0,0,0},        0.f,  1.2f, AX(21),AY(21), 0.8f, 64, 0.f, 0.1f, 1.f };
    p[22] = { "Cobblestone", true,  false, false, false, false, {0,0,0},        0.f,  1.5f, AX(22),AY(22), 0.8f, 64, 0.f, 0.2f, 1.f };
    p[23] = { "Obsidian",    true,  false, false, false, false, {0,0,0},        0.f,  9.0f, AX(23),AY(23), 0.8f, 64, 0.f, 0.5f, 1.f };

    // ---- Row 3: Glowstone + Walls + Vegetation ------------------------------
    p[24] = { "Glowstone",   true,  false, false, true,  false, {.9f,.8f,.4f}, 18.f,  2.0f, AX(24),AY(24), 0.8f, 64, 1.f, 0.3f, 0.5f };
    p[25] = { "DirtWall",    false, true,  false, false, true,  {0,0,0},        0.f,  0.3f, AX(25),AY(25), 0.8f, 64, 0.f, 0.0f, 1.f };
    p[26] = { "StoneWall",   false, true,  false, false, true,  {0,0,0},        0.f,  1.0f, AX(26),AY(26), 0.8f, 64, 0.f, 0.1f, 1.f };
    p[27] = { "WoodWall",    false, true,  false, false, true,  {0,0,0},        0.f,  0.8f, AX(27),AY(27), 0.8f, 64, 0.f, 0.0f, 1.f };
    p[28] = { "GraniteWall", false, true,  false, false, true,  {0,0,0},        0.f,  1.2f, AX(28),AY(28), 0.8f, 64, 0.f, 0.2f, 1.f };
    p[29] = { "Cactus",      true,  false, false, false, false, {0,0,0},        0.f,  0.4f, AX(29),AY(29), 0.8f, 64, 0.f, 0.0f, 0.8f };
    p[30] = { "Mushroom",    false, true,  false, false, false, {0,0,0},        0.f,  0.2f, AX(30),AY(30), 0.f,  64, 0.f, 0.0f, 0.5f };
    p[31] = { "GrassDecor",  false, true,  false, false, false, {0,0,0},        0.f,  0.1f, AX(31),AY(31), 0.f,  64, 0.f, 0.0f, 0.3f };

    // ---- Row 4: Crafted blocks ----------------------------------------------
    p[32] = { "Plank",       true,  false, false, false, false, {0,0,0},        0.f,  0.8f, AX(32),AY(32), 0.8f, 64, 0.f, 0.1f, 1.f };
    p[33] = { "Brick",       true,  false, false, false, false, {0,0,0},        0.f,  2.0f, AX(33),AY(33), 0.8f, 64, 0.f, 0.3f, 1.f };
    p[34] = { "Glass",       true,  true,  false, false, false, {0,0,0},        0.f,  0.5f, AX(34),AY(34), 0.8f, 64, 0.f, 0.9f, 0.2f };
    p[35] = { "Chest",       true,  false, false, false, false, {0,0,0},        0.f,  1.0f, AX(35),AY(35), 0.8f, 64, 0.f, 0.2f, 1.f };
    p[36] = { "Workbench",   true,  false, false, false, false, {0,0,0},        0.f,  1.0f, AX(36),AY(36), 0.8f, 64, 0.f, 0.1f, 1.f };
    p[37] = { "Furnace",     true,  false, false, false, false, {0,0,0},        0.f,  1.5f, AX(37),AY(37), 0.8f, 64, 0.f, 0.1f, 1.f };
    p[38] = { "Moss",        false, true,  false, false, false, {0,0,0},        0.f,  0.1f, AX(38),AY(38), 0.8f, 64, 0.f, 0.0f, 0.5f };
    p[39] = { "Vine",        false, true,  false, false, false, {0,0,0},        0.f,  0.1f, AX(39),AY(39), 0.8f, 64, 0.f, 0.0f, 0.5f };
}

#undef AX
#undef AY

// ---- Atlas texture generation -----------------------------------------------
// Atlas layout: 8 columns × 5 rows of 16×16 tiles = 128×80 pixels
// We generate a procedural pixel-art atlas for all 40 tile types.

void TileRegistry::buildAtlas() {
    const int W = ATLAS_PIXEL_SIZE;
    const int H = ATLAS_PIXEL_SIZE;
    std::vector<uint8_t> pixels(W * H * 4, 0);

    // Row 0 ---------------------------------------------------------------
    // 0: Air (transparent)
    paintGrass    (pixels.data(),  1, 0);                            // 1: Grass
    paintVariance (pixels.data(),  2, 0, 134, 96,  54, 15);         // 2: Dirt
    paintChecked  (pixels.data(),  3, 0, 110,110,110,  90, 90, 90); // 3: Stone
    paintVariance (pixels.data(),  4, 0, 210,190, 120, 12);         // 4: Sand
    paintChecked  (pixels.data(),  5, 0, 130,120,110, 100, 95, 90); // 5: Gravel
    paintVariance (pixels.data(),  6, 0, 170,130,  80, 10);         // 6: Wood (log)
    paintVariance (pixels.data(),  7, 0,  50,130,  50, 20);         // 7: Leaves

    // Row 1 ---------------------------------------------------------------
    paintChecked  (pixels.data(),  0, 1, 100,100,100,  35, 35, 35); // 8:  Coal ore
    paintChecked  (pixels.data(),  1, 1, 100,100,100, 170,120, 80); // 9:  Iron ore
    paintChecked  (pixels.data(),  2, 1, 100,100,100, 200,180,  0); // 10: Gold ore
    paintChecked  (pixels.data(),  3, 1, 100,100,100,  80,200,230); // 11: Diamond ore
    paintChecked  (pixels.data(),  4, 1, 100,100,100, 200,100, 50); // 12: Copper ore
    paintChecked  (pixels.data(),  5, 1, 100,100,100, 160,160,180); // 13: Tin ore
    paintChecked  (pixels.data(),  6, 1, 100,100,100, 192,192,200); // 14: Silver ore
    paintSolid    (pixels.data(),  7, 1, 240,180, 40);              // 15: Torch (flame yellow)

    // Row 2 ---------------------------------------------------------------
    // Water (semi-transparent blue)
    for (int py = 0; py < TILE_PIXEL; ++py)
        for (int px = 0; px < TILE_PIXEL; ++px) {
            int v = ((px ^ py) & 3) * 8;
            setPixel(pixels.data(), 0*TILE_PIXEL+px, 2*TILE_PIXEL+py,
                     (uint8_t)(50+v), (uint8_t)(90+v), (uint8_t)(200+v/2), 180);
        }
    paintVariance (pixels.data(),  1, 2, 200, 80,  10, 30);         // 17: Lava
    paintChecked  (pixels.data(),  2, 2,  40, 40, 40,   25, 25, 25); // 18: Bedrock
    paintVariance (pixels.data(),  3, 2, 230,240,255,  8);           // 19: Snow
    paintSolid    (pixels.data(),  4, 2, 140,180,220);               // 20: Ice (light blue)
    paintVariance (pixels.data(),  5, 2, 200,175,100,  8);           // 21: Sandstone
    paintChecked  (pixels.data(),  6, 2, 120,115,110,  80, 78, 75);  // 22: Cobblestone
    paintSolid    (pixels.data(),  7, 2,  20, 10, 30);               // 23: Obsidian (dark purple)

    // Row 3 ---------------------------------------------------------------
    // Glowstone: bright yellow-white
    paintVariance (pixels.data(),  0, 3, 240,230,150, 20);           // 24: Glowstone
    paintVariance (pixels.data(),  1, 3, 110, 78, 40, 10);           // 25: DirtWall (darker dirt)
    paintChecked  (pixels.data(),  2, 3,  80, 80, 80,  60, 60, 60);  // 26: StoneWall (darker stone)
    paintVariance (pixels.data(),  3, 3, 140,100, 60,  8);           // 27: WoodWall
    paintChecked  (pixels.data(),  4, 3,  90, 85, 80,  70, 65, 60);  // 28: GraniteWall
    // Cactus: dark green with spine details
    for (int py = 0; py < TILE_PIXEL; ++py)
        for (int px = 0; px < TILE_PIXEL; ++px) {
            bool spine = (px == 0 || px == TILE_PIXEL-1) && (py % 4 == 0);
            setPixel(pixels.data(), 5*TILE_PIXEL+px, 3*TILE_PIXEL+py,
                     spine ? 200 : 40, spine ? 200 : 120, spine ? 200 : 30, 255);
        }
    // Mushroom: red cap with white dots, brown stem
    for (int py = 0; py < TILE_PIXEL; ++py)
        for (int px = 0; px < TILE_PIXEL; ++px) {
            bool cap  = py < TILE_PIXEL/2;
            bool dot  = cap && ((px==4&&py==2)||(px==10&&py==3)||(px==7&&py==1));
            bool stem = !cap && (px > TILE_PIXEL/4 && px < 3*TILE_PIXEL/4);
            uint8_t r = stem ? 160 : (cap ? 200 : 0);
            uint8_t g = stem ? 110 : (cap ? 30  : 0);
            uint8_t b = stem ? 60  : (cap ? 30  : 0);
            if (dot) { r=240; g=240; b=240; }
            uint8_t a = (cap || stem) ? 255 : 0;
            setPixel(pixels.data(), 6*TILE_PIXEL+px, 3*TILE_PIXEL+py, r, g, b, a);
        }
    // GrassDecor: sparse thin blades
    for (int py = 0; py < TILE_PIXEL; ++py)
        for (int px = 0; px < TILE_PIXEL; ++px) {
            bool blade = (px % 4 == 2) && (py > TILE_PIXEL/3);
            int  fade  = blade ? std::max(0, TILE_PIXEL - py*2) : 0;
            setPixel(pixels.data(), 7*TILE_PIXEL+px, 3*TILE_PIXEL+py,
                     (uint8_t)(30+fade/4), (uint8_t)(140+fade/4), (uint8_t)30, blade ? 200 : 0);
        }

    // Row 4 ---------------------------------------------------------------
    paintVariance (pixels.data(),  0, 4, 180,140, 90, 12);           // 32: Plank
    paintChecked  (pixels.data(),  1, 4, 170, 80, 60,  130, 60, 45); // 33: Brick
    // Glass: very pale blue, semi-transparent
    for (int py = 0; py < TILE_PIXEL; ++py)
        for (int px = 0; px < TILE_PIXEL; ++px) {
            bool edge = (px==0||px==TILE_PIXEL-1||py==0||py==TILE_PIXEL-1);
            setPixel(pixels.data(), 2*TILE_PIXEL+px, 4*TILE_PIXEL+py,
                     180, 210, 240, edge ? 200 : 80);
        }
    // Chest: brown box with gold clasp
    paintVariance (pixels.data(),  3, 4, 160,110, 55, 10);           // 35: Chest base
    for (int py = 5; py < 11; ++py)                                  // gold clasp
        for (int px = 5; px < 11; ++px)
            if (px == 5||px == 10||py == 5||py == 10)
                setPixel(pixels.data(), 3*TILE_PIXEL+px, 4*TILE_PIXEL+py, 220,180,0,255);
    // Workbench: checkered with tool marks
    paintChecked  (pixels.data(),  4, 4, 160,110, 55,  190,150, 80); // 36: Workbench
    setPixel(pixels.data(), 4*TILE_PIXEL+4, 4*TILE_PIXEL+4, 80,60,40,255);
    setPixel(pixels.data(), 4*TILE_PIXEL+10,4*TILE_PIXEL+10,80,60,40,255);
    // Furnace: grey with glowing window
    paintSolid    (pixels.data(),  5, 4,  90, 90, 90);               // 37: Furnace
    for (int py = 4; py < 12; ++py)
        for (int px = 4; px < 12; ++px)
            setPixel(pixels.data(), 5*TILE_PIXEL+px, 4*TILE_PIXEL+py, 200,100,30,255);
    // Moss: green speckling
    paintVariance (pixels.data(),  6, 4,  60,130, 50, 25);           // 38: Moss
    // Vine: dark green vertical strands
    for (int py = 0; py < TILE_PIXEL; ++py)
        for (int px = 0; px < TILE_PIXEL; ++px) {
            bool strand = (px % 5 < 2);
            setPixel(pixels.data(), 7*TILE_PIXEL+px, 4*TILE_PIXEL+py,
                     30, strand?100:60, 20, strand ? 220 : 0);
        }

    m_atlas.createFromData(W, H, pixels.data());
    m_atlas.setFiltering(GL_NEAREST, GL_NEAREST);
}

// ---- Pixel helpers ----------------------------------------------------------
void TileRegistry::setPixel(uint8_t* pixels, int px, int py, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int idx = (py * ATLAS_PIXEL_SIZE + px) * 4;
    pixels[idx+0] = r; pixels[idx+1] = g; pixels[idx+2] = b; pixels[idx+3] = a;
}

void TileRegistry::paintSolid(uint8_t* pixels, int ax, int ay, uint8_t r, uint8_t g, uint8_t b) {
    for (int py = 0; py < TILE_PIXEL; ++py)
        for (int px = 0; px < TILE_PIXEL; ++px)
            setPixel(pixels, ax*TILE_PIXEL+px, ay*TILE_PIXEL+py, r, g, b);
}

void TileRegistry::paintVariance(uint8_t* pixels, int ax, int ay,
                                  uint8_t r, uint8_t g, uint8_t b, int var)
{
    unsigned seed = (unsigned)(ax*31+ay*97+13);
    for (int py = 0; py < TILE_PIXEL; ++py) {
        for (int px = 0; px < TILE_PIXEL; ++px) {
            seed = seed * 1664525u + 1013904223u;
            int v = (int)(seed >> 24) % (var*2+1) - var;
            int nr = std::clamp((int)r + v, 0, 255);
            int ng = std::clamp((int)g + v, 0, 255);
            int nb = std::clamp((int)b + v, 0, 255);
            setPixel(pixels, ax*TILE_PIXEL+px, ay*TILE_PIXEL+py,
                     (uint8_t)nr, (uint8_t)ng, (uint8_t)nb);
        }
    }
}

void TileRegistry::paintChecked(uint8_t* pixels, int ax, int ay,
                                  uint8_t r1, uint8_t g1, uint8_t b1,
                                  uint8_t r2, uint8_t g2, uint8_t b2)
{
    for (int py = 0; py < TILE_PIXEL; ++py) {
        for (int px = 0; px < TILE_PIXEL; ++px) {
            bool c = ((px/4 + py/4) % 2 == 0);
            if (c) setPixel(pixels, ax*TILE_PIXEL+px, ay*TILE_PIXEL+py, r1,g1,b1);
            else   setPixel(pixels, ax*TILE_PIXEL+px, ay*TILE_PIXEL+py, r2,g2,b2);
        }
    }
}

void TileRegistry::paintGrass(uint8_t* pixels, int ax, int ay) {
    unsigned seed = 12345u;
    for (int py = 0; py < TILE_PIXEL; ++py) {
        for (int px = 0; px < TILE_PIXEL; ++px) {
            seed = seed * 1664525u + 1013904223u;
            int v = (int)(seed >> 25) % 11 - 5;
            uint8_t r, g, b;
            if (py < 3) { r=(uint8_t)std::clamp(70+v,0,255);  g=(uint8_t)std::clamp(140+v,0,255); b=(uint8_t)std::clamp(40+v,0,255); }
            else         { r=(uint8_t)std::clamp(120+v,0,255); g=(uint8_t)std::clamp(80+v,0,255);  b=(uint8_t)std::clamp(40+v,0,255); }
            setPixel(pixels, ax*TILE_PIXEL+px, ay*TILE_PIXEL+py, r, g, b);
        }
    }
}
