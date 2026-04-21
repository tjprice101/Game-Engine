#include "RenderTarget.h"
#include <stdexcept>

void RenderTarget::addColorAttachment(int w, int h,
                                       GLenum internalFmt,
                                       GLenum fmt,
                                       GLenum type,
                                       GLenum filter)
{
    m_specs.push_back({ w, h, internalFmt, fmt, type, filter });
    m_width  = w;
    m_height = h;
}

void RenderTarget::addDepthAttachment(int w, int h) {
    m_hasDepth = true;
    m_width    = w;
    m_height   = h;
}

void RenderTarget::build() {
    buildInternal();
}

void RenderTarget::buildInternal() {
    destroy();

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    m_colorTextures.resize(m_specs.size());
    std::vector<GLenum> drawBuffers;

    for (int i = 0; i < (int)m_specs.size(); ++i) {
        const auto& s = m_specs[i];
        m_colorTextures[i].createEmpty(s.w, s.h, s.internalFmt, s.fmt, s.type);
        m_colorTextures[i].setFiltering(s.filter, s.filter);
        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0 + i,
                               GL_TEXTURE_2D,
                               m_colorTextures[i].id(), 0);
        drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
    }

    if (!drawBuffers.empty())
        glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data());

    if (m_hasDepth) {
        glGenRenderbuffers(1, &m_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, m_rbo);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("RenderTarget: framebuffer incomplete");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderTarget::resize(int w, int h) {
    m_width = w; m_height = h;
    for (auto& s : m_specs) { s.w = w; s.h = h; }
    buildInternal();
}

void RenderTarget::destroy() {
    m_colorTextures.clear();
    if (m_rbo) { glDeleteRenderbuffers(1, &m_rbo); m_rbo = 0; }
    if (m_fbo) { glDeleteFramebuffers(1, &m_fbo);  m_fbo = 0; }
}

void RenderTarget::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void RenderTarget::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderTarget::bindColorTex(int idx, int slot) const {
    m_colorTextures[idx].bind(slot);
}
