#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/lighting.glsl"

layout(binding = 0) uniform sampler2D GBufferPosition;
layout(binding = 1) uniform sampler2D GBufferNormal;
layout(binding = 2) uniform sampler2D GBufferAlbedo;
layout(binding = 3) uniform sampler2D GBufferTransparentIOR;
layout(binding = 4) uniform sampler2D GBufferOcclusionRoughness;
layout(binding = 5) uniform sampler2D MainLightShadowMap;

uniform mat4 mainLightMatrix;
uniform vec3 cameraPosition;

in vec2 texCoord;
layout(location = 0) out vec4 TransmittedIrradiance;
layout(location = 1) out vec4 ReflectedRadiance;

layout(std140, binding = 1) uniform LightBlock {
    ShaderLight lights[12];
    int nLights;
};

struct SurfaceParamsPBRMetallicRoughness {
    vec3 viewDirection;

    vec3 normal;
    vec3 baseColor;
    float metallic;
    float roughness;
};

float DistributionGGX(in vec3 N, in float roughness, in vec3 H) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = denom * denom;

    return roughness;
}
vec3 FresnelSchlickApprox(in vec3 F0, in vec3 V, in vec3 H) {
    float VoH = max(0.001, dot(V, H));
    float F = pow(1.0 - VoH, 5.0);
    return F0 + (1.0 - F0) * F;
}
float SchlickGGXGeometry(in float roughness, in vec3 L, in vec3 V, in vec3 N) {
    float alpha = (roughness + 1) * 0.5;
    alpha *= alpha;
    float k = alpha / 2.0;
    float VoN = max(0.001, dot(V, N));
    float LoN = max(0.001, dot(L, N));
    float G1V = VoN / (VoN * (1.0 - k) + k);
    float G1L = LoN / (LoN * (1.0 - k) + k);
    return G1V * G1L;
}

float PBRCookTorranceBRDF(in SurfaceParamsPBRMetallicRoughness surface,
                          in ShaderLight light, in vec3 L) {
    vec3 H = safeNormalize(L + surface.viewDirection);
    float NDF = DistributionGGX(surface.normal, surface.roughness, H);
    float G = SchlickGGXGeometry(surface.roughness, L, surface.viewDirection,
                                 surface.normal);
    return NDF * G /
           (4.0 * max(0.001, dot(surface.normal, L)) *
            max(0.001, dot(surface.normal, surface.viewDirection)));
}
// float PBRDiffuseStrength(in SurfaceParamsPBRMetallicRoughness surface,
//                          in ShaderLight light, in vec3 L) {
//     vec3 h = safeNormalize(L + surface.viewDirection);
//     float F90 = 0.5 + 2 * surface.roughness * dot(h, L);
//     float f_diff =
//         (1 + (F90 - 1) * pow(1 - dot(surface.normal, L), 5)) *
//         (1 +
//          (F90 - 1) * pow(1 - dot(surface.normal, surface.viewDirection), 5)) /
//         3.1415926;
//     return f_diff;
// }
void computePBRMetallicRoughnessLocalLighting(
    in SurfaceParamsPBRMetallicRoughness surface, in ShaderLight light,
    in vec3 V, in vec3 L, in float intensity, out vec3 diffuse,
    out vec3 specular) {
    vec3 N = surface.normal;
    vec3 baseColor = surface.baseColor;
    vec3 H = normalize(V + L);

    vec3 F0 = mix(vec3(0.04), surface.baseColor, surface.metallic);
    vec3 F = FresnelSchlickApprox(F0, surface.viewDirection, H);

    float NdotL = max(0.0, dot(N, L));
    vec3 kD = (1.0 - F) * (1.0 - surface.metallic);
    diffuse = kD * baseColor * intensity * NdotL * PI_INV;
    specular = F * PBRCookTorranceBRDF(surface, light, L) * intensity * NdotL;
}

void main() {
    vec3 positionWS;
    vec3 normalWS;
    // diffuse(rgb) + specular(a)
    vec4 albedo;
    vec3 transparent;
    float ior;
    float occlusion;
    float roughness;
    positionWS = texture(GBufferPosition, texCoord).xyz;
    normalWS = texture(GBufferNormal, texCoord).xyz;
    albedo = texture(GBufferAlbedo, texCoord).rgba;
    transparent = texture(GBufferTransparentIOR, texCoord).rgb;
    ior = texture(GBufferTransparentIOR, texCoord).a;
    vec2 occlusionRoughness = texture(GBufferOcclusionRoughness, texCoord).rg;
    occlusion = occlusionRoughness.r;
    roughness = occlusionRoughness.g;

    vec3 V = normalize(cameraPosition - positionWS);
    vec3 t_irradiance = vec3(0.0), r_radiance = vec3(0.0);
    for (int i = 0; i < nLights; i++) {
        ShaderLight light = lights[i];
        float distance = 1.0;
        vec3 L, H;
        switch (light.type) {
            case LIGHT_TYPE_DIRECTIONAL:
                L = normalize(-lights[0].direction.xyz);
                break;
            default:
                continue;
        }
        float intensity = light.intensity / (distance * distance);
        float shadow =
            computeShadow(mainLightMatrix, MainLightShadowMap, positionWS);
        vec3 irradiance, radiance;
#ifdef MATERIAL_PBR
        SurfaceParamsPBRMetallicRoughness surface;
        surface.viewDirection = V;
        surface.normal = normalWS;
        surface.baseColor = albedo.rgb;
        surface.metallic = albedo.a;
        surface.roughness = roughness;
        computePBRMetallicRoughnessLocalLighting(
            surface, light, V, L, intensity, irradiance, radiance);
#else
        SurfaceParamsBlinnPhong params;
        params.albedo = albedo;
        params.shininess = 20.0;
        params.normal = normalWS;
        computeBlinnPhongLocalLighting(params, light, V, L, intensity,
                                       irradiance, radiance);
#endif
        t_irradiance += irradiance * (1.0 - shadow);
        r_radiance += radiance * (1.0 - shadow);
    }
    TransmittedIrradiance = vec4(t_irradiance, 1.0);
    ReflectedRadiance = vec4(r_radiance, 1.0);
}