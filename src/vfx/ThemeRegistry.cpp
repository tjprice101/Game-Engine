#include "vfx/ThemeRegistry.h"
#include <stdexcept>

ThemeRegistry& ThemeRegistry::instance() {
    static ThemeRegistry reg;
    return reg;
}

void ThemeRegistry::add(Theme theme) {
    m_order.push_back(theme.name);
    m_themes[theme.name] = std::move(theme);
}

const Theme* ThemeRegistry::get(const std::string& name) const {
    auto it = m_themes.find(name);
    return it != m_themes.end() ? &it->second : nullptr;
}

void ThemeRegistry::setActive(const std::string& name) {
    auto it = m_themes.find(name);
    if (it == m_themes.end())
        throw std::runtime_error("ThemeRegistry: unknown theme '" + name + "'");
    m_active = &it->second;
}

void ThemeRegistry::loadDefaults() {
    add(makeLaCampanella());
    add(makeEroica());
    add(makeSwanLake());
    add(makeMoonlightSonata());
    add(makeEnigmaVariations());
    add(makeFate());
    add(makeSpring());
    add(makeSummer());
    add(makeAutumn());
    add(makeWinter());
    // Default to Moonlight Sonata (balanced and ethereal)
    setActive("MoonlightSonata");
}

// ============================================================================
//  Predefined themes
//  Inspired by MagnumOpus musical compositions
// ============================================================================

Theme ThemeRegistry::makeLaCampanella() {
    Theme t;
    t.name       = "LaCampanella";
    t.musicTrack = "music/la_campanella.ogg";

    // Black → Orange → Gold — volcanic fire, bell resonance
    t.palette.primary   = {1.0f, 0.55f, 0.0f};  // ember orange
    t.palette.secondary = {1.0f, 0.85f, 0.0f};  // gold
    t.palette.glow      = {1.0f, 0.65f, 0.1f};  // hot orange glow
    t.palette.skyTint   = {0.04f, 0.01f, 0.0f};
    t.palette.liftGrade = {0.04f, 0.01f, 0.0f};
    t.palette.gainGrade = {1.05f, 0.95f, 0.85f};

    t.vfx.bloomStyle     = BloomStyle::Infernal;
    t.vfx.bloomIntensity = 0.50f;
    t.vfx.bloomThreshold = 0.65f;
    t.vfx.bloomTint      = {1.0f, 0.6f, 0.1f};
    t.vfx.chromaticAb    = false;
    t.vfx.musicNotes     = true;  // bell chimes as note particles
    t.vfx.glyphs         = false;
    t.vfx.hasTrails      = true;
    return t;
}

Theme ThemeRegistry::makeEroica() {
    Theme t;
    t.name       = "Eroica";
    t.musicTrack = "music/eroica.ogg";

    // Scarlet → Gold — heroic, triumphant, sakura
    t.palette.primary   = {0.9f, 0.1f, 0.1f};   // scarlet
    t.palette.secondary = {1.0f, 0.85f, 0.2f};  // gold
    t.palette.glow      = {1.0f, 0.5f, 0.3f};   // warm orange-red
    t.palette.skyTint   = {0.03f, 0.0f, 0.0f};
    t.palette.liftGrade = {0.03f, 0.0f, 0.0f};
    t.palette.gainGrade = {1.05f, 0.98f, 0.90f};

    t.vfx.bloomStyle     = BloomStyle::Celestial;
    t.vfx.bloomIntensity = 0.45f;
    t.vfx.bloomThreshold = 0.68f;
    t.vfx.bloomTint      = {1.0f, 0.75f, 0.5f};
    t.vfx.chromaticAb    = false;
    t.vfx.musicNotes     = true;
    t.vfx.glyphs         = false;
    t.vfx.hasTrails      = true;
    return t;
}

