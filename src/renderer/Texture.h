#pragma once
#include <glad/gl.h>
#include <string>
#include <cstdint>

class Texture {
public:
    Texture() = default;
    ~Texture();

    // Non-copyable, movable
    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& o) noexcept;
    Texture& operator=(Texture&& o) noexcept;

    // Load from file (uses stb_image)
    bool loadFromFile(const std::string& path, bool flipY = true);

    // Create from raw RGBA pixel data
    void createFromData(int w, int h, const uint8_t* data);

    // Create empty texture (for FBOs)
    void createEmpty(int w, int h, GLenum internalFmt = GL_RGBA8, GLenum fmt = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE);

    void bind(int slot = 0) const;
    void unbind()           const;

    int    width()  const { return m_width; }
    int    height() const { return m_height; }
    GLuint id()     const { return m_id; }
    bool   valid()  const { return m_id != 0; }

    void setFiltering(GLenum min, GLenum mag);
    void setWrap(GLenum s, GLenum t);

private:
    GLuint m_id     = 0;
    int    m_width  = 0;
    int    m_height = 0;
};
