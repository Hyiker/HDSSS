#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/lighting.glsl"

layout(binding = 0) uniform sampler2D GBufferPosition;
layout(binding = 1) uniform sampler2D GBufferNormal;
layout(binding = 2) uniform sampler2D GBufferAlbedo;
layout(binding = 3) uniform isampler2D GBufferTransparentIOR;
layout(binding = 4) uniform sampler2D MainLightShadowMap;

uniform mat4 mainLightMatrix;
uniform vec3 cameraPosition;

in vec2 texCoord;
layout(location = 0) out vec4 TransmittedIrradiance;
layout(location = 1) out vec4 ReflectedRadiance;

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
    positionWS = texture(GBufferPosition, texCoord).xyz;
    normalWS = texture(GBufferNormal, texCoord).xyz;
    albedo = texture(GBufferAlbedo, texCoord).rgba;

    vec3 V = normalize(cameraPosition - positionWS);
    vec3 t_irradiance = vec3(0.0), r_radiance = vec3(0.0);
    SurfaceParams params;
    params.albedo = albedo;
    params.shininess = 20.0;
    params.viewDir = V;
    params.normal = normalWS;
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
        params.lightDir = L;
        float intensity = light.intensity / (distance * distance);
        float shadow =
            computeShadow(mainLightMatrix, MainLightShadowMap, positionWS);
        vec3 diffuse, specular;
        computeBlinnPhongLocalLighting(params, light, intensity, diffuse,
                                       specular);
        t_irradiance += diffuse * (1.0 - shadow);
        r_radiance += specular * (1.0 - shadow);
    }
    TransmittedIrradiance = vec4(t_irradiance, 1.0);
    ReflectedRadiance = vec4(r_radiance, 1.0);
}