Theme ThemeRegistry::makeSwanLake() {
    Theme t;
    t.name       = "SwanLake";
    t.musicTrack = "music/swan_lake.ogg";

    // White/Black with rainbow iridescence — grace, feathers, prismatic
    t.palette.primary   = {0.95f, 0.95f, 1.0f};  // near-white
    t.palette.secondary = {0.7f,  0.5f,  1.0f};  // soft violet
    t.palette.glow      = {0.9f,  0.8f,  1.0f};  // iridescent white
    t.palette.skyTint   = {0.01f, 0.01f, 0.02f};
    t.palette.liftGrade = {0.02f, 0.02f, 0.04f};
    t.palette.gainGrade = {1.0f,  1.0f,  1.02f};

    t.vfx.bloomStyle     = BloomStyle::Prismatic;
    t.vfx.bloomIntensity = 0.40f;
    t.vfx.bloomThreshold = 0.70f;
    t.vfx.bloomTint      = {0.9f, 0.85f, 1.0f};
    t.vfx.chromaticAb    = true;
    t.vfx.chromaticStr   = 0.002f;
    t.vfx.musicNotes     = false;
    t.vfx.glyphs         = false;
    t.vfx.hasTrails      = true;
    return t;
}

Theme ThemeRegistry::makeMoonlightSonata() {
    Theme t;
    t.name       = "MoonlightSonata";
    t.musicTrack = "music/moonlight_sonata.ogg";

    // Dark Purple → Light Blue — ethereal, lunar, dreamy
    t.palette.primary   = {0.35f, 0.2f,  0.8f};  // deep violet
    t.palette.secondary = {0.6f,  0.75f, 1.0f};  // light sky blue
    t.palette.glow      = {0.5f,  0.45f, 1.0f};  // soft purple-blue glow
    t.palette.skyTint   = {0.01f, 0.0f,  0.03f};
    t.palette.liftGrade = {0.02f, 0.01f, 0.05f};
    t.palette.gainGrade = {0.95f, 0.95f, 1.05f};

    t.vfx.bloomStyle     = BloomStyle::Ethereal;
    t.vfx.bloomIntensity = 0.38f;
    t.vfx.bloomThreshold = 0.72f;
    t.vfx.bloomTint      = {0.6f, 0.5f, 1.0f};
    t.vfx.chromaticAb    = false;
    t.vfx.musicNotes     = true;
    t.vfx.glyphs         = false;
    t.vfx.hasTrails      = true;
    return t;
}

Theme ThemeRegistry::makeEnigmaVariations() {
    Theme t;
    t.name       = "EnigmaVariations";
    t.musicTrack = "music/enigma.ogg";

    // Black → Purple → Green Flame — void, mystery, arcane watchers
    t.palette.primary   = {0.4f,  0.0f,  0.5f};  // deep violet
    t.palette.secondary = {0.1f,  0.8f,  0.2f};  // eerie green
    t.palette.glow      = {0.5f,  0.1f,  0.6f};  // void purple
    t.palette.skyTint   = {0.01f, 0.0f,  0.02f};
    t.palette.liftGrade = {0.02f, 0.0f,  0.03f};
    t.palette.gainGrade = {0.92f, 0.98f, 0.95f};

    t.vfx.bloomStyle     = BloomStyle::Void;
    t.vfx.bloomIntensity = 0.42f;
    t.vfx.bloomThreshold = 0.68f;
    t.vfx.bloomTint      = {0.5f, 0.1f, 0.7f};
    t.vfx.chromaticAb    = false;
    t.vfx.musicNotes     = false;
    t.vfx.glyphs         = true;  // arcane glyph particles
    t.vfx.hasTrails      = true;
    return t;
}

Theme ThemeRegistry::makeFate() {
    Theme t;
    t.name       = "Fate";
    t.musicTrack = "music/fate.ogg";

    // Black → Dark Pink → Bright Red → White — cosmic, endgame spectacle
    t.palette.primary   = {1.0f, 0.1f, 0.4f};   // hot pink-red
    t.palette.secondary = {1.0f, 1.0f, 1.0f};   // blinding white
    t.palette.glow      = {1.0f, 0.3f, 0.5f};   // crimson glow
    t.palette.skyTint   = {0.03f, 0.0f, 0.01f};
    t.palette.liftGrade = {0.03f, 0.0f, 0.01f};
    t.palette.gainGrade = {1.05f, 0.95f, 1.0f};

    t.vfx.bloomStyle     = BloomStyle::Chromatic;
    t.vfx.bloomIntensity = 0.60f;
    t.vfx.bloomThreshold = 0.60f;
    t.vfx.bloomTint      = {1.0f, 0.4f, 0.6f};
    t.vfx.chromaticAb    = true;
    t.vfx.chromaticStr   = 0.0045f;
    t.vfx.musicNotes     = true;
    t.vfx.glyphs         = true;
    t.vfx.hasTrails      = true;
    return t;
}

