#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"
#include "Texture.h"
#include "Camera.h"

// One sprite vertex
struct SpriteVertex {
    glm::vec2 pos;
    glm::vec2 uv;
    glm::vec4 color;
};

class EntityRenderer {
public:
    static constexpr int MAX_SPRITES = 4096;

    EntityRenderer();
    ~EntityRenderer();

    EntityRenderer(const EntityRenderer&)            = delete;
    EntityRenderer& operator=(const EntityRenderer&) = delete;

    void begin(const Camera& camera, const Shader& shader);

    // Submit a sprite. origin is (0,0) = top-left of sprite.
    void submit(const Texture* texture,
                glm::vec2     worldPos,
                glm::vec2     size,
                glm::vec2     uvMin   = {0.f,0.f},
                glm::vec2     uvMax   = {1.f,1.f},
                glm::vec4     tint    = {1.f,1.f,1.f,1.f},
                bool          flipX   = false);

    void end();  // Flush any remaining sprites

private:
    void flush();

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;

    const Shader*  m_shader  = nullptr;
    const Texture* m_curTex  = nullptr;
    const Camera*  m_camera  = nullptr;

    std::vector<SpriteVertex> m_verts;
    std::vector<uint32_t>     m_indices;
    int m_spriteCount = 0;
};
