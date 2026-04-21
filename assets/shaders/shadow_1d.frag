#version 450 core
// shadow_1d.frag — Generates a 512×1 1D polar shadow map for one point light.
//
// The output texture is 512×1 RGBA16F:
//   column i = angle θ = (i/512) * 2π
//   stored value = normalized distance to first occluder hit (0..1 in light radius)
//
// The shader is run as a fullscreen pass over a 512×1 render target.

in  vec2  vPos;
out vec4  fragColor;

uniform sampler2D uOccluder;    // the occluder FBO texture (screen-space)
uniform vec2      uLightUV;     // light position in occluder UV space [0..1]
uniform float     uLightRadius; // world pixels
uniform vec2      uOccluderSize;// occluder texture pixel dims

void main() {
    // vPos is NDC -1..1; map x to [0..1] = column/512
    float t = vPos.x * 0.5 + 0.5;  // 0..1

    float angle = t * 6.2831853; // 0..2π
    vec2  dir   = vec2(cos(angle), sin(angle));

    float shadowDist = 1.0; // assume fully lit (no hit)

    // Ray-march in UV space
    // Step size = 1 pixel in occluder, expressed as UV fraction
    float stepUV = 1.5 / min(uOccluderSize.x, uOccluderSize.y);
    float maxSteps = uLightRadius / (stepUV * min(uOccluderSize.x, uOccluderSize.y));
    maxSteps = clamp(maxSteps, 32.0, 512.0);

    for (float i = 0.0; i < maxSteps; i += 1.0) {
        float dist = i / maxSteps;
        vec2 uv = uLightUV + dir * dist * uLightRadius / uOccluderSize;

        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            shadowDist = dist;
            break;
        }

        float occ = texture(uOccluder, uv).r;
        if (occ > 0.5) {
            shadowDist = dist;
            break;
        }
    }

    fragColor = vec4(shadowDist, 0.0, 0.0, 1.0);
}
