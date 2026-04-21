#pragma once
#include <glad/gl.h>
#include "Texture.h"

class FrameBuffer {
public:
    FrameBuffer() = default;
    ~FrameBuffer();

    FrameBuffer(const FrameBuffer&)            = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    void create(int width, int height, bool depthStencil = false);
    void resize(int width, int height);

    void bind()   const;
    void unbind() const;

    Texture&       colorTexture()       { return m_color; }
    const Texture& colorTexture() const { return m_color; }

    int width()  const { return m_width; }
    int height() const { return m_height; }

private:
    GLuint  m_fbo         = 0;
    GLuint  m_rbo         = 0;  // depth/stencil renderbuffer
    Texture m_color;
    int     m_width       = 0;
    int     m_height      = 0;
    bool    m_depthStencil= false;
};
