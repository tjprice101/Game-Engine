#version 450 core
// tonemap.frag — Final post-processing pass (PostProcess::apply)
// Matches PostProcess.cpp uniform names exactly.
//
//  1. CRT barrel distortion (uCRTCurvature)
//  2. Chromatic aberration  (uChromAb)
//  3. FXAA anti-aliasing    (uFXAASpanMax > 0)
//  4. Bloom                 (uBloom * uBloomIntensity)
//  5. Exposure              (uExposure)
//  6. ACES tone mapping
//  7. Gamma correction      (uGamma)
//  8. Brightness            (uBrightness)
//  9. Saturation + contrast (uSaturation, uContrast)
// 10. Lift / Gamma / Gain   (uLift, uGammaGrade, uGain)
// 11. CRT scanlines         (uCRTStrength)
// 12. Vignette              (uVignetteStrength, uVignetteSoft)
// 13. Subtle film grain

in  vec2 vScreenUV;
out vec4 fragColor;

uniform sampler2D uScene;           // composited HDR input
uniform sampler2D uBloom;           // dual-Kawase bloom texture

uniform float uExposure;            // default 1.0
uniform float uGamma;               // default 2.2
uniform float uBloomIntensity;      // default 0.04
uniform float uSaturation;          // default 1.0
uniform float uContrast;            // default 1.0
uniform float uBrightness;          // default 0.0
uniform vec3  uLift;                // default vec3(0)
uniform vec3  uGammaGrade;          // per-channel gamma, default vec3(1)
uniform vec3  uGain;                // per-channel gain, default vec3(1)

uniform float uChromAb;             // chromatic aberration, 0 = off
uniform float uCRTStrength;         // 0 = off, 1 = full scanlines
uniform float uCRTCurvature;        // barrel distortion, 0 = off
uniform float uVignetteStrength;    // 0..1
uniform float uVignetteSoft;        // softness radius, default 0.45
uniform float uFXAASpanMax;         // 0 = FXAA off, else span max (8.0 typical)
uniform vec2  uResolution;          // screen size in pixels

