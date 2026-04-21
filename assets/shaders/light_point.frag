#version 450 core
// light_point.frag — Accumulates one point light into the HDR light buffer.
// Reads GBuffer (albedo/normal), shadow map, computes final lit colour.
// This pass is additively blended.

in  vec2 vScreenUV;
out vec4 fragColor;

uniform sampler2D uGAlbedoSpec;     // RGBA8: rgb=albedo, a=specular
uniform sampler2D uGNormalEmissive; // RGB16F: rg=normal encoded, b=emissive
uniform sampler2D uShadowMap;       // 512×1 R16F: distance to occluder by angle
uniform sampler2D uOccluder;        // for debug / edge softening

uniform vec2  uLightPosUV;    // light world pos mapped to screen UV
uniform vec3  uLightColor;    // HDR light colour (can be > 1)
uniform float uLightRadius;   // in screen pixels (post-projection)
uniform float uLightIntensity;

layout(std140, binding = 0) uniform CameraUBO {
    mat4  uViewProj;
    vec2  uCameraPos;
    vec2  uViewportSize;
    float uZoom;
    float _pad[3];
};

void main() {
    vec2 fragUV = vScreenUV;

    // ---- Albedo + specular --------------------------------------------------
    vec4 albedoSpec = texture(uGAlbedoSpec, fragUV);
    vec3 albedo     = albedoSpec.rgb;
    float specStr   = albedoSpec.a;

    // ---- Normal (decode XY, reconstruct Z) ----------------------------------
    vec3 nEnc = texture(uGNormalEmissive, fragUV);
    vec2 n2   = nEnc.rg * 2.0 - 1.0;
    float nz  = sqrt(max(0.0, 1.0 - dot(n2, n2)));
    vec3 norm = normalize(vec3(n2, nz));

    float emissive = nEnc.b;

    // ---- Fragment world position (recover from UV + camera) -----------------
    vec2 fragPos = (fragUV - 0.5) * uViewportSize / uZoom + uCameraPos;
    vec2 lightPosWorld = (uLightPosUV - 0.5) * uViewportSize / uZoom + uCameraPos;

    vec2 toLight     = fragPos - lightPosWorld;
    float dist       = length(toLight);
    if (dist > uLightRadius) { fragColor = vec4(0.0); return; }

    // ---- Shadow map lookup --------------------------------------------------
    float angle  = atan(toLight.y, toLight.x);   // -π..π
    float angleN = angle / 6.2831853 + 0.5;      // 0..1
    float shadowDist = texture(uShadowMap, vec2(angleN, 0.5)).r; // normalised 0..1

    // Hard shadow: is this fragment farther than the shadow hit?
    float occluderDist = shadowDist * uLightRadius;
    float shadow = (dist < occluderDist + 1.5) ? 1.0 : 0.0;

    // Soft penumbra (PCF-like, 2-tap)
    float shadow2 = (dist < texture(uShadowMap, vec2(angleN + 0.002, 0.5)).r * uLightRadius + 1.5) ? 1.0 : 0.0;
    shadow = mix(shadow, shadow2, 0.5);

    // ---- Attenuation --------------------------------------------------------
    // Inverse-square with smooth falloff at edge
    float atten = 1.0 - smoothstep(uLightRadius * 0.0, uLightRadius, dist);
    atten = atten * atten; // steeper falloff

    // ---- Normal-mapped diffuse ----------------------------------------------
    vec3  lightDir3 = normalize(vec3(-toLight.x, -toLight.y, uLightRadius * 0.3));
    float diffuse   = max(0.0, dot(norm, lightDir3));

    // ---- Specular (Blinn-Phong) ---------------------------------------------
    vec3  viewDir  = vec3(0.0, 0.0, 1.0); // 2D: view always from z+
    vec3  halfDir  = normalize(lightDir3 + viewDir);
    float spec     = pow(max(dot(norm, halfDir), 0.0), 32.0) * specStr;

    // ---- Combine ------------------------------------------------------------
    vec3 light = uLightColor * uLightIntensity
               * atten * shadow
               * (albedo * diffuse + spec);

    fragColor = vec4(light, 1.0);
}
