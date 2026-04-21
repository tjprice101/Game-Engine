#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera() = default;
    Camera(float viewportWidth, float viewportHeight);

    void setViewport(float w, float h);
    void setPosition(const glm::vec2& pos) { m_position = pos; m_dirty = true; }
    void move(const glm::vec2& delta)      { m_position += delta; m_dirty = true; }
    void setZoom(float zoom)               { m_zoom = zoom; m_dirty = true; }
    void zoomBy(float factor);

    // World-space position at the centre of the screen
    const glm::vec2& position()     const { return m_position; }
    float             zoom()         const { return m_zoom; }
    float             viewportWidth()  const { return m_vpW; }
    float             viewportHeight() const { return m_vpH; }

    const glm::mat4& viewProj() const;

    // Convert screen → world coordinates
    glm::vec2 screenToWorld(const glm::vec2& screenPos) const;
    // Convert world → screen coordinates
    glm::vec2 worldToScreen(const glm::vec2& worldPos)  const;

    // World-space visible rectangle (left, right, bottom, top)
    float left()   const;
    float right()  const;
    float bottom() const;
    float top()    const;

private:
    void recompute() const;

    glm::vec2 m_position = {0.f, 0.f};
    float     m_zoom     = 1.0f;
    float     m_vpW      = 1280.f;
    float     m_vpH      = 720.f;

    mutable glm::mat4 m_viewProj = glm::mat4(1.f);
    mutable bool      m_dirty    = true;
};
