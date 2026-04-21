#pragma once
#include <glad/gl.h>
#include <cstddef>
#include <cstring>

// ---- OpenGL Uniform Buffer Object wrapper -----------------------------------
// Binds to a fixed binding point. Shaders declare:
//   layout(std140, binding = N) uniform BlockName { ... };

class UniformBuffer {
public:
    UniformBuffer()  = default;
    ~UniformBuffer() { if (m_ubo) glDeleteBuffers(1, &m_ubo); }

    UniformBuffer(const UniformBuffer&)            = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;
    UniformBuffer(UniformBuffer&& o) noexcept
        : m_ubo(o.m_ubo), m_bindPoint(o.m_bindPoint), m_size(o.m_size)
    { o.m_ubo = 0; }

    void create(size_t byteSize, GLuint bindingPoint,
                GLenum usage = GL_DYNAMIC_DRAW)
    {
        m_size      = byteSize;
        m_bindPoint = bindingPoint;
        glGenBuffers(1, &m_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
        glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)byteSize, nullptr, usage);
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    // Update the whole buffer
    void update(const void* data) {
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, (GLsizeiptr)m_size, data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    // Update a sub-range
    void updateRange(const void* data, size_t offset, size_t size) {
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, (GLintptr)offset, (GLsizeiptr)size, data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void bind() const {
        glBindBufferBase(GL_UNIFORM_BUFFER, m_bindPoint, m_ubo);
    }

    GLuint id()          const { return m_ubo; }
    GLuint bindingPoint()const { return m_bindPoint; }
    size_t size()        const { return m_size; }

private:
    GLuint m_ubo        = 0;
    GLuint m_bindPoint  = 0;
    size_t m_size       = 0;
};

// ---- Standard UBO binding points -------------------------------------------
namespace UBOBindings {
    constexpr GLuint Camera    = 0;
    constexpr GLuint FrameData = 1;
    constexpr GLuint Lights    = 2;
}

// ---- Camera UBO data (binding 0) -------------------------------------------
struct alignas(16) CameraUBO {
    float viewProj[16];   // mat4
    float position[2];    // vec2 world position
    float zoom;
    float _pad0;
    float viewport[2];    // vec2 viewport size in pixels
    float _pad1[2];
};

// ---- Per-frame data UBO (binding 1) ----------------------------------------
struct alignas(16) FrameDataUBO {
    float time;        // total elapsed seconds
    float deltaTime;
    float dayTime;     // 0..1
    float sunlight;    // 0..1
    float screenW;
    float screenH;
    float _pad[2];
};
