#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"
#include "Texture.h"
#include "Camera.h"

// One entry in the per-instance GPU buffer
struct TileInstance {
    glm::vec2 tilePos;    // World-space top-left (pixels)
    glm::vec2 atlasUV;    // Top-left UV in atlas
    glm::vec3 light;      // RGB light 0..1
    float     _pad = 0.f; // Align to 32 bytes
};
static_assert(sizeof(TileInstance) == 32, "TileInstance must be 32 bytes");

class TileRenderer {
public:
    TileRenderer();
    ~TileRenderer();

    TileRenderer(const TileRenderer&)            = delete;
    TileRenderer& operator=(const TileRenderer&) = delete;

    // Upload instance data for a batch and draw
    void draw(const std::vector<TileInstance>& instances,
              const Texture& atlas,
              const Shader&  shader,
              const Camera&  camera,
              float tileSize,
              float atlasCellSize);

private:
    void buildQuad();

    GLuint m_vao         = 0;
    GLuint m_quadVBO     = 0;  // shared quad geometry
    GLuint m_quadEBO     = 0;
    GLuint m_instanceVBO = 0;
    size_t m_instanceCap = 0;
};