Theme ThemeRegistry::makeSpring() {
    Theme t;
    t.name       = "Spring";
    t.musicTrack = "music/spring.ogg";

    // Soft green → pink — floral renewal, fresh life
    t.palette.primary   = {0.4f,  0.85f, 0.3f};  // fresh green
    t.palette.secondary = {1.0f,  0.7f,  0.8f};  // blossom pink
    t.palette.glow      = {0.7f,  1.0f,  0.5f};  // verdant glow
    t.palette.skyTint   = {0.01f, 0.02f, 0.0f};
    t.palette.liftGrade = {0.01f, 0.02f, 0.0f};
    t.palette.gainGrade = {0.98f, 1.02f, 0.98f};

    t.vfx.bloomStyle     = BloomStyle::Radiant;
    t.vfx.bloomIntensity = 0.30f;
    t.vfx.bloomThreshold = 0.75f;
    t.vfx.bloomTint      = {0.7f, 1.0f, 0.6f};
    return t;
}

Theme ThemeRegistry::makeSummer() {
    Theme t;
    t.name       = "Summer";
    t.musicTrack = "music/summer.ogg";

    // Vivid yellow → orange — vibrant heat, sun-drenched
    t.palette.primary   = {1.0f,  0.9f,  0.1f};  // vivid yellow
    t.palette.secondary = {1.0f,  0.5f,  0.0f};  // sunset orange
    t.palette.glow      = {1.0f,  0.85f, 0.2f};  // warm radiance
    t.palette.skyTint   = {0.04f, 0.03f, 0.0f};
    t.palette.liftGrade = {0.03f, 0.02f, 0.0f};
    t.palette.gainGrade = {1.05f, 1.02f, 0.92f};

    t.vfx.bloomStyle     = BloomStyle::Infernal;
    t.vfx.bloomIntensity = 0.40f;
    t.vfx.bloomThreshold = 0.68f;
    t.vfx.bloomTint      = {1.0f, 0.9f, 0.3f};
    return t;
}

Theme ThemeRegistry::makeAutumn() {
    Theme t;
    t.name       = "Autumn";
    t.musicTrack = "music/autumn.ogg";

    // Orange → deep red — nostalgic warmth, falling leaves
    t.palette.primary   = {0.85f, 0.4f,  0.05f}; // burnt orange
    t.palette.secondary = {0.6f,  0.1f,  0.05f}; // deep red
    t.palette.glow      = {0.9f,  0.55f, 0.1f};  // amber
    t.palette.skyTint   = {0.03f, 0.01f, 0.0f};
    t.palette.liftGrade = {0.03f, 0.01f, 0.0f};
    t.palette.gainGrade = {1.04f, 0.97f, 0.90f};

    t.vfx.bloomStyle     = BloomStyle::Infernal;
    t.vfx.bloomIntensity = 0.32f;
    t.vfx.bloomThreshold = 0.72f;
    t.vfx.bloomTint      = {0.9f, 0.5f, 0.15f};
    return t;
}

Theme ThemeRegistry::makeWinter() {
    Theme t;
    t.name       = "Winter";
    t.musicTrack = "music/winter.ogg";

    // White → ice blue — crystalline serenity
    t.palette.primary   = {0.85f, 0.92f, 1.0f};  // ice white
    t.palette.secondary = {0.5f,  0.75f, 1.0f};  // glacial blue
    t.palette.glow      = {0.7f,  0.85f, 1.0f};  // cold glow
    t.palette.skyTint   = {0.01f, 0.01f, 0.03f};
    t.palette.liftGrade = {0.01f, 0.01f, 0.03f};
    t.palette.gainGrade = {0.97f, 0.98f, 1.04f};

    t.vfx.bloomStyle     = BloomStyle::Ethereal;
    t.vfx.bloomIntensity = 0.35f;
    t.vfx.bloomThreshold = 0.74f;
    t.vfx.bloomTint      = {0.7f, 0.85f, 1.0f};
    t.vfx.chromaticAb    = false;
    return t;
}
