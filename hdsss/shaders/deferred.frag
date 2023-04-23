#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/lighting.glsl"

layout(binding = 0) uniform sampler2D GBufferPosition;
layout(binding = 1) uniform sampler2D GBufferNormal;
layout(binding = 2) uniform sampler2D GBufferAlbedo;
layout(binding = 3) uniform isampler2D GBufferSSSMask;
layout(binding = 4) uniform sampler2D MainLightShadowMap;

uniform mat4 mainLightMatrix;
uniform vec3 cameraPosition;

in vec2 texCoord;
out vec4 FragColor;

layout(std140, binding = 1) uniform LightBlock {
    ShaderLight lights[12];
    int nLights;
};

float computeShadow(vec3 positionWS) {
    vec4 positionLS = mainLightMatrix * vec4(positionWS, 1.0);
    vec3 positionNDC = positionLS.xyz / positionLS.w;
    positionNDC = positionNDC * 0.5 + 0.5;
    float shadow = 0.0;
    float bias = 0.005;
    float shadowMapDepth = texture(MainLightShadowMap, positionNDC.xy).r;
    if (positionNDC.z - bias > shadowMapDepth) {
        shadow = 1.0;
    }
    return shadow;
}

void main() {
    vec3 positionWS;
    vec3 normalWS;
    // diffuse(rgb) + specular(a)
    vec4 albedo;
    int sssMask;
    positionWS = texture(GBufferPosition, texCoord).xyz;
    normalWS = texture(GBufferNormal, texCoord).xyz;
    albedo = texture(GBufferAlbedo, texCoord).rgba;
    sssMask = texture(GBufferSSSMask, texCoord).r;

    vec3 V = normalize(cameraPosition - positionWS);
    vec3 color = vec3(0.0);
    SurfaceParams params;
    params.albedo = albedo;
    params.shininess = 50.0;
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
        float attenuation = light.intensity / (distance * distance);
        float shadow = computeShadow(positionWS);
        color += computeBlinnPhongLocalLighting(params, light, attenuation) *
                 (1.0 - shadow);
    }
    FragColor = vec4(color, 1.0);
}