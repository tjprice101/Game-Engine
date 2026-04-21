#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include "renderer/Shader.h"
#include "renderer/Texture.h"
#include "renderer/Camera.h"

// ---- SpriteBatch ------------------------------------------------------------
// High-performance sprite batcher.
// - Supports up to 8 simultaneous texture slots (one flush per texture change)
// - Per-sprite: position, size, UV rect, tint, normal-map UVs, layer depth
// - Draws back-to-front within a layer group

static constexpr int SPRITE_MAX = 8192;
static constexpr int TEX_SLOTS  = 8;

struct SpriteVertex {
    glm::vec2 pos;
    glm::vec2 uv;
    glm::vec2 uvNorm;   // normal-map UV (same atlas, second section)
    glm::vec4 color;
    float     texIdx;   // which texture slot (0..7)
    float     emissive; // emissive multiplier (0=none, 1=full bright)
};

class SpriteBatch {
public:
    SpriteBatch();
    ~SpriteBatch();

    SpriteBatch(const SpriteBatch&)            = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;

    // ---- Frame lifecycle ----------------------------------------------------
    void begin(const Camera& cam, const Shader& shader);

    // Submit one sprite. normalTex may be null (no normal mapping).
    void submit(const Texture* tex,
                const Texture* normalTex,
                glm::vec2 pos,
                glm::vec2 size,
                glm::vec2 uvMin        = {0.f, 0.f},
                glm::vec2 uvMax        = {1.f, 1.f},
                glm::vec4 tint         = {1.f, 1.f, 1.f, 1.f},
                float     emissive     = 0.f,
                bool      flipX        = false);

    void end();   // flush all remaining sprites

    // ---- Stats --------------------------------------------------------------
    int drawCalls()  const { return m_drawCalls; }
    int spritesDrawn()const { return m_spritesDrawn; }
    void resetStats()       { m_drawCalls = 0; m_spritesDrawn = 0; }

private:
    void flush();
    int  getOrBindTexSlot(const Texture* tex);

    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;

    const Shader*  m_shader = nullptr;
    const Camera*  m_camera = nullptr;

    std::vector<SpriteVertex> m_verts;
    int m_spriteCount = 0;

    // Multi-texture slot tracking
    const Texture* m_texSlots[TEX_SLOTS]     = {};
    const Texture* m_normSlots[TEX_SLOTS]    = {};
    int            m_texSlotCount = 0;

    int m_drawCalls   = 0;
    int m_spritesDrawn= 0;
};
