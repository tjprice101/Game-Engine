#include "PostProcess.h"
#include "renderer/ShaderLibrary.h"
#include <stdexcept>
#include <algorithm>

void PostProcess::buildFullscreenQuad() {
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

void PostProcess::buildBloomMips(int w, int h) {
    for (int i = 0; i < BLOOM_LEVELS; ++i) {
        int mw = std::max(1, w >> (i+1));
        int mh = std::max(1, h >> (i+1));
        // Down
        m_bloomMips[i] = RenderTarget{};
        m_bloomMips[i].addColorAttachment(mw, mh, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR);
        m_bloomMips[i].build();
        // Up
        int ui = BLOOM_LEVELS + i;
        m_bloomMips[ui] = RenderTarget{};
        m_bloomMips[ui].addColorAttachment(mw, mh, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR);
        m_bloomMips[ui].build();
    }
}

void PostProcess::init(int w, int h) {
    m_vpW = w; m_vpH = h;
    buildFullscreenQuad();

    for (int i = 0; i < 2; ++i) {
        m_pingpong[i].addColorAttachment(w, h, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR);
        m_pingpong[i].build();
    }

    buildBloomMips(w, h);
}

void PostProcess::onResize(int w, int h) {
    m_vpW = w; m_vpH = h;
    for (auto& rt : m_pingpong) rt.resize(w, h);
    buildBloomMips(w, h);
}

void PostProcess::apply(GLuint sourceHDR, const PostProcessSettings& cfg) {
    auto& slib = ShaderLibrary::instance();

    GLuint resultTex = sourceHDR;

    // ---- Bloom --------------------------------------------------------------
    GLuint bloomTex = 0;
    if (cfg.bloom.enabled && slib.has("bloom_threshold") && slib.has("bloom_blur")) {
        Shader& thresh = slib.get("bloom_threshold");
        Shader& blur   = slib.get("bloom_blur");

        // Threshold pass → mip 0 down
        m_bloomMips[0].bind();
        glViewport(0, 0, m_bloomMips[0].width(), m_bloomMips[0].height());
        glClear(GL_COLOR_BUFFER_BIT);
        thresh.bind();
        thresh.setInt  ("uScene",     0);
        thresh.setFloat("uThreshold", cfg.bloom.threshold);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sourceHDR);
        glBindVertexArray(m_fsVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        thresh.unbind();
        m_bloomMips[0].unbind();

        // Downsample chain
        for (int i = 1; i < cfg.bloom.passes && i < BLOOM_LEVELS; ++i) {
            m_bloomMips[i].bind();
            glViewport(0, 0, m_bloomMips[i].width(), m_bloomMips[i].height());
            glClear(GL_COLOR_BUFFER_BIT);
            blur.bind();
            blur.setInt  ("uTex",        0);
            blur.setInt  ("uDown",       1);
            blur.setFloat("uSampleStep", 0.5f);
            blur.setVec2 ("uTexelSize",  {1.f/m_bloomMips[i-1].width(),
                                          1.f/m_bloomMips[i-1].height()});
            m_bloomMips[i-1].bindColorTex(0, 0);
            glBindVertexArray(m_fsVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            blur.unbind();
            m_bloomMips[i].unbind();
        }

        // Upsample chain (starting from deepest mip)
        int maxPass = std::min(cfg.bloom.passes, BLOOM_LEVELS);
        for (int i = maxPass - 2; i >= 0; --i) {
            int upIdx = BLOOM_LEVELS + i;
            int srcDown = i + 1;
            m_bloomMips[upIdx].bind();
            glViewport(0, 0, m_bloomMips[upIdx].width(), m_bloomMips[upIdx].height());
            glClear(GL_COLOR_BUFFER_BIT);
            blur.bind();
            blur.setInt  ("uTex",        0);
            blur.setInt  ("uDown",       0);
            blur.setFloat("uSampleStep", 1.5f);
            blur.setVec2 ("uTexelSize",  {1.f/m_bloomMips[srcDown].width(),
                                          1.f/m_bloomMips[srcDown].height()});
            m_bloomMips[srcDown].bindColorTex(0, 0);
            glBindVertexArray(m_fsVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            blur.unbind();
            m_bloomMips[upIdx].unbind();
        }

        bloomTex = m_bloomMips[BLOOM_LEVELS].colorTexture(0).id();
    }

    // ---- Final composite: tone map + color grade + effects + output ---------
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_vpW, m_vpH);
    glDisable(GL_BLEND);
    glClear(GL_COLOR_BUFFER_BIT);

    if (slib.has("tonemap")) {
        Shader& tm = slib.get("tonemap");
        tm.bind();

        // Scene HDR
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, resultTex);
        tm.setInt("uScene", 0);

        // Bloom
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bloomTex ? bloomTex : 0);
        tm.setInt("uBloom", 1);

        // Settings
        tm.setFloat("uExposure",       cfg.toneMap.exposure);
        tm.setFloat("uGamma",          cfg.gamma);
        tm.setFloat("uBloomIntensity", cfg.bloom.enabled ? cfg.bloom.intensity : 0.f);
        tm.setVec3 ("uBloomTint",      cfg.bloom.enabled ? cfg.bloom.tint : glm::vec3{1.f});
        tm.setFloat("uSaturation",     cfg.colorGrade.saturation);
        tm.setFloat("uContrast",       cfg.colorGrade.contrast);
        tm.setVec3 ("uLift",           cfg.colorGrade.lift);
        tm.setVec3 ("uGammaGrade",     cfg.colorGrade.gamma);
        tm.setVec3 ("uGain",           cfg.colorGrade.gain);
        tm.setFloat("uBrightness",     cfg.colorGrade.brightness);
        tm.setFloat("uChromAb",        cfg.chromAb.enabled ? cfg.chromAb.strength : 0.f);
        tm.setFloat("uCRTStrength",    cfg.crt.enabled ? cfg.crt.scanlineAlpha : 0.f);
        tm.setFloat("uCRTCurvature",   cfg.crt.curvature);
        tm.setFloat("uVignetteStrength", cfg.vignette.enabled ? cfg.vignette.strength : 0.f);
        tm.setFloat("uVignetteSoft",   cfg.vignette.softness);
        tm.setFloat("uFXAASpanMax",    cfg.fxaa.enabled ? cfg.fxaa.spanMax : 0.f);
        tm.setVec2 ("uResolution",     { (float)m_vpW, (float)m_vpH });
        tm.setVec4 ("uFlashColor",     cfg.flash.color);
        tm.setFloat("uTime",           cfg.time);
        tm.setInt  ("uDistortType",    cfg.distort.strength > 0.f ? cfg.distort.type : 0);
        tm.setFloat("uDistortStr",     cfg.distort.strength);
        tm.setVec2 ("uDistortCenter",  cfg.distort.center);


        glBindVertexArray(m_fsVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        tm.unbind();
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
