#version 450 core
// shadow_1d.frag — Generates a 512×1 1D polar shadow map for one point light.
//
// Column i maps to angle θ = (i/512) × 2π.
// Stored value = normalised distance to first occluder hit (0..1 relative to uLightRadiusUV).
// Ray-marches the occluder FBO in UV space.

in  vec2  vPos;
out vec4  fragColor;

uniform sampler2D uOccluder;      // occluder FBO texture (screen UV space)
uniform vec2      uLightUV;       // light position in screen UV [0..1]
uniform float     uLightRadiusUV; // light influence radius in UV space

void main() {
    // vPos is NDC -1..1; map x → [0..1] = column/512
    float t     = vPos.x * 0.5 + 0.5;
    float angle = t * 6.2831853; // 0..2π
    vec2  dir   = vec2(cos(angle), sin(angle));

    float shadowDist = 1.0; // default: fully lit

    // March 128 steps from light center to edge of radius
    const int STEPS = 128;
    for (int i = 1; i <= STEPS; ++i) {
        float frac = float(i) / float(STEPS);
        vec2  uv   = uLightUV + dir * frac * uLightRadiusUV;

        // Out-of-screen = treat as blocker (world edge)
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            shadowDist = frac;
            break;
        }

        // Occluder FBO: red channel > 0.5 means solid
        if (texture(uOccluder, uv).r > 0.5) {
            shadowDist = frac;
            break;
        }
    }

    // Store normalised hit distance in red channel
    fragColor = vec4(shadowDist, 0.0, 0.0, 1.0);
}
