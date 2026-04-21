#version 450 core
// composite.frag — GBuffer combination pass.
// Combines: albedo × (accumulated_light + ambient) + albedo × emissive → HDR FBO.
// Input: GBuffer textures + light accumulation FBO.
// Output: HDR RGBA16F colour ready for PostProcess (bloom, tonemap, etc).

in  vec2 vScreenUV;
out vec4 fragColor;

uniform sampler2D uGAlbedo;       // GBuffer attachment 0: rgb=albedo, a=specular
uniform sampler2D uGNormEmissive; // GBuffer attachment 1: rg=normal enc, b=emissive
uniform sampler2D uLightBuffer;   // HDR light accumulation
uniform float     uSunlight;      // 0..1 ambient sunlight level (day/night)

void main() {
    vec2 uv = vScreenUV;

    vec3  albedo    = texture(uGAlbedo,       uv).rgb;
    float emissive  = texture(uGNormEmissive, uv).b;
    vec3  light     = texture(uLightBuffer,   uv).rgb;

    // Ambient colour: cool night → warm day
    vec3 ambient = mix(vec3(0.04, 0.05, 0.09), vec3(0.82, 0.79, 0.70), uSunlight);

    // HDR composite: diffuse + emissive glow
    vec3 hdr = albedo * (light + ambient)
             + albedo * emissive * 2.8;

    fragColor = vec4(hdr, 1.0);
}

