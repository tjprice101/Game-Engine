#include "LightSystem.h"
#include "renderer/ShaderLibrary.h"
#include <glm/gtc/type_ptr.hpp>
#include <functional>
#include <algorithm>
#include <cmath>
#include <stdexcept>

void LightSystem::init(int viewW, int viewH) {
    m_vpW = viewW; m_vpH = viewH;

    // Occluder FBO: RGBA8 (red channel = occluded)
    m_occluderFBO.addColorAttachment(viewW, viewH, GL_RGBA8);
    m_occluderFBO.build();

    // Light accumulation FBO: HDR float
    m_lightFBO.addColorAttachment(viewW, viewH, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    m_lightFBO.build();

    // Per-light 1D shadow maps: 512x1 RGBA16F
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        glGenTextures(1, &m_shadowTex[i]);
        glBindTexture(GL_TEXTURE_2D, m_shadowTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, SHADOW_MAP_RES, 1, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &m_shadowFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, m_shadowTex[i], 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    buildFullscreenQuad();

    // Light quad (unit square, positioned via shader)
    float lightVerts[] = {
        0.f, 0.f,  0.f, 0.f,
        1.f, 0.f,  1.f, 0.f,
        1.f, 1.f,  1.f, 1.f,
        0.f, 1.f,  0.f, 1.f,
    };
    uint32_t lightIdx[] = {0,1,2, 0,2,3};
    glGenVertexArrays(1, &m_lightVAO);
    glGenBuffers(1, &m_lightVBO);
    glGenBuffers(1, &m_lightEBO);
    glBindVertexArray(m_lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lightVerts), lightVerts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_lightEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(lightIdx), lightIdx, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glBindVertexArray(0);
}

void LightSystem::buildFullscreenQuad() {
    float verts[] = {
        -1.f,-1.f, 0.f,0.f,
         1.f,-1.f, 1.f,0.f,
         1.f, 1.f, 1.f,1.f,
        -1.f, 1.f, 0.f,1.f,
    };
    uint32_t idx[] = {0,1,2, 0,2,3};
    glGenVertexArrays(1, &m_fsVAO);
    glGenBuffers(1, &m_fsVBO);
    glGenBuffers(1, &m_fsEBO);
    glBindVertexArray(m_fsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_fsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_fsEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glBindVertexArray(0);
}

void LightSystem::onResize(int w, int h) {
    m_vpW = w; m_vpH = h;
    m_occluderFBO.resize(w, h);
    m_lightFBO.resize(w, h);
}

void LightSystem::beginFrame(const Camera& cam, float sunlight) {
    m_camera   = &cam;
    m_sunlight = sunlight;
    m_lights.clear();
}

void LightSystem::addLight(const PointLight2D& light) {
    if ((int)m_lights.size() < MAX_LIGHTS)
        m_lights.push_back(light);
}

void LightSystem::compute(std::function<void()> drawOccluders) {
    occluderPass(drawOccluders);

    // Clear light FBO to ambient (sunlight gray)
    m_lightFBO.bind();
    glViewport(0, 0, m_vpW, m_vpH);
    float s = m_sunlight;
    glClearColor(s, s, s, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    m_lightFBO.unbind();

    // Per-light shadow + accumulation
    int count = (int)m_lights.size();
    for (int i = 0; i < count; ++i) {
        if (!m_lights[i].active) continue;
        if (m_lights[i].castShadow) shadowPass(m_lights[i], i);
        lightPass(m_lights[i], i);
    }
}

void LightSystem::occluderPass(std::function<void()> drawOccluders) {
    m_occluderFBO.bind();
    glViewport(0, 0, m_vpW, m_vpH);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);
    drawOccluders();
    glEnable(GL_BLEND);
    m_occluderFBO.unbind();
}

void LightSystem::shadowPass(const PointLight2D& light, int idx) {
    auto& slib = ShaderLibrary::instance();
    if (!slib.has("shadow_1d")) return;
    Shader& shader = slib.get("shadow_1d");

    // Convert light world position to screen UV
    glm::vec2 screenPos = m_camera->worldToScreen(light.worldPos);
    glm::vec2 lightUV   = { screenPos.x / m_vpW, screenPos.y / m_vpH };

    // Radius in UV space (use X axis)
    float halfViewW = m_camera->viewportWidth() / m_camera->zoom();
    float radiusUV  = light.radius / halfViewW * 0.5f;

    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO[idx]);
    glViewport(0, 0, SHADOW_MAP_RES, 1);
    glClearColor(1.f, 1.f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);

    shader.bind();
    shader.setVec2 ("uLightUV",       lightUV);
    shader.setFloat("uLightRadiusUV", radiusUV);
    shader.setInt  ("uOccluder",      0);
    m_occluderFBO.colorTexture(0).bind(0);

    glBindVertexArray(m_fsVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    shader.unbind();

    glEnable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LightSystem::lightPass(const PointLight2D& light, int idx) {
    auto& slib = ShaderLibrary::instance();
    if (!slib.has("light_point")) return;
    Shader& shader = slib.get("light_point");

    // Compute screen-space quad bounding box of the light influence circle
    glm::vec2 lightScreen = m_camera->worldToScreen(light.worldPos);
    float radPx = light.radius * m_camera->zoom();
    float x0 = (lightScreen.x - radPx) / m_vpW;
    float y0 = (lightScreen.y - radPx) / m_vpH;
    float x1 = (lightScreen.x + radPx) / m_vpW;
    float y1 = (lightScreen.y + radPx) / m_vpH;
    // Clamp to screen
    x0 = std::max(x0, 0.f); y0 = std::max(y0, 0.f);
    x1 = std::min(x1, 1.f); y1 = std::min(y1, 1.f);

    m_lightFBO.bind();
    glViewport(0, 0, m_vpW, m_vpH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE); // additive

    // Convert UV quad to NDC for the light quad shader
    float ndcX0 = x0 * 2.f - 1.f, ndcY0 = 1.f - y1 * 2.f;
    float ndcX1 = x1 * 2.f - 1.f, ndcY1 = 1.f - y0 * 2.f;

    glm::vec2 lightUV   = { lightScreen.x / m_vpW, lightScreen.y / m_vpH };
    float halfViewW = m_camera->viewportWidth() / m_camera->zoom();
    float radiusUV  = light.radius / halfViewW * 0.5f;

    // Update light quad vertices
    float lightVerts[] = {
        ndcX0, ndcY0,  x0, y0,
        ndcX1, ndcY0,  x1, y0,
        ndcX1, ndcY1,  x1, y1,
        ndcX0, ndcY1,  x0, y1,
    };
    glBindBuffer(GL_ARRAY_BUFFER, m_lightVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lightVerts), lightVerts);

    shader.bind();
    shader.setVec2 ("uLightUV",       lightUV);
    shader.setFloat("uLightRadiusUV", radiusUV);
    shader.setVec3 ("uLightColor",    light.color * light.intensity);
    shader.setInt  ("uShadowMap",     0);
    shader.setInt  ("uGBuffer",       1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_shadowTex[idx]);
    // GBuffer normals bound if available
    shader.setInt("uUseNormals", 0); // disabled unless GBuffer attached

    glBindVertexArray(m_lightVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    shader.unbind();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_lightFBO.unbind();
}

void LightSystem::bindLightTex(int slot) const {
    m_lightFBO.colorTexture(0).bind(slot);
}
