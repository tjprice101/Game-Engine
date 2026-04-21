#include "TileRenderer.h"
#include <cstring>

// Quad vertices: pos (-0.5..0.5), UV (0..1) — origin at top-left corner
static const float kQuadVerts[] = {
//  x     y     u    v
    0.0f, 0.0f, 0.f, 1.f,  // top-left  (screen +y = down)
    1.0f, 0.0f, 1.f, 1.f,  // top-right
    1.0f, 1.0f, 1.f, 0.f,  // bottom-right
    0.0f, 1.0f, 0.f, 0.f,  // bottom-left
};
static const uint32_t kQuadIdx[] = { 0,1,2, 0,2,3 };

TileRenderer::TileRenderer() { buildQuad(); }

TileRenderer::~TileRenderer() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_quadVBO);
    glDeleteBuffers(1, &m_quadEBO);
    glDeleteBuffers(1, &m_instanceVBO);
}

void TileRenderer::buildQuad() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_quadVBO);
    glGenBuffers(1, &m_quadEBO);
    glGenBuffers(1, &m_instanceVBO);

    glBindVertexArray(m_vao);

    // Quad geometry
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVerts), kQuadVerts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kQuadIdx), kQuadIdx, GL_STATIC_DRAW);

    // Attrib 0: vertex position (vec2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    // Attrib 1: vertex UV (vec2)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));

    // Instance buffer (will be filled on each draw call)
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);

    // Attrib 2: tilePos (vec2) — instance
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TileInstance), (void*)offsetof(TileInstance, tilePos));
    glVertexAttribDivisor(2, 1);

    // Attrib 3: atlasUV (vec2) — instance
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(TileInstance), (void*)offsetof(TileInstance, atlasUV));
    glVertexAttribDivisor(3, 1);

    // Attrib 4: light (vec3) — instance
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(TileInstance), (void*)offsetof(TileInstance, light));
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);
}

void TileRenderer::draw(const std::vector<TileInstance>& instances,
                        const Texture& atlas,
                        const Shader&  shader,
                        const Camera&  camera,
                        float tileSize,
                        float atlasCellSize)
{
    if (instances.empty()) return;

    // Upload instance data
    size_t needed = instances.size() * sizeof(TileInstance);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    if (instances.size() > m_instanceCap) {
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)needed, instances.data(), GL_DYNAMIC_DRAW);
        m_instanceCap = instances.size();
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)needed, instances.data());
    }

    shader.bind();
    shader.setMat4 ("uViewProj",      camera.viewProj());
    shader.setFloat("uTileSize",      tileSize);
    shader.setFloat("uAtlasCellSize", atlasCellSize);
    shader.setInt  ("uAtlas",         0);

    atlas.bind(0);

    glBindVertexArray(m_vao);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                            static_cast<GLsizei>(instances.size()));
    glBindVertexArray(0);

    shader.unbind();
}
