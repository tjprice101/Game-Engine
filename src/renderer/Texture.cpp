#include "Texture.h"
#include <stdexcept>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::~Texture() {
    if (m_id) glDeleteTextures(1, &m_id);
}

Texture::Texture(Texture&& o) noexcept : m_id(o.m_id), m_width(o.m_width), m_height(o.m_height) {
    o.m_id = 0; o.m_width = 0; o.m_height = 0;
}
Texture& Texture::operator=(Texture&& o) noexcept {
    if (this != &o) {
        if (m_id) glDeleteTextures(1, &m_id);
        m_id = o.m_id; m_width = o.m_width; m_height = o.m_height;
        o.m_id = 0; o.m_width = 0; o.m_height = 0;
    }
    return *this;
}

bool Texture::loadFromFile(const std::string& path, bool flipY) {
    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);
    int channels;
    uint8_t* data = stbi_load(path.c_str(), &m_width, &m_height, &channels, 4);
    if (!data) return false;
    createFromData(m_width, m_height, data);
    stbi_image_free(data);
    return true;
}

void Texture::createFromData(int w, int h, const uint8_t* data) {
    if (m_id) glDeleteTextures(1, &m_id);
    m_width = w; m_height = h;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Nearest-neighbour for pixel art
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::createEmpty(int w, int h, GLenum internalFmt, GLenum fmt, GLenum type) {
    if (m_id) glDeleteTextures(1, &m_id);
    m_width = w; m_height = h;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, w, h, 0, fmt, type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::bind(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
}
void Texture::unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }

void Texture::setFiltering(GLenum min, GLenum mag) {
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
    glBindTexture(GL_TEXTURE_2D, 0);
}
void Texture::setWrap(GLenum s, GLenum t) {
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t);
    glBindTexture(GL_TEXTURE_2D, 0);
}
