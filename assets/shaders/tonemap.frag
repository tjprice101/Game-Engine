#version 450 core
// tonemap.frag — Final post-processing pass.
//
// Order of operations:
//  1.  CRT barrel distortion     (uCRTCurvature)
//  2.  Screen warp distortion    (uDistortType / uDistortStr / uDistortCenter)
//  3.  Chromatic aberration      (uChromAb)
//  4.  FXAA anti-aliasing        (uFXAASpanMax > 0)
//  5.  Bloom + tinted bloom      (uBloom * uBloomIntensity * uBloomTint)
//  6.  Exposure                  (uExposure)
//  7.  ACES tone mapping
//  8.  Gamma correction          (uGamma)
//  9.  Brightness                (uBrightness)
//  10. Saturation + contrast     (uSaturation, uContrast)
//  11. Lift / Gamma / Gain       (uLift, uGammaGrade, uGain)
//  12. CRT scanlines             (uCRTStrength)
//  13. Vignette                  (uVignetteStrength, uVignetteSoft)
//  14. Screen flash overlay      (uFlashColor)
//  15. Film grain

in  vec2 vScreenUV;
out vec4 fragColor;

uniform sampler2D uScene;
uniform sampler2D uBloom;

uniform float uExposure;
uniform float uGamma;
uniform float uBloomIntensity;
uniform float uSaturation;
uniform float uContrast;
uniform float uBrightness;
uniform vec3  uLift;
uniform vec3  uGammaGrade;
uniform vec3  uGain;

uniform float uChromAb;
uniform float uCRTStrength;
uniform float uCRTCurvature;
uniform float uVignetteStrength;
uniform float uVignetteSoft;
uniform float uFXAASpanMax;
uniform vec2  uResolution;

// ---- New uniforms -----------------------------------------------------------
uniform vec3  uBloomTint;         // per-theme bloom color tint (default vec3(1))
uniform vec4  uFlashColor;        // screen flash overlay (RGBA, alpha = strength)
uniform float uTime;              // cumulative time for animated distortions

// Screen distortion: 0=none, 1=ripple, 2=shatter, 3=warp, 4=pulse, 5=tear
uniform int   uDistortType;
uniform float uDistortStr;
uniform vec2  uDistortCenter;     // UV space (default 0.5, 0.5)
// ---------------------------------------------------------------------------

