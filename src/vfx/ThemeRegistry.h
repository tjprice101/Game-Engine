#pragma once
#include "vfx/Theme.h"
#include <unordered_map>
#include <string>
#include <vector>

// ---- ThemeRegistry ----------------------------------------------------------
// Centralised registry of all musical composition themes.
// Themes drive every visual system: bloom, particles, trails, color grading.
//
// Usage:
//   ThemeRegistry::instance().loadDefaults();    // registers all 10 built-ins
//   ThemeRegistry::instance().setActive("Fate"); // switches active theme
//   const Theme* t = ThemeRegistry::instance().active();

class ThemeRegistry {
public:
    static ThemeRegistry& instance();

    // ---- Registration -------------------------------------------------------
    void         add(Theme theme);
    const Theme* get(const std::string& name) const;
    const Theme* active() const   { return m_active; }
    void         setActive(const std::string& name);
    void         setActive(const Theme* theme) { m_active = theme; }

    // ---- Built-in themes (MagnumOpus-inspired) ------------------------------
    static Theme makeLaCampanella();   // Black → Orange → Gold  — fire/bells
    static Theme makeEroica();         // Scarlet → Gold          — heroic/sakura
    static Theme makeSwanLake();       // White/Black + iridescent — feathers/grace
    static Theme makeMoonlightSonata();// Dark Purple → Light Blue — lunar/ethereal
    static Theme makeEnigmaVariations();// Black → Purple → Green — void/mystery
    static Theme makeFate();           // Black → Pink → Red → White — cosmic/endgame
    static Theme makeSpring();         // Soft Green → Pink       — floral renewal
    static Theme makeSummer();         // Vivid Yellow → Orange   — vibrant heat
    static Theme makeAutumn();         // Orange → Deep Red       — nostalgic warmth
    static Theme makeWinter();         // White → Ice Blue        — crystalline serenity

    // ---- Init ---------------------------------------------------------------
    // Registers all 10 built-in themes. Call once at startup.
    void loadDefaults();

    const std::vector<std::string>& names() const { return m_order; }

private:
    ThemeRegistry() = default;

    std::unordered_map<std::string, Theme> m_themes;
    std::vector<std::string>               m_order;
    const Theme*                           m_active = nullptr;
};
