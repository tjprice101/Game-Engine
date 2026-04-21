#version 450 core

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uScene;
uniform float     uAmbient;   // 0..1 minimum light everywhere (sky / time-of-day)
uniform float     uGamma;     // typically 2.2
uniform float     uContrast;  // 1.0 = neutral
uniform float     uSaturation;// 1.0 = neutral

// Vignette
uniform float uVignetteStrength; // 0 = off, 0.5 = subtle

vec3 adjustSaturation(vec3 color, float sat) {
    float lum = dot(color, vec3(0.299, 0.587, 0.114));
    return mix(vec3(lum), color, sat);
}

void main() {
    vec3 sceneColor = texture(uScene, vTexCoord).rgb;

    // Contrast (S-curve approximation)
    sceneColor = ((sceneColor - 0.5) * uContrast) + 0.5;
    sceneColor = clamp(sceneColor, 0.0, 1.0);

    // Saturation
    sceneColor = adjustSaturation(sceneColor, uSaturation);

    // Vignette
    vec2 uv = vTexCoord * 2.0 - 1.0;
    float vignette = 1.0 - uVignetteStrength * dot(uv, uv);
    sceneColor *= clamp(vignette, 0.0, 1.0);

    // Gamma correction
    sceneColor = pow(sceneColor, vec3(1.0 / uGamma));

    fragColor = vec4(sceneColor, 1.0);
}
