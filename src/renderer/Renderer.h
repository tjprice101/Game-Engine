#pragma once
#include "Shader.h"
#include "Texture.h"
#include "FrameBuffer.h"
#include "Camera.h"
#include "TileRenderer.h"
#include "EntityRenderer.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    void init(int viewportWidth, int viewportHeight);
    void onResize(int w, int h);

    // ---- Frame flow ----
    void beginFrame();
    void endFrame();       // Runs composite pass and presents to screen

    // ---- Tile rendering ----
    void drawTiles(const std::vector<TileInstance>& instances,
                   const Texture& atlas,
                   const Camera&  camera,
                   float tileSize, float atlasCellSize);

    // ---- Entity rendering ----
    void beginEntities(const Camera& camera);
    void drawSprite(const Texture* tex,
                    glm::vec2 worldPos, glm::vec2 size,
                    glm::vec2 uvMin = {0.f,0.f}, glm::vec2 uvMax = {1.f,1.f},
                    glm::vec4 tint  = {1.f,1.f,1.f,1.f}, bool flipX = false);
    void endEntities();

    // ---- Background / sky ---------------------------------------------------
    // Call after beginFrame(), before drawTiles()
    void drawBackground(float dayTime, float sunlight);

    // ---- Post-process settings ----
    float ambient     = 1.0f;
    float gamma       = 2.2f;
    float contrast    = 1.0f;
    float saturation  = 1.0f;
    float vignette    = 0.2f;

    Camera& debugCamera() { return m_debugCam; }

private:
    void buildFullscreenQuad();
    void compositePass();

    Shader m_tileShader;
    Shader m_entityShader;
    Shader m_compositeShader;
    Shader m_bgShader;

    FrameBuffer  m_sceneFBO;
    TileRenderer m_tileRenderer;
    EntityRenderer m_entityRenderer;

    GLuint m_fsVAO = 0;
    GLuint m_fsVBO = 0;

    Camera m_debugCam;
    int    m_vpW = 0;
    int    m_vpH = 0;

    static const std::string SHADER_DIR;
};
