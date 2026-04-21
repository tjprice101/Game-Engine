#include "Renderer.h"
#include <glad/gl.h>
#include <stdexcept>

const std::string Renderer::SHADER_DIR = "assets/shaders/";

void Renderer::init(int viewportWidth, int viewportHeight) {
    m_vpW = viewportWidth;
    m_vpH = viewportHeight;

    // Load shaders
    m_tileShader      = Shader(SHADER_DIR + "tile.vert",      SHADER_DIR + "tile.frag");
    m_entityShader    = Shader(SHADER_DIR + "entity.vert",    SHADER_DIR + "entity.frag");
    m_compositeShader = Shader(SHADER_DIR + "composite.vert", SHADER_DIR + "composite.frag");
    m_bgShader        = Shader(SHADER_DIR + "bg.vert",        SHADER_DIR + "bg.frag");

    m_sceneFBO.create(viewportWidth, viewportHeight, false);

    buildFullscreenQuad();

    m_debugCam.setViewport(static_cast<float>(viewportWidth), static_cast<float>(viewportHeight));
}

void Renderer::onResize(int w, int h) {
    m_vpW = w; m_vpH = h;
    glViewport(0, 0, w, h);
    m_sceneFBO.resize(w, h);
    m_debugCam.setViewport(static_cast<float>(w), static_cast<float>(h));
}

void Renderer::buildFullscreenQuad() {
    // Clip-space quad covering the whole screen
    float verts[] = {
        -1.f, -1.f,   0.f, 0.f,
         1.f, -1.f,   1.f, 0.f,
         1.f,  1.f,   1.f, 1.f,
        -1.f,  1.f,   0.f, 1.f,
    };
    uint32_t idx[] = {0,1,2, 0,2,3};

    glGenVertexArrays(1, &m_fsVAO);
    glGenBuffers(1, &m_fsVBO);

    glBindVertexArray(m_fsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_fsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    GLuint ebo; glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));

    glBindVertexArray(0);
}

void Renderer::beginFrame() {
    m_sceneFBO.bind();
    glViewport(0, 0, m_vpW, m_vpH);
    glClearColor(0.05f, 0.05f, 0.08f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::endFrame() {
    compositePass();
}

void Renderer::drawTiles(const std::vector<TileInstance>& instances,
                         const Texture& atlas,
                         const Camera&  camera,
                         float tileSize, float atlasCellSize)
{
    m_tileRenderer.draw(instances, atlas, m_tileShader, camera, tileSize, atlasCellSize);
}

void Renderer::beginEntities(const Camera& camera) {
    m_entityRenderer.begin(camera, m_entityShader);
}

void Renderer::drawSprite(const Texture* tex,
                          glm::vec2 worldPos, glm::vec2 size,
                          glm::vec2 uvMin, glm::vec2 uvMax,
                          glm::vec4 tint, bool flipX)
{
    m_entityRenderer.submit(tex, worldPos, size, uvMin, uvMax, tint, flipX);
}

void Renderer::endEntities() {
    m_entityRenderer.end();
}

void Renderer::compositePass() {
    // Unbind scene FBO → render to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_vpW, m_vpH);
    glClear(GL_COLOR_BUFFER_BIT);

    m_compositeShader.bind();
    m_compositeShader.setInt  ("uScene",           0);
    m_compositeShader.setFloat("uAmbient",         ambient);
    m_compositeShader.setFloat("uGamma",           gamma);
    m_compositeShader.setFloat("uContrast",        contrast);
    m_compositeShader.setFloat("uSaturation",      saturation);
    m_compositeShader.setFloat("uVignetteStrength",vignette);

    m_sceneFBO.colorTexture().bind(0);

    glBindVertexArray(m_fsVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    m_compositeShader.unbind();
}

void Renderer::drawBackground(float dayTime, float sunlight) {
    // Draw directly into the scene FBO (already bound from beginFrame)
    // Disable blend so sky fully replaces background
    glDisable(GL_BLEND);

    m_bgShader.bind();
    m_bgShader.setFloat("uDayTime",  dayTime);
    m_bgShader.setFloat("uSunlight", sunlight);

    glBindVertexArray(m_fsVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    m_bgShader.unbind();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
