#version 450 core
// sprite.frag — Sprite to GBuffer

in vec2  vUV;
in vec2  vUVNorm;
in vec4  vColor;
in float vTexIdx;
in float vEmissive;

layout(location = 0) out vec4 gAlbedoSpec;
layout(location = 1) out vec3 gNormalEmissive;

uniform sampler2D uTextures[8];
uniform sampler2D uNormalMaps[8];

void main() {
    int  idx   = int(vTexIdx);
    vec4 texel;
    // Dynamic texture array sampling (unroll manually for GLSL compliance)
    if      (idx == 0) texel = texture(uTextures[0], vUV);
    else if (idx == 1) texel = texture(uTextures[1], vUV);
    else if (idx == 2) texel = texture(uTextures[2], vUV);
    else if (idx == 3) texel = texture(uTextures[3], vUV);
    else if (idx == 4) texel = texture(uTextures[4], vUV);
    else if (idx == 5) texel = texture(uTextures[5], vUV);
    else if (idx == 6) texel = texture(uTextures[6], vUV);
    else               texel = texture(uTextures[7], vUV);

    texel *= vColor;
    if (texel.a < 0.05) discard;

    // Normal map (packed as RGB, decode to XY)
    vec4 normSample;
    if      (idx == 0) normSample = texture(uNormalMaps[0], vUVNorm);
    else if (idx == 1) normSample = texture(uNormalMaps[1], vUVNorm);
    else if (idx == 2) normSample = texture(uNormalMaps[2], vUVNorm);
    else if (idx == 3) normSample = texture(uNormalMaps[3], vUVNorm);
    else               normSample = vec4(0.5, 0.5, 1.0, 1.0); // flat normal fallback

    vec2 encodedN = normSample.xy; // already 0..1 encoded

    float spec     = texel.a * 0.3;
    float emissive = vEmissive;

    gAlbedoSpec     = vec4(texel.rgb, spec);
    gNormalEmissive = vec3(encodedN, emissive);
}
