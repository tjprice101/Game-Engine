#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

Camera::Camera(float viewportWidth, float viewportHeight)
    : m_vpW(viewportWidth), m_vpH(viewportHeight) {}

void Camera::setViewport(float w, float h) {
    m_vpW = w; m_vpH = h; m_dirty = true;
}

void Camera::zoomBy(float factor) {
    m_zoom = std::clamp(m_zoom * factor, 0.25f, 8.0f);
    m_dirty = true;
}

const glm::mat4& Camera::viewProj() const {
    if (m_dirty) recompute();
    return m_viewProj;
}

void Camera::recompute() const {
    float halfW = (m_vpW  * 0.5f) / m_zoom;
    float halfH = (m_vpH  * 0.5f) / m_zoom;

    glm::mat4 proj = glm::ortho(
        m_position.x - halfW, m_position.x + halfW,
        m_position.y + halfH, m_position.y - halfH,   // y flipped: +y = down
        -1.0f, 1.0f);

    m_viewProj = proj;  // No separate view matrix needed for ortho
    m_dirty    = false;
}

glm::vec2 Camera::screenToWorld(const glm::vec2& sp) const {
    float halfW = (m_vpW * 0.5f) / m_zoom;
    float halfH = (m_vpH * 0.5f) / m_zoom;
    float wx = m_position.x - halfW + (sp.x / m_vpW) * halfW * 2.0f;
    float wy = m_position.y - halfH + (sp.y / m_vpH) * halfH * 2.0f;
    return { wx, wy };
}

glm::vec2 Camera::worldToScreen(const glm::vec2& wp) const {
    float halfW = (m_vpW * 0.5f) / m_zoom;
    float halfH = (m_vpH * 0.5f) / m_zoom;
    float sx = ((wp.x - m_position.x + halfW) / (halfW * 2.0f)) * m_vpW;
    float sy = ((wp.y - m_position.y + halfH) / (halfH * 2.0f)) * m_vpH;
    return { sx, sy };
}

float Camera::left()   const { return m_position.x - (m_vpW * 0.5f) / m_zoom; }
float Camera::right()  const { return m_position.x + (m_vpW * 0.5f) / m_zoom; }
float Camera::top()    const { return m_position.y - (m_vpH * 0.5f) / m_zoom; }
float Camera::bottom() const { return m_position.y + (m_vpH * 0.5f) / m_zoom; }
