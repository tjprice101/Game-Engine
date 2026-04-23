#pragma once
#include <glm/glm.hpp>
#include <string>

// ---- BloomStyle -------------------------------------------------------------
// Controls the hue and energy character of the bloom layer.
// Inspired by MagnumOpus's per-theme visual identities.
enum class BloomStyle {
    Default,    // Neutral white bloom
    Infernal,   // Orange/red — volcanic heat and fire
    Celestial,  // Gold/white — divine radiance
    Chromatic,  // RGB channel separation — reality fracture
    Void,       // Dark bloom with purple rim — abyssal energy
    Ethereal,   // Soft blue/white — lunar dreaminess
    Prismatic,  // Full HSL cycle — iridescent rainbow
    Radiant,    // Pure brilliant white — blinding light
};

// ---- ScreenDistortion -------------------------------------------------------
// Types of full-screen distortion effects used during dramatic moments.
enum class ScreenDistortion {
    None,
    Ripple,     // Concentric wave from impact point
    Shatter,    // Radial cracks warp
    Warp,       // Smooth barrel/pinch warp
    Pulse,      // Uniform radial push-out
    Tear,       // Horizontal glitch tear
};

// ---- ThemePalette -----------------------------------------------------------
// The full color identity of a musical composition theme.
struct ThemePalette {
    glm::vec3 primary;      // Core color  (particle base, weapon tint)
    glm::vec3 secondary;    // Accent      (gradient end, trail tip)
    glm::vec3 glow;         // Bloom/light emission color
    glm::vec3 skyTint;      // Additive sky/background color shift (small values)
    glm::vec3 liftGrade;    // Color-grade lift (shadow color tint)
    glm::vec3 gainGrade;    // Color-grade gain (highlight color tint)
};

// ---- ThemeVFXConfig ---------------------------------------------------------
// Controls which visual effects are active and how intense they are.
struct ThemeVFXConfig {
    BloomStyle  bloomStyle       = BloomStyle::Default;
    float       bloomIntensity   = 0.35f;
    float       bloomThreshold   = 0.70f;
    glm::vec3   bloomTint        = {1.f, 1.f, 1.f}; // tint applied to bloom layer
    bool        chromaticAb      = false;
    float       chromaticStr     = 0.003f;
    bool        musicNotes       = false;  // spawn music-note particles on impacts
    bool        glyphs           = false;  // spawn arcane glyph particles on crits
    bool        hasTrails        = true;   // entities/projectiles inherit theme trails
};

// ---- Theme ------------------------------------------------------------------
// A complete musical composition theme, driving all visual systems.
// Inspired by MagnumOpus's theme-first design philosophy.
struct Theme {
    std::string    name;
    ThemePalette   palette;
    ThemeVFXConfig vfx;
    std::string    musicTrack;  // path hint (game layer plays the actual audio)
};
