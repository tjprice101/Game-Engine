#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include "renderer/Shader.h"
#include "renderer/RenderTarget.h"

// ---- Post-Processing Stack -------------------------------------------------
// Each effect is individually toggleable. Applied in order:
//   Bloom → Tone Map → Color Grade → Chromatic Aberration → CRT → Vignette → FXAA

struct BloomSettings {
    bool  enabled       = true;
    float threshold     = 0.7f;  // luminance threshold
    float intensity     = 0.35f; // bloom strength
    int   passes        = 6;     // downsample/upsample depth
};

struct ToneMapSettings {
    bool  enabled   = true;
    float exposure  = 1.0f;  // pre-tone-map exposure multiplier
    // ACES curve is hardcoded; exposure scales the input
};

struct ColorGradeSettings {
    bool      enabled    = true;
    float     saturation = 1.0f;
    float     contrast   = 1.0f;
    glm::vec3 lift       = {0.f, 0.f, 0.f};    // shadows color shift
    glm::vec3 gamma      = {1.f, 1.f, 1.f};    // midtones
    glm::vec3 gain       = {1.f, 1.f, 1.f};    // highlights
    float     brightness = 0.0f;               // additive brightness
};

struct ChromAbSettings {
    bool  enabled   = false;
    float strength  = 0.003f;  // UV offset magnitude
};

struct CRTSettings {
    bool  enabled        = false;
    float scanlineAlpha  = 0.12f;   // darkness of scanlines
    float vignetteStrength= 0.4f;
    float curvature      = 0.05f;   // lens warp amount
};

struct VignetteSettings {
    bool  enabled   = true;
    float strength  = 0.30f;
    float softness  = 0.45f;
};

struct FXAASettings {
    bool  enabled   = true;
    float spanMax   = 8.f;
};

struct PostProcessSettings {
    BloomSettings     bloom;
    ToneMapSettings   toneMap;
    ColorGradeSettings colorGrade;
    ChromAbSettings   chromAb;
    CRTSettings       crt;
    VignetteSettings  vignette;
    FXAASettings      fxaa;
    float             gamma = 2.2f;
};

class PostProcess {
public:
    PostProcess()  = default;
    ~PostProcess() = default;

    PostProcess(const PostProcess&)            = delete;
    PostProcess& operator=(const PostProcess&) = delete;

    void init(int w, int h);
    void onResize(int w, int h);

    // Apply full post-process stack to `sourceHDR`, output to default FBO.
    // sourceHDR must be a RGBA16F texture containing the HDR composite.
    void apply(GLuint sourceHDR, const PostProcessSettings& cfg);

    PostProcessSettings& settings() { return m_settings; }

private:
    void bloomPass(GLuint src, const BloomSettings& cfg);
    void buildBloomMips(int w, int h);
    void buildFullscreenQuad();

    Shader* m_bloomThresh = nullptr;
    Shader* m_bloomBlur   = nullptr;
    Shader* m_tonemapFXAA = nullptr;

    // Bloom pyramid: 6 down + 6 up
    static constexpr int BLOOM_LEVELS = 6;
    RenderTarget m_bloomMips[BLOOM_LEVELS * 2]; // down[0..5] + up[0..5]

    RenderTarget m_pingpong[2]; // for intermediate passes
    int m_vpW = 0, m_vpH = 0;

    GLuint m_fsVAO = 0, m_fsVBO = 0, m_fsEBO = 0;
    PostProcessSettings m_settings;

    GLuint m_bloomResult = 0; // final bloom texture (owned by bloom mips)
};
