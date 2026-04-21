#include "UISystem.h"
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cmath>

// ---- GPU init ---------------------------------------------------------------
void UISystem::initGPU() {
    if (m_gpuInit) return;
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    // UIVertex layout: pos(2), uv(2), color(4), texSlot(1 as float)
    GLsizei stride = sizeof(UIVertex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(UIVertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(UIVertex, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(UIVertex, color));
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT,   stride, (void*)offsetof(UIVertex, texSlot));

    glBindVertexArray(0);
    m_gpuInit = true;
}

// ---- Frame ------------------------------------------------------------------
void UISystem::beginFrame(int screenW, int screenH) {
    m_screenW = screenW;
    m_screenH = screenH;
    m_verts.clear();
    m_indices.clear();
    m_slotCount = 0;
    std::fill(std::begin(m_texSlots), std::end(m_texSlots), 0u);
    m_tooltip.visible = false;
    m_inFrame = true;
}

void UISystem::endFrame(uint32_t uiShaderID) {
    if (!m_inFrame) return;
    m_inFrame = false;

    // Draw pending tooltip on top of everything else
    if (m_tooltip.visible) drawTooltip(m_tooltip);

    // Draw floating damage numbers
    drawDamageNumbers();

    if (m_verts.empty()) return;
    initGPU();

    // Upload geometry
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(m_verts.size() * sizeof(UIVertex)),
                 m_verts.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(m_indices.size() * sizeof(uint32_t)),
                 m_indices.data(), GL_STREAM_DRAW);

    // Bind shader + ortho matrix
    glUseProgram(uiShaderID);

    // Orthographic projection: screen pixels (0,0)=TL, (W,H)=BR
    float proj[16] = {
        2.f/m_screenW, 0,  0, 0,
        0, -2.f/m_screenH, 0, 0,
        0, 0, -1, 0,
        -1, 1,  0, 1
    };
    GLint projLoc = glGetUniformLocation(uiShaderID, "uProj");
    if (projLoc >= 0) glUniformMatrix4fv(projLoc, 1, GL_FALSE, proj);

    // Bind textures
    int samplers[8] = {0,1,2,3,4,5,6,7};
    GLint texLoc = glGetUniformLocation(uiShaderID, "uTextures");
    if (texLoc >= 0) glUniform1iv(texLoc, 8, samplers);

    for (int i = 0; i < m_slotCount; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_texSlots[i]);
    }

    // Draw
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)m_indices.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

// ---- Quad helpers -----------------------------------------------------------
int UISystem::bindTex(uint32_t texID) {
    if (texID == 0) return 0; // slot 0 = no texture
    for (int i = 0; i < m_slotCount; ++i)
        if (m_texSlots[i] == texID) return i + 1;
    if (m_slotCount < 7) {
        m_texSlots[m_slotCount] = texID;
        return ++m_slotCount;
    }
    return 1; // fallback: overwrite slot 1
}

void UISystem::pushQuad(glm::vec2 tl, glm::vec2 br, glm::vec4 color,
                         uint32_t texID, glm::vec2 uvMin, glm::vec2 uvMax) {
    uint32_t slot = (uint32_t)bindTex(texID);
    uint32_t base = (uint32_t)m_verts.size();

    m_verts.push_back({{tl.x, tl.y}, {uvMin.x, uvMin.y}, color, slot});
    m_verts.push_back({{br.x, tl.y}, {uvMax.x, uvMin.y}, color, slot});
    m_verts.push_back({{br.x, br.y}, {uvMax.x, uvMax.y}, color, slot});
    m_verts.push_back({{tl.x, br.y}, {uvMin.x, uvMax.y}, color, slot});

    m_indices.insert(m_indices.end(), {base, base+1, base+2, base, base+2, base+3});
}

void UISystem::drawRect(glm::vec2 pos, glm::vec2 size, glm::vec4 color,
                         uint32_t texID, glm::vec2 uvMin, glm::vec2 uvMax) {
    pushQuad(pos, pos + size, color, texID, uvMin, uvMax);
}

void UISystem::drawBar(glm::vec2 pos, glm::vec2 size, float fraction,
                        glm::vec4 fillColor, glm::vec4 bgColor) {
    fraction = std::clamp(fraction, 0.f, 1.f);
    // Background
    pushQuad(pos, pos + size, bgColor, 0, {0,0}, {1,1});
    // Fill
    glm::vec2 fillSize = {size.x * fraction, size.y};
    if (fillSize.x > 0)
        pushQuad(pos + glm::vec2{1,1}, pos + fillSize - glm::vec2{1,1}, fillColor, 0, {0,0}, {1,1});
}

// ---- Hotbar -----------------------------------------------------------------
static constexpr int HOTBAR_SLOTS  = 10;
static constexpr float SLOT_SIZE   = 40.f;
static constexpr float SLOT_GAP    = 4.f;
static constexpr float SLOT_BORDER = 2.f;

