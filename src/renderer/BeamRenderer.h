#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include "renderer/Camera.h"

// ---- BeamStyle --------------------------------------------------------------
// Five beam styles inspired by MagnumOpus's BeamVariations.
enum class BeamStyle {
    Laser,    // Thin bright core with soft glow falloff
    Ribbon,   // Wavy animated ribbon
    Spiral,   // Helical pattern along the beam axis
    Chain,    // Segmented chain lightning pulses
    Radiant,  // Radiating rays from a center line
};

// ---- BeamVertex -------------------------------------------------------------
// Per-vertex data for beam rendering (8 floats, stride = 32 bytes).
struct BeamVertex {
    glm::vec2 pos;   // world position
    glm::vec2 uv;    // (along 0..1, across -1..1 from center)
    glm::vec4 color; // beam color + intensity in alpha
};
static_assert(sizeof(BeamVertex) == 32);

// ---- Beam -------------------------------------------------------------------
struct Beam {
    glm::vec2  start;
    glm::vec2  end;
    float      width      = 8.f;
    BeamStyle  style      = BeamStyle::Laser;
    glm::vec4  color      = {1.f, 1.f, 1.f, 1.f};
    float      lifetime   = 0.f;   // 0 = persistent until removed
    float      age        = 0.f;
    bool       active     = false;
};

// ---- BeamRenderer -----------------------------------------------------------
// Dynamic beam/laser renderer inspired by MagnumOpus's BeamVariations system.
// Beams are generated CPU-side as quad strips and rendered additively.
//
// Usage:
//   int id = m_beams.addBeam({0,0}, {200,0}, BeamStyle::Laser, {1,0.5f,0,1});
//   m_beams.updateBeam(id, newStart, newEnd);  // move persistent beam
//   m_beams.removeBeam(id);

class BeamRenderer {
public:
    static constexpr int MAX_BEAMS = 64;

    BeamRenderer();
    ~BeamRenderer();

    BeamRenderer(const BeamRenderer&)            = delete;
    BeamRenderer& operator=(const BeamRenderer&) = delete;

    void init();

    // ---- Beam management ----------------------------------------------------
    int  addBeam(glm::vec2 start, glm::vec2 end, BeamStyle style,
                 const glm::vec4& color, float width = 8.f, float lifetime = 0.f);
    void removeBeam(int id);
    void updateBeam(int id, glm::vec2 start, glm::vec2 end);

    // ---- Update & Draw ------------------------------------------------------
    void update(float dt);
    void draw(const Camera& camera, float time);

private:
    void buildBeamQuad(const Beam& b, std::vector<BeamVertex>& verts,
                       std::vector<uint32_t>& idxs, int baseIdx) const;

    Beam   m_beams[MAX_BEAMS];
    int    m_beamCount = 0;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;

    std::vector<BeamVertex>  m_vertBuf;
    std::vector<uint32_t>    m_idxBuf;
};
