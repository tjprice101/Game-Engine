#include "FrameBuffer.h"
#include <stdexcept>

FrameBuffer::~FrameBuffer() {
    if (m_rbo) glDeleteRenderbuffers(1, &m_rbo);
    if (m_fbo) glDeleteFramebuffers (1, &m_fbo);
}

void FrameBuffer::create(int width, int height, bool depthStencil) {
    m_width = width; m_height = height; m_depthStencil = depthStencil;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Color attachment
    m_color.createEmpty(width, height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_color.id(), 0);

    // Optional depth/stencil renderbuffer
    if (depthStencil) {
        glGenRenderbuffers(1, &m_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Framebuffer is not complete");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::resize(int width, int height) {
    if (m_rbo) { glDeleteRenderbuffers(1, &m_rbo); m_rbo = 0; }
    if (m_fbo) { glDeleteFramebuffers (1, &m_fbo); m_fbo = 0; }
    create(width, height, m_depthStencil);
}

void FrameBuffer::bind()   const { glBindFramebuffer(GL_FRAMEBUFFER, m_fbo); }
void FrameBuffer::unbind() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