// ---- ACES tone map ----------------------------------------------------------
vec3 ACES(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

// ---- Pseudo-random hash (grain) --------------------------------------------
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
    vec3 luma  = vec3(0.299, 0.587, 0.114);
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
    dir = clamp(dir*rcp, vec2(-uFXAASpanMax), vec2(uFXAASpanMax)) * ts;
    vec3 rgbA = 0.5*(texture(tex, uv+dir*(1.0/3.0-0.5)).rgb
                   + texture(tex, uv+dir*(2.0/3.0-0.5)).rgb);
    vec3 rgbB = rgbA*0.5 + 0.25*(texture(tex, uv+dir*-0.5).rgb
                                + texture(tex, uv+dir* 0.5).rgb);
    float lumaB = dot(rgbB, luma);
    return (lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB;
}

// ---- Screen distortion ------------------------------------------------------
vec2 applyDistortion(vec2 uv) {
    if (uDistortType == 0 || uDistortStr <= 0.0) return uv;

    vec2  delta  = uv - uDistortCenter;
    float dist   = length(delta);
    vec2  offset = vec2(0.0);

    if (uDistortType == 1) {
        // Ripple: concentric wave from center
        float wave = sin(dist * 40.0 - uTime * 12.0) * uDistortStr;
        offset = normalize(delta + vec2(0.001)) * wave;

    } else if (uDistortType == 2) {
        // Shatter: radial crack pattern
        float angle = atan(delta.y, delta.x);
        float crack = abs(sin(angle * 6.0)) * (1.0 - dist) * uDistortStr * 0.5;
        offset = normalize(delta + vec2(0.001)) * crack;

    } else if (uDistortType == 3) {
        // Warp: smooth barrel/pinch
        offset = delta * dist * uDistortStr * 0.8;

    } else if (uDistortType == 4) {
        // Pulse: uniform radial push-out
        float pulse = (1.0 - smoothstep(0.0, 0.5, dist)) * uDistortStr;
        offset = normalize(delta + vec2(0.001)) * pulse;

    } else if (uDistortType == 5) {
        // Tear: horizontal glitch bands
        float band = sin(uv.y * uResolution.y * 0.08 + uTime * 20.0);
        offset.x = band * uDistortStr * (1.0 - smoothstep(0.0, 0.6, dist));
    }

    return clamp(uv + offset, vec2(0.001), vec2(0.999));
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

    // ---- 2. Screen warp distortion ------------------------------------------
    uv = applyDistortion(uv);

    // ---- 3. Chromatic aberration --------------------------------------------
    vec3 hdrColor;
    if (uChromAb > 0.0) {
        vec2 aberr = normalize(uv - 0.5) * uChromAb;
        hdrColor.r = texture(uScene, uv + aberr).r;
        hdrColor.g = texture(uScene, uv        ).g;
        hdrColor.b = texture(uScene, uv - aberr).b;
    } else {
        hdrColor = texture(uScene, uv).rgb;
    }

    // ---- 4. FXAA (pre-tonemapping) ------------------------------------------
    if (uFXAASpanMax > 0.0) {
        hdrColor = fxaa(uScene, uv, ts);
    }

    // ---- 5. Bloom (with per-theme tint) -------------------------------------
    vec3 bloomSample = texture(uBloom, uv).rgb;
    hdrColor += bloomSample * uBloomTint * uBloomIntensity;

    // ---- 6. Exposure --------------------------------------------------------
    hdrColor *= uExposure;

    // ---- 7. ACES tone mapping -----------------------------------------------
    vec3 ldrColor = ACES(hdrColor);

    // ---- 8. Gamma correction ------------------------------------------------
    float g = max(uGamma, 0.001);
    ldrColor = pow(max(ldrColor, vec3(0.0)), vec3(1.0 / g));

    // ---- 9. Brightness ------------------------------------------------------
    ldrColor += uBrightness;

    // ---- 10. Saturation + contrast ------------------------------------------
    float grey = dot(ldrColor, vec3(0.2126, 0.7152, 0.0722));
    ldrColor = mix(vec3(grey), ldrColor, uSaturation);
    ldrColor = (ldrColor - 0.5) * uContrast + 0.5;

    // ---- 11. Lift / Gamma / Gain color grade --------------------------------
    ldrColor = pow(max(ldrColor * uGain + uLift, vec3(0.0)),
                   1.0 / max(uGammaGrade, vec3(0.001)));
    ldrColor = clamp(ldrColor, 0.0, 1.0);

    // ---- 12. CRT scanlines --------------------------------------------------
    if (uCRTStrength > 0.0) {
        float scanline = sin(uv.y * uResolution.y * 3.14159) * 0.5 + 0.5;
        scanline = pow(scanline, 0.4) * 0.18 * uCRTStrength;
        ldrColor *= 1.0 - scanline;
        float pixelGrid = sin(uv.x * uResolution.x * 1.777 * 3.14159) * 0.5 + 0.5;
        ldrColor *= 1.0 - pixelGrid * 0.05 * uCRTStrength;
        vec3 phosphor = vec3(1.02, 0.98, 1.01);
        ldrColor *= mix(vec3(1.0), phosphor, uCRTStrength * 0.4);
    }

    // ---- 13. Vignette -------------------------------------------------------
    if (uVignetteStrength > 0.0) {
        vec2  vigUV = uv * 2.0 - 1.0;
        float vig   = dot(vigUV, vigUV);
        vig = smoothstep(1.0 - uVignetteSoft, 1.0 + uVignetteSoft, vig);
        ldrColor *= 1.0 - vig * uVignetteStrength;
    }

    // ---- 14. Screen flash overlay -------------------------------------------
    if (uFlashColor.a > 0.001) {
        ldrColor = mix(ldrColor, uFlashColor.rgb, uFlashColor.a);
    }

    // ---- 15. Film grain -----------------------------------------------------
    float grain = (hash(uv * uResolution) - 0.5) * 0.022;
    ldrColor += vec3(grain);

    fragColor = vec4(clamp(ldrColor, 0.0, 1.0), 1.0);
}