// ---- ACES tone map ----------------------------------------------------------
vec3 ACES(vec3 x) {
    // Narkowicz 2015 — "ACES Filmic Tone Mapping Curve"
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// ---- Pseudo-random hash (for grain) ----------------------------------------
float hash(vec2 p) {
    p = fract(p * vec2(443.897, 441.423));
    p += dot(p, p + 19.19);
    return fract(p.x * p.y);
}

// ---- FXAA (Lottes simplified) -----------------------------------------------
vec3 fxaa(sampler2D tex, vec2 uv, vec2 ts) {
    vec3 rgbNW = texture(tex, uv + vec2(-1,-1)*ts).rgb;
    vec3 rgbNE = texture(tex, uv + vec2( 1,-1)*ts).rgb;
    vec3 rgbSW = texture(tex, uv + vec2(-1, 1)*ts).rgb;
    vec3 rgbSE = texture(tex, uv + vec2( 1, 1)*ts).rgb;
    vec3 rgbM  = texture(tex, uv).rgb;

    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma), lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma), lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);
    float lumaMin = min(lumaM, min(min(lumaNW,lumaNE), min(lumaSW,lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW,lumaNE), max(lumaSW,lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW+lumaNE) - (lumaSW+lumaSE));
    dir.y =  ((lumaNW+lumaSW) - (lumaNE+lumaSE));
    float dirReduce = max((lumaNW+lumaNE+lumaSW+lumaSE)*0.03125, 0.0078125);
    float rcp = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcp, vec2(-uFXAASpanMax), vec2(uFXAASpanMax)) * ts;

    vec3 rgbA = 0.5  * (texture(tex, uv+dir*(1.0/3.0-0.5)).rgb
                       + texture(tex, uv+dir*(2.0/3.0-0.5)).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25*(texture(tex, uv+dir*-0.5).rgb
                                  + texture(tex, uv+dir* 0.5).rgb);
    float lumaB = dot(rgbB, luma);
    return (lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB;
}

void main() {
    vec2 uv = vScreenUV;
    vec2 ts = 1.0 / uResolution;

    // ---- 1. CRT barrel distortion -------------------------------------------
    if (uCRTCurvature > 0.0) {
        vec2 cc = uv * 2.0 - 1.0;
        cc *= 1.0 + dot(cc, cc) * uCRTCurvature * 0.05;
        uv = cc * 0.5 + 0.5;
        if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))) {
            fragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
    }

    // ---- 2. Chromatic aberration --------------------------------------------
    vec3 hdrColor;
    if (uChromAb > 0.0) {
        vec2 aberr = normalize(uv - 0.5) * uChromAb;
        hdrColor.r = texture(uScene, uv + aberr).r;
        hdrColor.g = texture(uScene, uv        ).g;
        hdrColor.b = texture(uScene, uv - aberr).b;
    } else {
        hdrColor = texture(uScene, uv).rgb;
    }

    // ---- 3. FXAA (pre-tonemapping, good enough for 2D) ---------------------
    if (uFXAASpanMax > 0.0) {
        hdrColor = fxaa(uScene, uv, ts);
    }

    // ---- 4. Bloom -----------------------------------------------------------
    hdrColor += texture(uBloom, uv).rgb * uBloomIntensity;

    // ---- 5. Exposure --------------------------------------------------------
    hdrColor *= uExposure;

    // ---- 6. ACES tone mapping -----------------------------------------------
    vec3 ldrColor = ACES(hdrColor);

    // ---- 7. Gamma correction ------------------------------------------------
    float g = max(uGamma, 0.001);
    ldrColor = pow(max(ldrColor, vec3(0.0)), vec3(1.0 / g));

    // ---- 8. Brightness ------------------------------------------------------
    ldrColor += uBrightness;

    // ---- 9. Saturation + contrast -------------------------------------------
    float grey = dot(ldrColor, vec3(0.2126, 0.7152, 0.0722));
    ldrColor = mix(vec3(grey), ldrColor, uSaturation);
    ldrColor = (ldrColor - 0.5) * uContrast + 0.5;

    // ---- 10. Lift / Gamma / Gain color grade --------------------------------
    ldrColor = pow(max(ldrColor * uGain + uLift, vec3(0.0)),
                   1.0 / max(uGammaGrade, vec3(0.001)));
    ldrColor = clamp(ldrColor, 0.0, 1.0);

    // ---- 11. CRT scanlines --------------------------------------------------
    if (uCRTStrength > 0.0) {
        // Horizontal scanlines
        float scanline = sin(uv.y * uResolution.y * 3.14159) * 0.5 + 0.5;
        scanline = pow(scanline, 0.4) * 0.18 * uCRTStrength;
        ldrColor *= 1.0 - scanline;
        // Subtle vertical sub-pixel grid
        float pixelGrid = sin(uv.x * uResolution.x * 1.777 * 3.14159) * 0.5 + 0.5;
        ldrColor *= 1.0 - pixelGrid * 0.05 * uCRTStrength;
        // RGB phosphor tint
        vec3 phosphor = vec3(1.02, 0.98, 1.01);
        ldrColor *= mix(vec3(1.0), phosphor, uCRTStrength * 0.4);
    }

    // ---- 12. Vignette -------------------------------------------------------
    if (uVignetteStrength > 0.0) {
        vec2 vigUV = uv * 2.0 - 1.0;
        float vig  = dot(vigUV, vigUV);
        vig = smoothstep(1.0 - uVignetteSoft, 1.0 + uVignetteSoft, vig);
        ldrColor  *= 1.0 - vig * uVignetteStrength;
    }

    // ---- 13. Subtle film grain ----------------------------------------------
    float grain = (hash(uv * uResolution) - 0.5) * 0.022;
    ldrColor += vec3(grain);

    fragColor = vec4(clamp(ldrColor, 0.0, 1.0), 1.0);
}
