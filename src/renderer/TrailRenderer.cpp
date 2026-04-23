#include "renderer/TrailRenderer.h"
#include "renderer/ShaderLibrary.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <algorithm>

// ---- Style color tables -----------------------------------------------------

glm::vec4 TrailRenderer::styleColorHead(TrailStyle s) {
    switch (s) {
    case TrailStyle::Flame:     return {1.0f, 0.7f, 0.1f, 1.0f};
    case TrailStyle::Ice:       return {0.9f, 0.97f, 1.0f, 1.0f};
    case TrailStyle::Lightning: return {0.9f, 0.95f, 1.0f, 1.0f};
    case TrailStyle::Nature:    return {0.4f, 0.9f, 0.2f, 1.0f};
    case TrailStyle::Cosmic:    return {0.7f, 0.4f, 1.0f, 1.0f};
    case TrailStyle::Void:      return {0.35f, 0.0f, 0.5f, 1.0f};
    case TrailStyle::Sakura:    return {1.0f, 0.6f, 0.75f, 1.0f};
    case TrailStyle::Lunar:     return {0.65f, 0.75f, 1.0f, 1.0f};
    default:                    return {1.0f, 1.0f, 1.0f, 1.0f};
    }
}

glm::vec4 TrailRenderer::styleColorTail(TrailStyle s) {
    switch (s) {
    case TrailStyle::Flame:     return {0.9f, 0.1f, 0.0f, 0.0f};
    case TrailStyle::Ice:       return {0.3f, 0.6f, 1.0f, 0.0f};
    case TrailStyle::Lightning: return {0.4f, 0.6f, 1.0f, 0.0f};
    case TrailStyle::Nature:    return {0.8f, 0.75f, 0.1f, 0.0f};
    case TrailStyle::Cosmic:    return {0.3f, 0.1f, 0.8f, 0.0f};
    case TrailStyle::Void:      return {0.1f, 0.0f, 0.2f, 0.0f};
    case TrailStyle::Sakura:    return {1.0f, 0.9f, 0.95f, 0.0f};
    case TrailStyle::Lunar:     return {0.4f, 0.5f, 1.0f, 0.0f};
    default:                    return {0.5f, 0.5f, 0.5f, 0.0f};
    }
}

// ---- Construction -----------------------------------------------------------

TrailRenderer::TrailRenderer() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    using TV = TrailVertex;
    // location 0: vec2 pos   (offset 0,  size 8)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TV), (void*)offsetof(TV, pos));
    // location 1: float t    (offset 8,  size 4)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(TV), (void*)offsetof(TV, t));
    // location 2: vec4 color (offset 12, size 16)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(TV), (void*)offsetof(TV, color));

    glBindVertexArray(0);
}

TrailRenderer::~TrailRenderer() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
}

void TrailRenderer::init() {
    auto& sl = ShaderLibrary::instance();
    sl.load("trail", "trail.vert", "trail.frag");
}

// ---- Trail management -------------------------------------------------------

int TrailRenderer::createTrail(TrailStyle style, float width) {
    for (int i = 0; i < MAX_TRAILS; ++i) {
        if (!m_trails[i].active) {
            Trail& tr       = m_trails[i];
            tr              = Trail{};
            tr.style        = style;
            tr.width        = width;
            tr.colorHead    = styleColorHead(style);
            tr.colorTail    = styleColorTail(style);
            tr.active       = true;
            tr.pointCount   = 0;
            return i;
        }
    }
    return -1; // no free slot
}

void TrailRenderer::destroyTrail(int id) {
    if (id < 0 || id >= MAX_TRAILS) return;
    m_trails[id] = Trail{};
    m_trails[id].active = false;
}

void TrailRenderer::addPoint(int id, glm::vec2 worldPos) {
    if (id < 0 || id >= MAX_TRAILS || !m_trails[id].active) return;
    Trail& tr = m_trails[id];

    // Shift existing points back to make room at head (index 0)
    if (tr.pointCount < Trail::MAX_POINTS) {
        ++tr.pointCount;
    }
    for (int i = tr.pointCount - 1; i > 0; --i)
        tr.points[i] = tr.points[i - 1];
    tr.points[0] = worldPos;
}

void TrailRenderer::setActive(int id, bool active) {
    if (id >= 0 && id < MAX_TRAILS)
        m_trails[id].active = active;
}

Trail* TrailRenderer::getTrail(int id) {
    if (id < 0 || id >= MAX_TRAILS) return nullptr;
    return &m_trails[id];
}

void TrailRenderer::update(float /*dt*/) {
    // Trails are point-based; age/lifetime would need per-point timestamps.
    // This is a simple version — caller drives addPoint frequency.
}

// ---- Vertex generation ------------------------------------------------------

void TrailRenderer::buildVertices(const Trail& trail, std::vector<TrailVertex>& out) {
    int n = trail.pointCount;
    if (n < 2) return;

    for (int i = 0; i < n; ++i) {
        float t = float(i) / float(n - 1); // 0=head, 1=tail

        // Compute perpendicular direction at this point
        glm::vec2 dir;
        if (i == 0)
            dir = trail.points[0] - trail.points[1];
        else if (i == n - 1)
            dir = trail.points[n - 2] - trail.points[n - 1];
        else
            dir = trail.points[i - 1] - trail.points[i + 1];

        float len = glm::length(dir);
        if (len < 0.001f) dir = {0.f, 1.f};
        else               dir /= len;

        glm::vec2 perp = {-dir.y, dir.x};

        // Width tapers towards tail
        float halfW = trail.width * 0.5f * (1.f - t * 0.7f);

        glm::vec4 color = glm::mix(trail.colorHead, trail.colorTail, t);

        out.push_back({ trail.points[i] - perp * halfW, t, color });
        out.push_back({ trail.points[i] + perp * halfW, t, color });
    }
}

// ---- Draw -------------------------------------------------------------------

void TrailRenderer::draw(const Camera& camera) {
    auto& sl = ShaderLibrary::instance();
    if (!sl.has("trail")) return;
    Shader& sh = sl.get("trail");

    m_vertBuf.clear();
    for (auto& tr : m_trails) {
        if (!tr.active || tr.pointCount < 2) continue;
        buildVertices(tr, m_vertBuf);
    }
    if (m_vertBuf.empty()) return;

    // Upload
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(m_vertBuf.size() * sizeof(TrailVertex)),
                 m_vertBuf.data(), GL_DYNAMIC_DRAW);

    sh.bind();
    sh.setFloat("uFadeStart", 0.35f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(m_vao);

    // Draw each trail as a TRIANGLE_STRIP
    int offset = 0;
    for (auto& tr : m_trails) {
        if (!tr.active || tr.pointCount < 2) continue;
        int vertCount = tr.pointCount * 2;
        glDrawArrays(GL_TRIANGLE_STRIP, offset, vertCount);
        offset += vertCount;
    }

    glBindVertexArray(0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    sh.unbind();
}
