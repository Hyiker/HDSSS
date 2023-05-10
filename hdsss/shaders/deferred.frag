#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/lighting.glsl"

layout(binding = 0) uniform sampler2D GBufferPosition;
layout(binding = 1) uniform sampler2D GBufferNormal;
layout(binding = 2) uniform sampler2D GBufferAlbedo;
layout(binding = 3) uniform sampler2D GBuffer3;
layout(binding = 4) uniform sampler2D GBuffer4;
layout(binding = 5) uniform sampler2D GBuffer5;
layout(binding = 6) uniform sampler2D MainLightShadowMap;

uniform mat4 mainLightMatrix;
uniform vec3 cameraPosition;

in vec2 texCoord;
layout(location = 0) out vec3 DiffuseResult;
layout(location = 1) out vec3 TransmittedIrradiance;
layout(location = 2) out vec3 ReflectedRadiance;

layout(std140, binding = 1) uniform LightBlock {
    ShaderLight lights[12];
    int nLights;
};

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
    transparent = texture(GBuffer3, texCoord).rgb;
    ior = texture(GBuffer3, texCoord).a;
    roughness = texture(GBuffer4, texCoord).a;
    occlusion = texture(GBuffer5, texCoord).r;

    vec3 V = normalize(cameraPosition - positionWS);
    vec3 diffuse_reflect = vec3(0.0), r_radiance = vec3(0.0),
         t_irradiance = vec3(0.0);
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
        vec3 diff, radiance, irradiance;
#ifdef MATERIAL_PBR
        SurfaceParamsPBRMetallicRoughness surface;
        surface.viewDirection = V;
        surface.normal = normalize(normalWS);
        surface.baseColor = albedo.rgb;
        surface.metallic = albedo.a;
        surface.roughness = roughness;
        computePBRMetallicRoughnessLocalLighting(surface, light, V, L,
                                                 intensity, diff, radiance);
        irradiance += transparent.r *
                      computeSurfaceIrradiance(positionWS, normalWS, light) *
                      (1.0 - shadow);
#else
        SurfaceParamsBlinnPhong params;
        params.albedo = albedo;
        params.shininess = 20.0;
        params.normal = normalWS;
        computeBlinnPhongLocalLighting(params, light, V, L, intensity, diff,
                                       radiance);
        irradiance += transparent *
                      computeSurfaceIrradiance(positionWS, normalWS, light) *
                      (1.0 - shadow);
#endif
        diffuse_reflect += diff * (1.0 - shadow);
        r_radiance += radiance * (1.0 - shadow);
        t_irradiance += irradiance * (1.0 - shadow);
    }
    DiffuseResult = diffuse_reflect;
    TransmittedIrradiance = t_irradiance;
    ReflectedRadiance = r_radiance;
}