void UISystem::drawHotbar(const Inventory& inv, int screenW, int screenH,
                           uint32_t atlasTexID) {
    float totalW = HOTBAR_SLOTS * (SLOT_SIZE + SLOT_GAP) - SLOT_GAP;
    float startX = (screenW - totalW) * 0.5f;
    float startY = screenH - SLOT_SIZE - 14.f;

    int selected = inv.hotbarSelected();

    for (int i = 0; i < HOTBAR_SLOTS; ++i) {
        float x = startX + i * (SLOT_SIZE + SLOT_GAP);
        bool  isSelected = (i == selected);

        // Slot background
        glm::vec4 bgColor = isSelected
            ? glm::vec4{0.8f, 0.8f, 0.2f, 0.9f}
            : glm::vec4{0.15f, 0.15f, 0.15f, 0.8f};

        drawRect({x, startY}, {SLOT_SIZE, SLOT_SIZE}, bgColor);

        // Slot border
        glm::vec4 borderCol = isSelected
            ? glm::vec4{1.f, 1.f, 0.3f, 1.f}
            : glm::vec4{0.4f, 0.4f, 0.4f, 1.f};
        // Top / bottom / left / right borders as thin rects
        drawRect({x, startY}, {SLOT_SIZE, SLOT_BORDER}, borderCol);
        drawRect({x, startY + SLOT_SIZE - SLOT_BORDER}, {SLOT_SIZE, SLOT_BORDER}, borderCol);
        drawRect({x, startY}, {SLOT_BORDER, SLOT_SIZE}, borderCol);
        drawRect({x + SLOT_SIZE - SLOT_BORDER, startY}, {SLOT_BORDER, SLOT_SIZE}, borderCol);

        // Item icon
        const ItemStack& stack = inv.slot(i);
        if (stack.type != ItemType::None && atlasTexID != 0) {
            glm::vec2 uvMin = itemUVMin((int)stack.type);
            glm::vec2 uvMax = itemUVMax((int)stack.type);
            float pad = SLOT_SIZE * 0.1f;
            drawRect({x + pad, startY + pad},
                     {SLOT_SIZE - pad*2, SLOT_SIZE - pad*2},
                     {1,1,1,1}, atlasTexID, uvMin, uvMax);

            // Stack count (only show if > 1)
            if (stack.count > 1) {
                char buf[8];
                std::snprintf(buf, sizeof(buf), "%d", stack.count);
                drawText(buf, {x + SLOT_SIZE - 14.f, startY + SLOT_SIZE - 14.f},
                         10.f, {1.f, 1.f, 0.8f, 1.f});
            }
        }
    }
}

// ---- Health/Mana bars -------------------------------------------------------
void UISystem::drawHealthMana(float hpFrac, float manaFrac, int screenW, int screenH) {
    float barW = 200.f, barH = 14.f;
    float totalW = HOTBAR_SLOTS * (SLOT_SIZE + SLOT_GAP) - SLOT_GAP;
    float startX = (screenW - totalW) * 0.5f;
    float barY   = screenH - SLOT_SIZE - 14.f - barH - 6.f;

    // HP: left side
    drawBar({startX, barY}, {barW, barH}, hpFrac,
            {0.9f, 0.15f, 0.15f, 1.f}, {0.1f, 0.1f, 0.1f, 0.8f});
    drawText("HP", {startX + 4.f, barY + 2.f}, 9.f, {1,1,1,0.8f});

    // Mana: right side
    float manaX = startX + totalW - barW;
    drawBar({manaX, barY}, {barW, barH}, manaFrac,
            {0.15f, 0.4f, 0.95f, 1.f}, {0.1f, 0.1f, 0.1f, 0.8f});
    drawText("MP", {manaX + 4.f, barY + 2.f}, 9.f, {1,1,1,0.8f});
}

// ---- Stamina ----------------------------------------------------------------
void UISystem::drawStamina(float frac, int screenW, int screenH) {
    if (frac < 0.f) return;
    float barW = 120.f, barH = 8.f;
    float totalW = HOTBAR_SLOTS * (SLOT_SIZE + SLOT_GAP) - SLOT_GAP;
    float x = (screenW - totalW) * 0.5f + totalW * 0.5f - barW * 0.5f;
    float y = screenH - SLOT_SIZE - 14.f - 8.f - 6.f - 16.f;
    drawBar({x, y}, {barW, barH}, frac,
            {0.2f, 0.85f, 0.3f, 1.f}, {0.1f, 0.1f, 0.1f, 0.8f});
}

// ---- Tooltip ----------------------------------------------------------------
void UISystem::setTooltip(const std::string& title, const std::string& body, glm::vec2 pos) {
    m_tooltip = {title, body, pos, true};
}
void UISystem::clearTooltip() { m_tooltip.visible = false; }

