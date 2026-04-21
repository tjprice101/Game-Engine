#include "SpriteBatch.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>

SpriteBatch::SpriteBatch() {
    // Pre-build index buffer
    std::vector<uint32_t> indices;
    indices.reserve(SPRITE_MAX * 6);
    for (int i = 0; i < SPRITE_MAX; ++i) {
        uint32_t b = i * 4;
        indices.insert(indices.end(), {b,b+1,b+2, b,b+2,b+3});
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, SPRITE_MAX * 4 * sizeof(SpriteVertex), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Layout:
    using V = SpriteVertex;
    auto attr = [](GLuint idx, GLint n, GLenum t, size_t off){
        glEnableVertexAttribArray(idx);
        glVertexAttribPointer(idx, n, t, GL_FALSE, sizeof(V), (void*)off);
    };
    attr(0, 2, GL_FLOAT, offsetof(V, pos));
    attr(1, 2, GL_FLOAT, offsetof(V, uv));
    attr(2, 2, GL_FLOAT, offsetof(V, uvNorm));
    attr(3, 4, GL_FLOAT, offsetof(V, color));
    attr(4, 1, GL_FLOAT, offsetof(V, texIdx));
    attr(5, 1, GL_FLOAT, offsetof(V, emissive));

    glBindVertexArray(0);
}

SpriteBatch::~SpriteBatch() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
}

void SpriteBatch::begin(const Camera& cam, const Shader& shader) {
    m_camera      = &cam;
    m_shader      = &shader;
    m_spriteCount = 0;
    m_texSlotCount= 0;
    m_verts.clear();
    std::fill(std::begin(m_texSlots),  std::end(m_texSlots),  nullptr);
    std::fill(std::begin(m_normSlots), std::end(m_normSlots), nullptr);
}

int SpriteBatch::getOrBindTexSlot(const Texture* tex) {
    if (!tex) return -1;
    for (int i = 0; i < m_texSlotCount; ++i)
        if (m_texSlots[i] == tex) return i;
    if (m_texSlotCount >= TEX_SLOTS) flush();
    int slot = m_texSlotCount++;
    m_texSlots[slot] = tex;
    return slot;
}

void SpriteBatch::submit(const Texture* tex,
                          const Texture* normalTex,
                          glm::vec2 pos,
                          glm::vec2 size,
                          glm::vec2 uvMin,
                          glm::vec2 uvMax,
                          glm::vec4 tint,
                          float emissive,
                          bool flipX)
{
    if (m_spriteCount >= SPRITE_MAX) flush();

    float slot = (float)getOrBindTexSlot(tex);
    if (flipX) std::swap(uvMin.x, uvMax.x);

    // Normal map UV: same coords, but sampled from the normal atlas (slot+8 typically)
    glm::vec2 nMin = uvMin, nMax = uvMax;

    // 4 vertices: TL, TR, BR, BL
    m_verts.push_back({ pos,                        {uvMin.x, uvMax.y}, {nMin.x, nMax.y}, tint, slot, emissive });
    m_verts.push_back({ pos + glm::vec2(size.x,0),  {uvMax.x, uvMax.y}, {nMax.x, nMax.y}, tint, slot, emissive });
    m_verts.push_back({ pos + size,                  {uvMax.x, uvMin.y}, {nMax.x, nMin.y}, tint, slot, emissive });
    m_verts.push_back({ pos + glm::vec2(0, size.y),  {uvMin.x, uvMin.y}, {nMin.x, nMin.y}, tint, slot, emissive });

    ++m_spriteCount;
}

void SpriteBatch::end() { flush(); }

void SpriteBatch::flush() {
    if (m_verts.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    m_verts.size() * sizeof(SpriteVertex), m_verts.data());

    m_shader->bind();
    m_shader->setMat4("uViewProj", m_camera->viewProj());

    // Bind textures
    int samplers[TEX_SLOTS];
    for (int i = 0; i < TEX_SLOTS; ++i) {
        samplers[i] = i;
        if (i < m_texSlotCount && m_texSlots[i]) {
            m_texSlots[i]->bind(i);
        } else {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    glUniform1iv(glGetUniformLocation(m_shader->id(), "uTextures"), TEX_SLOTS, samplers);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_spriteCount * 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    ++m_drawCalls;
    m_spritesDrawn += m_spriteCount;

    m_spriteCount  = 0;
    m_texSlotCount = 0;
    m_verts.clear();
    std::fill(std::begin(m_texSlots), std::end(m_texSlots), nullptr);
}
