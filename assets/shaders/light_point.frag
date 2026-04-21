#version 450 core
// light_point.frag — Accumulates one point light into the HDR light buffer.
// Additively blended over the light FBO.
//
// Works in screen UV space [0..1] to match LightSystem.cpp's computation.
// uLightColor already contains color * intensity (pre-multiplied by C++).

in  vec2 vScreenUV;
out vec4 fragColor;

uniform sampler2D uShadowMap;    // 512×1 R16F: normalized hit distance per angle
uniform sampler2D uGBuffer;      // GBuffer normals (optional, slot 1)

uniform vec2  uLightUV;          // light center in screen UV [0..1]
uniform float uLightRadiusUV;    // light radius in UV space
uniform vec3  uLightColor;       // HDR colour * intensity (pre-multiplied)
uniform int   uUseNormals;       // 1 = sample GBuffer for normal-mapped diffuse

// ---- Soft shadow (PCF over 3 samples) ---------------------------------------
float sampleShadow(vec2 angleUV, float distN) {
    float s0 = texture(uShadowMap, vec2(angleUV.x,          0.5)).r;
    float s1 = texture(uShadowMap, vec2(angleUV.x + 0.002,  0.5)).r;
    float s2 = texture(uShadowMap, vec2(angleUV.x - 0.002,  0.5)).r;

    // Bias prevents self-shadowing from shadow map quantisation
    const float BIAS = 0.006;
    float lit = 0.0;
    lit += (distN < s0 + BIAS) ? 1.0 : 0.0;
    lit += (distN < s1 + BIAS) ? 1.0 : 0.0;
    lit += (distN < s2 + BIAS) ? 1.0 : 0.0;
    return lit * (1.0 / 3.0);
}

void main() {
    vec2 fragUV  = vScreenUV;
    vec2 toLight = fragUV - uLightUV;   // vector from fragment to light (UV space)
    float dist   = length(toLight);

    // Early-out: outside influence radius
    if (dist > uLightRadiusUV) { fragColor = vec4(0.0); return; }

    float distN = dist / uLightRadiusUV; // 0 at light, 1 at edge

    // ---- Shadow map lookup --------------------------------------------------
    float angle  = atan(toLight.y, toLight.x);
    float angleN = angle / 6.2831853 + 0.5; // 0..1
    float shadow = sampleShadow(vec2(angleN, 0.5), distN);

    // ---- Smooth attenuation (inverse-square feel, clamped at edge) ----------
    float atten = 1.0 - smoothstep(0.0, 1.0, distN);
    atten = atten * atten; // extra falloff punch

    // ---- Optional normal-map diffuse ----------------------------------------
    float diffuse = 1.0;
    if (uUseNormals != 0) {
        vec3 nEnc = texture(uGBuffer, fragUV).rgb;
        vec2 n2   = nEnc.rg * 2.0 - 1.0;
        float nz  = sqrt(max(0.0, 1.0 - dot(n2, n2)));
        vec3 norm = normalize(vec3(n2, nz));

        // Light direction in view-aligned 3D (z = depth bias for 2D feel)
        vec3 lightDir3 = normalize(vec3(-toLight, 0.4));
        diffuse = max(0.1, dot(norm, lightDir3));
    }

    // ---- Final contribution -------------------------------------------------
    vec3 light = uLightColor * atten * shadow * diffuse;

    // Soft glow falloff at the very center (avoids harsh hotspot)
    float centerGlow = 1.0 - smoothstep(0.0, 0.15, distN);
    light += uLightColor * centerGlow * 0.2;

    fragColor = vec4(light, 1.0);
}