void UISystem::drawTooltip(const Tooltip& tip) {
    if (!tip.visible) return;
    float w = 180.f, h = tip.body.empty() ? 28.f : 50.f;
    glm::vec2 p = tip.screenPos + glm::vec2{10.f, -h - 10.f};
    p.x = std::clamp(p.x, 2.f, (float)m_screenW - w - 2.f);
    p.y = std::clamp(p.y, 2.f, (float)m_screenH - h - 2.f);

    // Shadow
    drawRect(p + glm::vec2{2,2}, {w,h}, {0,0,0,0.5f});
    // Background
    drawRect(p, {w,h}, {0.08f, 0.08f, 0.12f, 0.92f});
    // Border
    drawRect(p, {w, 2.f}, {0.5f, 0.5f, 0.9f, 1.f});
    // Title
    drawText(tip.title, p + glm::vec2{6.f, 6.f}, 11.f, {1.f, 1.f, 0.4f, 1.f});
    if (!tip.body.empty())
        drawText(tip.body,  p + glm::vec2{6.f, 22.f}, 9.f, {0.85f, 0.85f, 0.85f, 1.f});
}

// ---- Inventory grid ---------------------------------------------------------
void UISystem::drawInventoryGrid(const Inventory& inv, int screenW, int screenH,
                                  uint32_t atlasTexID) {
    static constexpr int COLS = 9, ROWS = 5;
    float gridW = COLS * (SLOT_SIZE + SLOT_GAP) - SLOT_GAP;
    float gridH = ROWS * (SLOT_SIZE + SLOT_GAP) - SLOT_GAP;
    float ox = (screenW - gridW) * 0.5f;
    float oy = (screenH - gridH) * 0.5f;

    // Panel background
    drawRect({ox - 12.f, oy - 12.f}, {gridW + 24.f, gridH + 24.f},
             {0.1f, 0.1f, 0.14f, 0.95f});

    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            int idx = r * COLS + c;
            float x = ox + c * (SLOT_SIZE + SLOT_GAP);
            float y = oy + r * (SLOT_SIZE + SLOT_GAP);
            drawRect({x, y}, {SLOT_SIZE, SLOT_SIZE}, {0.2f, 0.2f, 0.2f, 0.8f});

            if (idx < Inventory::TOTAL_SLOTS) {
                const ItemStack& stack = inv.slot(idx);
                if (stack.type != ItemType::None && atlasTexID) {
                    float pad = SLOT_SIZE * 0.1f;
                    drawRect({x+pad, y+pad}, {SLOT_SIZE-pad*2, SLOT_SIZE-pad*2},
                             {1,1,1,1}, atlasTexID,
                             itemUVMin((int)stack.type),
                             itemUVMax((int)stack.type));
                    if (stack.count > 1) {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "%d", stack.count);
                        drawText(buf, {x + SLOT_SIZE - 14.f, y + SLOT_SIZE - 14.f},
                                 10.f, {1.f,1.f,0.8f,1.f});
                    }
                }
            }
        }
    }
}

// ---- Damage numbers ---------------------------------------------------------
void UISystem::spawnDamageNumber(glm::vec2 worldPos, float damage, bool isCrit,
                                  glm::vec2 worldToScreen) {
    FloatNumber fn;
    char buf[16];
    if (isCrit)
        std::snprintf(buf, sizeof(buf), "%.0f!", damage);
    else
        std::snprintf(buf, sizeof(buf), "%.0f", damage);
    fn.text  = buf;
    fn.pos   = worldPos + worldToScreen;  // caller maps world→screen offset
    fn.color = isCrit ? glm::vec4{1.f,0.85f,0.f,1.f} : glm::vec4{1.f,0.3f,0.3f,1.f};
    fn.life  = 1.2f;
    fn.vy    = isCrit ? -90.f : -60.f;
    m_floatNumbers.push_back(fn);
}

void UISystem::updateDamageNumbers(float dt) {
    for (auto& fn : m_floatNumbers) {
        fn.pos.y += fn.vy * dt;
        fn.life  -= dt;
        fn.color.a = std::clamp(fn.life / 0.5f, 0.f, 1.f);
    }
    m_floatNumbers.erase(
        std::remove_if(m_floatNumbers.begin(), m_floatNumbers.end(),
                       [](const FloatNumber& n){ return n.life <= 0.f; }),
        m_floatNumbers.end());
}

void UISystem::drawDamageNumbers() {
    for (auto& fn : m_floatNumbers)
        drawText(fn.text, fn.pos, fn.life > 0.8f ? 14.f : 12.f, fn.color);
}

