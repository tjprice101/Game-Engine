#pragma once
#include <glad/gl.h>
#include "renderer/Texture.h"
#include <vector>
#include <stdexcept>

// ---- RenderTarget -----------------------------------------------------------
// Multi-attachment framebuffer. Supports up to 8 color attachments + depth.
// Usage:
//   RenderTarget rt;
//   rt.addColorAttachment(w, h, GL_RGBA8);       // attachment 0
//   rt.addColorAttachment(w, h, GL_RGBA16F);      // attachment 1 (HDR)
//   rt.addDepthAttachment(w, h);
//   rt.build();

class RenderTarget {
public:
    RenderTarget()  = default;
    ~RenderTarget() { destroy(); }

    RenderTarget(const RenderTarget&)            = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    // ---- Configuration (call before build) ----------------------------------
    void addColorAttachment(int w, int h,
                            GLenum internalFmt = GL_RGBA8,
                            GLenum fmt         = GL_RGBA,
                            GLenum type        = GL_UNSIGNED_BYTE,
                            GLenum filter      = GL_LINEAR);

    void addDepthAttachment(int w, int h);

    void build();
    void resize(int w, int h);
    void destroy();

    // ---- Usage --------------------------------------------------------------
    void bind()   const;
    void unbind() const;

    // Bind a color attachment as a texture on the given slot
    void bindColorTex(int attachIndex, int slot = 0) const;

    Texture&       colorTexture(int idx = 0)       { return m_colorTextures[idx]; }
    const Texture& colorTexture(int idx = 0) const { return m_colorTextures[idx]; }

    int colorCount() const { return (int)m_colorTextures.size(); }
    int width()      const { return m_width; }
    int height()     const { return m_height; }
    bool valid()     const { return m_fbo != 0; }

private:
    struct AttachSpec {
        int     w, h;
        GLenum  internalFmt;
        GLenum  fmt;
        GLenum  type;
        GLenum  filter;
    };

    void buildInternal();

    GLuint m_fbo = 0;
    GLuint m_rbo = 0; // depth renderbuffer

    std::vector<AttachSpec> m_specs;
    std::vector<Texture>    m_colorTextures;
    bool m_hasDepth = false;
    int  m_width    = 0;
    int  m_height   = 0;
};
