#pragma once
#include "Item.h"
#include <array>
#include <cstdint>

// ---- Inventory --------------------------------------------------------------
// 40-slot inventory (4 rows x 10 cols) + 10-slot hotbar
// Hotbar is slots 0-9, main inventory is slots 10-39.

class Inventory {
public:
    static constexpr int HOTBAR_SIZE = 10;
    static constexpr int MAIN_ROWS   = 4;
    static constexpr int TOTAL_SLOTS = HOTBAR_SIZE + HOTBAR_SIZE * MAIN_ROWS; // 50

    Inventory() { m_slots.fill(ItemStack{}); }

    // ---- Slot access --------------------------------------------------------
    ItemStack& slot(int i)             { return m_slots[i]; }
    const ItemStack& slot(int i) const { return m_slots[i]; }

    ItemStack& hotbar(int i)             { return m_slots[i]; }
    const ItemStack& hotbar(int i) const { return m_slots[i]; }

    int hotbarSelected() const { return m_hotbarSelected; }
    void setHotbarSelected(int i) { m_hotbarSelected = i; }

    const ItemStack& selectedStack() const { return m_slots[m_hotbarSelected]; }

    // ---- Add items ----------------------------------------------------------
    // Returns count that could not be added (0 = all added)
    int addItem(ItemType type, int count = 1) {
        if (type == ItemType::None || count <= 0) return 0;
        const ItemProperties& props = getItemProperties(type);
        int maxStack = props.maxStack;

        // Fill existing stacks first
        for (auto& s : m_slots) {
            if (s.type == type && s.count < maxStack) {
                int canAdd = std::min(count, (int)(maxStack - s.count));
                s.count += (uint8_t)canAdd;
                count   -= canAdd;
                if (count <= 0) return 0;
            }
        }
        // Fill empty slots
        for (auto& s : m_slots) {
            if (s.empty()) {
                int canAdd = std::min(count, (int)maxStack);
                s.type  = type;
                s.count = (uint8_t)canAdd;
                count  -= canAdd;
                if (count <= 0) return 0;
            }
        }
        return count; // leftover
    }

    // Remove items from selected hotbar slot
    bool consumeSelected(int count = 1) {
        auto& s = m_slots[m_hotbarSelected];
        if (s.empty() || s.count < count) return false;
        s.count -= (uint8_t)count;
        if (s.count == 0) s.clear();
        return true;
    }

    // Check if inventory has at least `count` of `type`
    bool has(ItemType type, int count = 1) const {
        int total = 0;
        for (const auto& s : m_slots) {
            if (s.type == type) total += s.count;
            if (total >= count) return true;
        }
        return false;
    }

private:
    std::array<ItemStack, TOTAL_SLOTS> m_slots;
    int m_hotbarSelected = 0;
};