// ---- Minimap ----------------------------------------------------------------
void UISystem::drawMinimap(uint32_t minimapTexID, int screenW, int screenH) {
    float sz = 120.f;
    glm::vec2 pos = {(float)screenW - sz - 10.f, 10.f};
    drawRect(pos - glm::vec2{2,2}, {sz+4.f, sz+4.f}, {0,0,0,0.6f});
    if (minimapTexID)
        drawRect(pos, {sz, sz}, {1,1,1,1}, minimapTexID, {0,0}, {1,1});
    else
        drawRect(pos, {sz, sz}, {0.1f, 0.1f, 0.15f, 0.7f});
}

// ---- Crosshair --------------------------------------------------------------
void UISystem::drawCrosshair(int screenW, int screenH) {
    float cx = screenW * 0.5f, cy = screenH * 0.5f;
    float len = 8.f, thick = 1.5f;
    glm::vec4 col = {1,1,1,0.7f};
    drawRect({cx - len, cy - thick*0.5f}, {len - 4.f, thick}, col);
    drawRect({cx + 4.f, cy - thick*0.5f}, {len - 4.f, thick}, col);
    drawRect({cx - thick*0.5f, cy - len}, {thick, len - 4.f}, col);
    drawRect({cx - thick*0.5f, cy + 4.f}, {thick, len - 4.f}, col);
}

// ---- Text (simple stub — real SDF font needs glyph atlas) -------------------
void UISystem::drawText(const std::string& text, glm::vec2 pos, float size,
                         glm::vec4 color) {
    // Without a font atlas we render a placeholder solid rect per character.
    // Real implementation: look up each glyph in an SDF atlas and emit a textured quad.
    float x = pos.x;
    float charW = size * 0.6f;
    for (char c : text) {
        if (c == ' ') { x += charW; continue; }
        drawRect({x, pos.y}, {charW, size}, color);
        x += charW + 1.f;
    }
}

// ---- Click button -----------------------------------------------------------
void UISystem::drawClickButton(glm::vec2 centre, glm::vec2 size,
                                const std::string& label, float pressAnim,
                                glm::vec4 color) {
    float press = std::clamp(pressAnim, 0.f, 1.f);
    glm::vec2 scaledSize = size * (1.f - press * 0.05f);
    glm::vec2 tl = centre - scaledSize * 0.5f;

    // Shadow
    drawRect(tl + glm::vec2{4.f*(1.f-press), 4.f*(1.f-press)},
             scaledSize, {0,0,0,0.4f});

    // Button face
    glm::vec4 col = color * (1.f - press * 0.3f);
    col.a = color.a;
    drawRect(tl, scaledSize, col);

    // Highlight top edge
    drawRect(tl, {scaledSize.x, 3.f}, color + glm::vec4{0.3f,0.3f,0.3f,0.f});

    // Label centred
    float textW = label.size() * scaledSize.x * 0.06f;
    drawText(label, {centre.x - textW*0.5f, centre.y - 7.f}, 14.f, {1,1,1,1});
}

// ---- Resource counter -------------------------------------------------------
void UISystem::drawResourceCounter(glm::vec2 pos, uint32_t iconTex, float value,
                                    const std::string& suffix) {
    // Icon
    if (iconTex) drawRect(pos, {20.f, 20.f}, {1,1,1,1}, iconTex, {0,0}, {1,1});
    // Value text
    char buf[32];
    if (value >= 1e6f)
        std::snprintf(buf, sizeof(buf), "%.2fM%s", value/1e6f, suffix.c_str());
    else if (value >= 1e3f)
        std::snprintf(buf, sizeof(buf), "%.1fK%s", value/1e3f, suffix.c_str());
    else
        std::snprintf(buf, sizeof(buf), "%.0f%s", value, suffix.c_str());
    drawText(buf, {pos.x + 24.f, pos.y + 4.f}, 12.f, {1.f, 0.95f, 0.3f, 1.f});
}

// ---- Debug stats ------------------------------------------------------------
void UISystem::drawDebugStats(float fps, int entities, int drawCalls, int screenW) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "FPS:%.0f  ENT:%d  DC:%d", fps, entities, drawCalls);
    drawRect({2.f, 2.f}, {(float)strlen(buf)*7.f + 6.f, 16.f}, {0,0,0,0.5f});
    drawText(buf, {4.f, 4.f}, 10.f, {0.4f, 1.f, 0.4f, 1.f});
}

// ---- Item UV helpers (assumes items atlas is a 16x16 grid of 16px cells) ----
glm::vec2 UISystem::itemUVMin(int itemType) const {
    // Each item occupies one cell in a 16-wide atlas
    int col = itemType % 16;
    int row = itemType / 16;
    return { col / 16.f, row / 16.f };
}
glm::vec2 UISystem::itemUVMax(int itemType) const {
    return itemUVMin(itemType) + glm::vec2{1.f/16.f, 1.f/16.f};
}
