#pragma once
#include "world/Tile.h"
#include <cstdint>
#include <string_view>

// ---- Item IDs ---------------------------------------------------------------
// Block items mirror TileType IDs (0-99).
// Tool items start at 100.
// Consumable items start at 200.

enum class ItemType : uint16_t {
    None = 0,
    // ---- Blocks (match TileType values) ----
    Grass     = 1,
    Dirt      = 2,
    Stone     = 3,
    Sand      = 4,
    Gravel    = 5,
    Wood      = 6,
    Leaves    = 7,
    Coal      = 8,
    Iron      = 9,
    Gold      = 10,
    Diamond   = 11,
    Torch     = 12,
    Cobblestone = 19,
    // ---- Tools ----
    WoodPickaxe  = 100,
    StonePickaxe = 101,
    IronPickaxe  = 102,
    GoldPickaxe  = 103,
    DiamondPickaxe = 104,
    WoodAxe      = 110,
    WoodShovel   = 120,
    WoodSword    = 130,
    // ---- Consumables ----
    HealingPotion = 200,
    _Count
};

// ---- Item stack (what a slot holds) ----------------------------------------
struct ItemStack {
    ItemType type  = ItemType::None;
    uint8_t  count = 0;

    bool empty() const { return type == ItemType::None || count == 0; }
    void clear() { type = ItemType::None; count = 0; }
};

// ---- Static item properties -------------------------------------------------
struct ItemProperties {
    const char* name       = "None";
    bool        isBlock    = false;  // Can be placed as a tile
    TileType    placesAs   = TileType::Air;
    bool        isTool     = false;
    float       mineSpeed  = 1.0f;  // Multiplier for mining speed
    float       tileBonus  = 1.0f;  // Extra bonus on matching tile type
    int         atlasX     = 0;     // For tools without a tile (future spritesheet)
    int         atlasY     = 0;
    uint8_t     maxStack   = 64;
    int         damage     = 0;     // Weapon damage
};

// ---- Item registry (constexpr table) ----------------------------------------
inline const ItemProperties& getItemProperties(ItemType t) {
    // Indexed by (int)t — sparse, so use a switch
    static ItemProperties table[(int)ItemType::_Count] = {};
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        auto& p = table;
        // Blocks
        p[(int)ItemType::Grass]     = {"Grass",      true,  TileType::Grass,     false, 1.f, 1.f, 1,0, 64, 0};
        p[(int)ItemType::Dirt]      = {"Dirt",       true,  TileType::Dirt,      false, 1.f, 1.f, 2,0, 64, 0};
        p[(int)ItemType::Stone]     = {"Stone",      true,  TileType::Stone,     false, 1.f, 1.f, 3,0, 64, 0};
        p[(int)ItemType::Sand]      = {"Sand",       true,  TileType::Sand,      false, 1.f, 1.f, 4,0, 64, 0};
        p[(int)ItemType::Gravel]    = {"Gravel",     true,  TileType::Gravel,    false, 1.f, 1.f, 5,0, 64, 0};
        p[(int)ItemType::Wood]      = {"Wood",       true,  TileType::Wood,      false, 1.f, 1.f, 6,0, 64, 0};
        p[(int)ItemType::Leaves]    = {"Leaves",     true,  TileType::Leaves,    false, 1.f, 1.f, 7,0, 64, 0};
        p[(int)ItemType::Coal]      = {"Coal",       true,  TileType::Coal,      false, 1.f, 1.f, 0,1, 64, 0};
        p[(int)ItemType::Iron]      = {"Iron",       true,  TileType::Iron,      false, 1.f, 1.f, 1,1, 64, 0};
        p[(int)ItemType::Gold]      = {"Gold",       true,  TileType::Gold,      false, 1.f, 1.f, 2,1, 64, 0};
        p[(int)ItemType::Diamond]   = {"Diamond",    true,  TileType::Diamond,   false, 1.f, 1.f, 3,1, 64, 0};
        p[(int)ItemType::Torch]     = {"Torch",      true,  TileType::Torch,     false, 1.f, 1.f, 4,1, 64, 0};
        p[(int)ItemType::Cobblestone]={"Cobblestone",true,  TileType::Cobblestone,false,1.f, 1.f, 3,2, 64, 0};
        // Tools
        p[(int)ItemType::WoodPickaxe]    = {"Wood Pickaxe",   false, TileType::Air, true, 1.5f, 1.f, 0,3, 1, 0};
        p[(int)ItemType::StonePickaxe]   = {"Stone Pickaxe",  false, TileType::Air, true, 2.5f, 1.f, 1,3, 1, 0};
        p[(int)ItemType::IronPickaxe]    = {"Iron Pickaxe",   false, TileType::Air, true, 4.0f, 1.f, 2,3, 1, 0};
        p[(int)ItemType::GoldPickaxe]    = {"Gold Pickaxe",   false, TileType::Air, true, 5.0f, 1.f, 3,3, 1, 0};
        p[(int)ItemType::DiamondPickaxe] = {"Diamond Pickaxe",false, TileType::Air, true, 8.0f, 1.f, 4,3, 1, 0};
        p[(int)ItemType::WoodAxe]        = {"Wood Axe",       false, TileType::Air, true, 1.5f, 2.f, 5,3, 1, 0};
        p[(int)ItemType::WoodShovel]     = {"Wood Shovel",    false, TileType::Air, true, 1.5f, 2.f, 6,3, 1, 0};
        p[(int)ItemType::WoodSword]      = {"Wood Sword",     false, TileType::Air, true, 1.f,  1.f, 7,3, 1, 5};
    }
    int idx = (int)t;
    if (idx < 0 || idx >= (int)ItemType::_Count) return table[0];
    return table[idx];
}

// Convert mined TileType → dropped ItemType
inline ItemType tileToItem(TileType t) {
    switch (t) {
        case TileType::Grass:      return ItemType::Dirt;        // grass drops dirt
        case TileType::Dirt:       return ItemType::Dirt;
        case TileType::Stone:      return ItemType::Cobblestone; // stone drops cobblestone
        case TileType::Sand:       return ItemType::Sand;
        case TileType::Gravel:     return ItemType::Gravel;
        case TileType::Wood:       return ItemType::Wood;
        case TileType::Leaves:     return ItemType::None;        // leaves drop nothing
        case TileType::Coal:       return ItemType::Coal;
        case TileType::Iron:       return ItemType::Iron;
        case TileType::Gold:       return ItemType::Gold;
        case TileType::Diamond:    return ItemType::Diamond;
        case TileType::Torch:      return ItemType::Torch;
        case TileType::Cobblestone:return ItemType::Cobblestone;
        default:                   return ItemType::None;
    }
}
