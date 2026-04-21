#include "EntityRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

EntityRenderer::EntityRenderer() {
    m_verts.reserve(MAX_SPRITES * 4);
    m_indices.reserve(MAX_SPRITES * 6);

    // Pre-build index buffer pattern
    std::vector<uint32_t> indices;
    indices.reserve(MAX_SPRITES * 6);
    for (int i = 0; i < MAX_SPRITES; ++i) {
        uint32_t base = i * 4;
        indices.insert(indices.end(), {base,base+1,base+2, base,base+2,base+3});
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_SPRITES * 4 * sizeof(SpriteVertex), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Attrib 0: position (vec2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)offsetof(SpriteVertex, pos));
    // Attrib 1: uv (vec2)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)offsetof(SpriteVertex, uv));
    // Attrib 2: color (vec4)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), (void*)offsetof(SpriteVertex, color));

    glBindVertexArray(0);
}

EntityRenderer::~EntityRenderer() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
}

void EntityRenderer::begin(const Camera& camera, const Shader& shader) {
    m_camera = &camera;
    m_shader = &shader;
    m_verts.clear();
    m_indices.clear();
    m_spriteCount = 0;
    m_curTex = nullptr;
}

void EntityRenderer::submit(const Texture* texture,
                             glm::vec2 worldPos, glm::vec2 size,
                             glm::vec2 uvMin, glm::vec2 uvMax,
                             glm::vec4 tint, bool flipX)
{
    if (m_spriteCount >= MAX_SPRITES || (m_curTex && m_curTex != texture))
        flush();

    m_curTex = texture;

    if (flipX) std::swap(uvMin.x, uvMax.x);

    // 4 verts: top-left, top-right, bottom-right, bottom-left
    m_verts.push_back({ worldPos,                        {uvMin.x, uvMax.y}, tint });
    m_verts.push_back({ worldPos + glm::vec2(size.x,0),  {uvMax.x, uvMax.y}, tint });
    m_verts.push_back({ worldPos + size,                  {uvMax.x, uvMin.y}, tint });
    m_verts.push_back({ worldPos + glm::vec2(0, size.y),  {uvMin.x, uvMin.y}, tint });

    ++m_spriteCount;
}

void EntityRenderer::end() { flush(); }

void EntityRenderer::flush() {
    if (m_verts.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_verts.size() * sizeof(SpriteVertex), m_verts.data());

    m_shader->bind();
    m_shader->setMat4("uViewProj", m_camera->viewProj());
    m_shader->setMat4("uModel",    glm::mat4(1.f));
    m_shader->setInt ("uSprite",   0);

    if (m_curTex) {
        m_curTex->bind(0);
    } else {
        // No texture: unbind slot 0 (entity shader will use tint color only)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)(m_spriteCount * 6), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    m_verts.clear();
    m_spriteCount = 0;
    m_curTex = nullptr;
}
