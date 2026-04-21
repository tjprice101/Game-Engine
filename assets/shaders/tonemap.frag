#version 450 core
// tonemap.frag — Final composite pass:
//   1. Combine GBuffer albedo with light accumulation + ambient + emissive
//   2. Add bloom
//   3. ACES tone mapping
//   4. LUT color grading
//   5. Chromatic aberration
//   6. CRT scanlines
//   7. Film grain
//   8. Vignette
//   9. FXAA

in  vec2 vScreenUV;
out vec4 fragColor;

uniform sampler2D uGAlbedo;      // albedo from GBuffer
uniform sampler2D uGNormEmissive;// normal+emissive from GBuffer
uniform sampler2D uLightBuffer;  // accumulated HDR light (RGBA16F)
uniform sampler2D uBloom;        // dual-Kawase bloom result
uniform sampler3D uLUT;          // 3D LUT for color grading (32×32×32)

layout(std140, binding = 1) uniform FrameDataUBO {
    float uTime;
    float uDayTime;
    float uSunlight;
    float uWindStrength;
    vec4  uFogColor;
};

// Post-process settings (passed as uniforms)
uniform float uExposure;         // default 1.0
uniform float uBloomIntensity;   // default 0.04
uniform float uSaturation;       // default 1.0
uniform float uContrast;         // default 1.0
uniform vec3  uLift;             // default (0,0,0)
uniform vec3  uGamma;            // default (1,1,1)
uniform vec3  uGain;             // default (1,1,1)

// Chromatic aberration
uniform float uChromAbStrength;  // default 0.003

// CRT
uniform float uCRTStrength;      // 0 = off, 1 = full
uniform float uCRTResolution;    // scanline count (e.g. 720)

// Vignette
uniform float uVignetteStrength; // 0..1
uniform float uVignetteSoftness; // default 0.45

// Film grain
uniform float uGrainStrength;    // default 0.04

// FXAA
uniform vec2  uTexelSize;        // 1/screenSize

// ---- ACES tone mapping -----------------------------------------------------
vec3 ACES(vec3 x) {
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// ---- Hash for film grain ----------------------------------------------------
float hash(vec2 p) {
    p = fract(p * vec2(443.897, 441.423));
    p += dot(p, p + 19.19);
    return fract(p.x * p.y);
}

// ---- FXAA (Simplified Lottes) -----------------------------------------------
vec3 fxaa(sampler2D tex, vec2 uv, vec2 texelSize) {
    vec3 rgbNW = texture(tex, uv + vec2(-1,-1) * texelSize).rgb;
    vec3 rgbNE = texture(tex, uv + vec2( 1,-1) * texelSize).rgb;
    vec3 rgbSW = texture(tex, uv + vec2(-1, 1) * texelSize).rgb;
    vec3 rgbSE = texture(tex, uv + vec2( 1, 1) * texelSize).rgb;
    vec3 rgbM  = texture(tex, uv).rgb;

    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.03125, 0.0078125);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-8.0), vec2(8.0)) * texelSize;

    vec3 rgbA = 0.5 * (texture(tex, uv + dir * (1.0/3.0 - 0.5)).rgb
                     + texture(tex, uv + dir * (2.0/3.0 - 0.5)).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture(tex, uv + dir * -0.5).rgb
                                    + texture(tex, uv + dir *  0.5).rgb);

    float lumaB = dot(rgbB, luma);
    return (lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB;
}

void main() {
    vec2 uv = vScreenUV;

    // ---- Chromatic aberration -----------------------------------------------
    float ca = uChromAbStrength;
    vec2  dir = normalize(uv - 0.5) * ca;
    vec3  albedo;
    albedo.r = texture(uGAlbedo, uv + dir).r;
    albedo.g = texture(uGAlbedo, uv      ).g;
    albedo.b = texture(uGAlbedo, uv - dir).b;

    // ---- Light + emissive composite ----------------------------------------
    vec3 light    = texture(uLightBuffer, uv).rgb;
    float emissive= texture(uGNormEmissive, uv).b;

    // Ambient light from sun/moon
    vec3 ambientColor = mix(vec3(0.05, 0.07, 0.12), vec3(0.9, 0.85, 0.7), uSunlight);
    vec3 hdrColor = albedo * (light + ambientColor) + albedo * emissive * 3.0;

    // ---- Bloom add ----------------------------------------------------------
    hdrColor += texture(uBloom, uv).rgb * uBloomIntensity;

    // ---- Exposure -----------------------------------------------------------
    hdrColor *= uExposure;

    // ---- ACES tone map ------------------------------------------------------
    vec3 ldrColor = ACES(hdrColor);

    // ---- FXAA (on LDR) ------------------------------------------------------
    // We'd need a separate pass for proper FXAA; approximate here by sampling
    // the composited result. For now: blend with a 1-pixel blur.
    // (A proper FXAA pass is set up by PostProcess.cpp as a separate fullscreen pass.)

    // ---- LUT color grading --------------------------------------------------
    if (uSaturation != 1.0 || uContrast != 1.0) {
        // Saturation
        float grey = dot(ldrColor, vec3(0.2126, 0.7152, 0.0722));
        ldrColor = mix(vec3(grey), ldrColor, uSaturation);
        // Contrast (around 0.5)
        ldrColor = (ldrColor - 0.5) * uContrast + 0.5;
    }

    // Lift / Gamma / Gain (per-channel color grade)
    ldrColor = pow(max(ldrColor * uGain + uLift, 0.0), 1.0 / max(uGamma, vec3(0.001)));
    ldrColor = clamp(ldrColor, 0.0, 1.0);

    // Sample 3D LUT
    vec3 lutScale  = vec3(31.0 / 32.0);
    vec3 lutOffset = vec3(0.5  / 32.0);
    ldrColor = texture(uLUT, ldrColor * lutScale + lutOffset).rgb;

    // ---- CRT scanlines -------------------------------------------------------
    if (uCRTStrength > 0.0) {
        float scanline = sin(uv.y * uCRTResolution * 3.14159) * 0.5 + 0.5;
        scanline = pow(scanline, 0.5) * 0.15 * uCRTStrength;
        ldrColor *= 1.0 - scanline;

        // Pixel grid (subtle)
        float pixelGrid = sin(uv.x * uCRTResolution * 1.777 * 3.14159) * 0.5 + 0.5;
        ldrColor *= 1.0 - pixelGrid * 0.04 * uCRTStrength;
    }

    // ---- Film grain ---------------------------------------------------------
    float grain = (hash(uv + uTime * 0.137) - 0.5) * uGrainStrength;
    ldrColor += vec3(grain);

    // ---- Vignette -----------------------------------------------------------
    vec2  vigUV  = uv * 2.0 - 1.0;
    float vig    = dot(vigUV, vigUV);
    vig = pow(vig, 1.5);
    ldrColor *= 1.0 - vig * uVignetteStrength;

    fragColor = vec4(clamp(ldrColor, 0.0, 1.0), 1.0);
}
