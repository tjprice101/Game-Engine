#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include "renderer/Camera.h"

// ---- TrailStyle -------------------------------------------------------------
// Visual style determines the color gradient along the trail.
enum class TrailStyle {
    Flame,     // Orange → Red → transparent
    Ice,       // White → Blue → transparent
    Lightning, // White → Cyan electric
    Nature,    // Green → Gold
    Cosmic,    // Purple → White → transparent
    Void,      // Dark purple → black
    Sakura,    // Pink → white
    Lunar,     // Blue-white glow
    Custom,    // Uses trailColorStart/End directly
};

// ---- TrailVertex ------------------------------------------------------------
// Per-vertex data for trail rendering (7 floats, stride = 28 bytes).
struct TrailVertex {
    glm::vec2 pos;   // world position
    float     t;     // 0=head (newest point), 1=tail (oldest)
    glm::vec4 color; // interpolated RGBA
};
static_assert(sizeof(TrailVertex) == 28);

// ---- TrailHandle ------------------------------------------------------------
// A single trail instance (a ribbon of connected quads).
struct Trail {
    static constexpr int MAX_POINTS = 32;

    glm::vec2  points[MAX_POINTS];
    int        pointCount    = 0;
    float      width         = 6.f;        // peak width at head
    TrailStyle style         = TrailStyle::Cosmic;
    glm::vec4  colorHead     = {1.f, 1.f, 1.f, 1.f}; // used by Custom style
    glm::vec4  colorTail     = {0.5f, 0.5f, 1.f, 0.f};
    float      fadeStart     = 0.4f;       // t value where fade begins
    bool       active        = false;
};

// ---- TrailRenderer ----------------------------------------------------------
// Vertex-strip trail renderer inspired by MagnumOpus's PrimitiveTrailRenderer.
// Trails are updated CPU-side and uploaded each frame via a dynamic VBO.
//
// Usage:
//   int id = m_trails.createTrail(TrailStyle::Cosmic, 8.f);
//   m_trails.addPoint(id, entityPos);   // call each frame
//   m_trails.draw(camera);

class TrailRenderer {
public:
    static constexpr int MAX_TRAILS   = 256;
    static constexpr int VERTS_PER_TRAIL = Trail::MAX_POINTS * 2;

    TrailRenderer();
    ~TrailRenderer();

    TrailRenderer(const TrailRenderer&)            = delete;
    TrailRenderer& operator=(const TrailRenderer&) = delete;

    // ---- Lifecycle ----------------------------------------------------------
    void init();

    // ---- Trail management ---------------------------------------------------
    int  createTrail(TrailStyle style = TrailStyle::Cosmic, float width = 6.f);
    void destroyTrail(int id);
    void addPoint(int id, glm::vec2 worldPos);
    void setActive(int id, bool active);
    Trail* getTrail(int id);

    // ---- Update & Draw ------------------------------------------------------
    void update(float dt);  // ages out old points
    void draw(const Camera& camera);

private:
    static glm::vec4 styleColorHead(TrailStyle s);
    static glm::vec4 styleColorTail(TrailStyle s);
    void buildVertices(const Trail& trail, std::vector<TrailVertex>& out);

    Trail  m_trails[MAX_TRAILS];

    GLuint m_vao = 0;
    GLuint m_vbo = 0;

    std::vector<TrailVertex> m_vertBuf;
};
