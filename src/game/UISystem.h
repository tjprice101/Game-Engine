#pragma once
#include "ecs/ECS.h"
#include "ecs/Components.h"
#include "game/Inventory.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <functional>

// ---- UISystem ----------------------------------------------------------------
// Immediate-mode HUD rendered in screen space (no world-to-screen transform).
// All coordinates are in screen pixels; (0,0) = top-left.
// Call beginFrame() → individual draw*() calls → endFrame() each frame.
// Actual GL drawing is deferred to a separate ui.vert/ui.frag shader pass.

// ---- UI Vertex (written into a single VBO, batched draw) --------------------
struct UIVertex {
    glm::vec2 pos;      // screen pixels
    glm::vec2 uv;       // texture UV
    glm::vec4 color;
    uint32_t  texSlot;  // 0 = colour-only, 1+ = texture
};

// ---- Tooltip data -----------------------------------------------------------
struct Tooltip {
    std::string title;
    std::string body;
    glm::vec2   screenPos = {0.f, 0.f};
    bool        visible   = false;
};

// ---- Floating damage number state -------------------------------------------
struct FloatNumber {
    std::string text;
    glm::vec2   pos;    // screen pixels
    glm::vec4   color;
    float       life;   // seconds remaining
    float       vy;     // pixels/sec upward (negative Y)
};

// ---- UISystem ----------------------------------------------------------------
class UISystem {
public:
    // Must be called before any draw calls; sets screen size
    void beginFrame(int screenW, int screenH);

    // Submit all vertices to GPU and draw
    void endFrame(uint32_t uiShaderID);

    // ---- Element draws -------------------------------------------------------

    // Hotbar: 10 item slots, centered horizontally near bottom
    void drawHotbar(const Inventory& inv, int screenW, int screenH,
                    uint32_t atlasTexID);

    // Health / mana bars (above hotbar)
    void drawHealthMana(float hpFrac, float manaFrac, int screenW, int screenH);

    // Stamina/hunger if present (optional — pass -1 to skip)
    void drawStamina(float frac, int screenW, int screenH);

    // Full inventory grid (9×5 grid): call when inventory is open
    void drawInventoryGrid(const Inventory& inv, int screenW, int screenH,
                           uint32_t atlasTexID);

    // Tooltip box rendered at screenPos
    void drawTooltip(const Tooltip& tip);

    // Floating damage numbers (called by game when an entity takes damage)
    void spawnDamageNumber(glm::vec2 worldPos, float damage, bool isCrit,
                           glm::vec2 worldToScreen); // helper transform
    void updateDamageNumbers(float dt);
    void drawDamageNumbers();

    // Mini-map (drawn in top-right corner)
    void drawMinimap(uint32_t minimapTexID, int screenW, int screenH);

    // Crosshair (thin line reticle)
    void drawCrosshair(int screenW, int screenH);

    // Generic filled rect (used by all other elements)
    void drawRect(glm::vec2 pos, glm::vec2 size, glm::vec4 color,
                  uint32_t texID = 0, glm::vec2 uvMin = {0,0}, glm::vec2 uvMax = {1,1});

    // Text via SDF atlas (glyphs packed as UIVertex quads)
    void drawText(const std::string& text, glm::vec2 pos, float size,
                  glm::vec4 color = {1,1,1,1});

    // Progress bar helper
    void drawBar(glm::vec2 pos, glm::vec2 size,
                 float fraction, glm::vec4 fillColor, glm::vec4 bgColor);

    // ---- Tooltip set (called by game hover logic) ----------------------------
    void setTooltip(const std::string& title, const std::string& body, glm::vec2 pos);
    void clearTooltip();

    // ---- Clicker / tycoon elements -------------------------------------------
    // Big click button with animated press feedback
    void drawClickButton(glm::vec2 centre, glm::vec2 size,
                         const std::string& label, float pressAnim,
                         glm::vec4 color = {0.2f,0.6f,1.f,1.f});

    // Resource counter row (coin icon + value)
    void drawResourceCounter(glm::vec2 pos, uint32_t iconTex, float value,
                              const std::string& suffix = "");

    // ---- Frame stats (debug) ------------------------------------------------
    void drawDebugStats(float fps, int entities, int drawCalls, int screenW);

private:
    int   m_screenW = 0, m_screenH = 0;
    bool  m_inFrame = false;

    std::vector<UIVertex>  m_verts;
    std::vector<uint32_t>  m_indices;
    std::vector<FloatNumber> m_floatNumbers;
    Tooltip                m_tooltip;

    uint32_t m_vao = 0, m_vbo = 0, m_ebo = 0;
    bool     m_gpuInit = false;

    // Texture slots bound this frame (UI typically only needs atlas + SDF font)
    uint32_t m_texSlots[8] = {};
    int      m_slotCount   = 0;

    // Internal: ensure GL buffers exist
    void initGPU();
    // Push a quad (2 triangles) to m_verts/m_indices
    void pushQuad(glm::vec2 tl, glm::vec2 br, glm::vec4 color,
                  uint32_t texID, glm::vec2 uvMin, glm::vec2 uvMax);
    // Resolve a texture to slot index (0-7), returns slot
    int  bindTex(uint32_t texID);

    // Slot constants for item atlas tile lookups (6 items per hotbar row shown)
    glm::vec2 itemUVMin(int itemType) const;
    glm::vec2 itemUVMax(int itemType) const;
};
