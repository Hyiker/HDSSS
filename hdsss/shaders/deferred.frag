#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/lighting.glsl"

layout(binding = 0) uniform sampler2D GBufferPosition;
layout(binding = 1) uniform sampler2D GBufferNormal;
layout(binding = 2) uniform sampler2D GBufferAlbedo;
layout(binding = 3) uniform isampler2D GBufferSSSMask;

uniform vec3 uCameraPosition;

in vec2 texCoord;
out vec4 FragColor;

layout(std140, binding = 1) uniform LightBlock {
    ShaderLight lights[12];
    int nLights;
};

void main() {
    vec3 positionWS;
    vec3 normalWS;
    // diffuse(rgb) + specular(a)
    vec4 albedo;
    int sssMask;
    positionWS = texture(GBufferPosition, texCoord).rgb;
    normalWS = texture(GBufferNormal, texCoord).rgb;
    albedo.rgb = texture(GBufferAlbedo, texCoord).rgb;
    albedo.a = texture(GBufferAlbedo, texCoord).a;
    sssMask = texture(GBufferSSSMask, texCoord).r;

    vec3 V = normalize(uCameraPosition - positionWS);
    vec3 color = vec3(0.0);
    for (int i = 0; i < nLights; i++) {
        ShaderLight light = lights[i];
        float distance = 1.0;
        vec3 L, H;
        switch (light.type) {
            case LIGHT_TYPE_DIRECTIONAL:
                L = normalize(-lights[0].direction.xyz);
                H = normalize(V + L);
                break;
            default:
                continue;
        }
        float attenuation = light.intensity / (distance * distance);

        vec3 Ld = albedo.rgb * attenuation * max(dot(L, normalWS), 0.0);
        vec3 Ls = max(vec3(0.0), vec3(albedo.a) * attenuation *
                                     pow(max(0.0, dot(H, normalWS)), 200.0));
        color += light.color.rgb * (Ld + Ls);
    }
    FragColor = vec4(color, 1.0);
}