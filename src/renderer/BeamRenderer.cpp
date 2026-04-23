#include "renderer/BeamRenderer.h"
#include "renderer/ShaderLibrary.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

// ---- Construction -----------------------------------------------------------

BeamRenderer::BeamRenderer() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    using BV = BeamVertex;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(BV), (void*)offsetof(BV, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(BV), (void*)offsetof(BV, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(BV), (void*)offsetof(BV, color));

    glBindVertexArray(0);
}

BeamRenderer::~BeamRenderer() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
}

void BeamRenderer::init() {
    auto& sl = ShaderLibrary::instance();
    sl.load("beam", "beam.vert", "beam.frag");
}

// ---- Beam management --------------------------------------------------------

int BeamRenderer::addBeam(glm::vec2 start, glm::vec2 end, BeamStyle style,
                           const glm::vec4& color, float width, float lifetime) {
    for (int i = 0; i < MAX_BEAMS; ++i) {
        if (!m_beams[i].active) {
            Beam& b   = m_beams[i];
            b.start   = start;
            b.end     = end;
            b.style   = style;
            b.color   = color;
            b.width   = width;
            b.lifetime= lifetime;
            b.age     = 0.f;
            b.active  = true;
            return i;
        }
    }
    return -1;
}

void BeamRenderer::removeBeam(int id) {
    if (id >= 0 && id < MAX_BEAMS)
        m_beams[id].active = false;
}

void BeamRenderer::updateBeam(int id, glm::vec2 start, glm::vec2 end) {
    if (id >= 0 && id < MAX_BEAMS && m_beams[id].active) {
        m_beams[id].start = start;
        m_beams[id].end   = end;
    }
}

// ---- Update -----------------------------------------------------------------

void BeamRenderer::update(float dt) {
    for (auto& b : m_beams) {
        if (!b.active) continue;
        if (b.lifetime > 0.f) {
            b.age += dt;
            if (b.age >= b.lifetime) b.active = false;
        }
    }
}

// ---- Quad generation --------------------------------------------------------

void BeamRenderer::buildBeamQuad(const Beam& b, std::vector<BeamVertex>& verts,
                                   std::vector<uint32_t>& idxs, int baseIdx) const {
    glm::vec2 dir  = b.end - b.start;
    float     len  = glm::length(dir);
    if (len < 0.001f) return;
    dir /= len;

    glm::vec2 perp     = {-dir.y, dir.x};
    float     halfW    = b.width * 0.5f;

    // Fade by lifetime if set
    float alpha = b.color.a;
    if (b.lifetime > 0.f)
        alpha *= 1.f - (b.age / b.lifetime);

    glm::vec4 col = {b.color.r, b.color.g, b.color.b, alpha};

    // 4 vertices: bottom-left, bottom-right, top-right, top-left
    // UV: x=along(0..1), y=across(-1..1)
    verts.push_back({ b.start - perp * halfW, {0.f, -1.f}, col });
    verts.push_back({ b.start + perp * halfW, {0.f,  1.f}, col });
    verts.push_back({ b.end   + perp * halfW, {1.f,  1.f}, col });
    verts.push_back({ b.end   - perp * halfW, {1.f, -1.f}, col });

    idxs.push_back(baseIdx + 0);
    idxs.push_back(baseIdx + 1);
    idxs.push_back(baseIdx + 2);
    idxs.push_back(baseIdx + 0);
    idxs.push_back(baseIdx + 2);
    idxs.push_back(baseIdx + 3);
}

// ---- Draw -------------------------------------------------------------------

void BeamRenderer::draw(const Camera& /*camera*/, float time) {
    auto& sl = ShaderLibrary::instance();
    if (!sl.has("beam")) return;
    Shader& sh = sl.get("beam");

    m_vertBuf.clear();
    m_idxBuf.clear();

    int base = 0;
    for (const auto& b : m_beams) {
        if (!b.active) continue;
        buildBeamQuad(b, m_vertBuf, m_idxBuf, base);
        base += 4;
    }
    if (m_vertBuf.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(m_vertBuf.size() * sizeof(BeamVertex)),
                 m_vertBuf.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(m_idxBuf.size() * sizeof(uint32_t)),
                 m_idxBuf.data(), GL_DYNAMIC_DRAW);

    sh.bind();
    sh.setFloat("uTime", time);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(m_vao);

    // Draw each beam with its style uniform
    int idxOffset = 0;
    for (const auto& b : m_beams) {
        if (!b.active) continue;
        sh.setInt("uStyle", (int)b.style);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
                       (void*)(idxOffset * sizeof(uint32_t)));
        idxOffset += 6;
    }

    glBindVertexArray(0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    sh.unbind();
}
