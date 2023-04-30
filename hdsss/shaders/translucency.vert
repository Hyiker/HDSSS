#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/lighting.glsl"
#include "include/surfel.glsl"

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in float aRadius;
layout(location = 3) in vec3 sigmaT;
layout(location = 4) in vec3 sigmaA;

layout(location = 0) flat out Surfel vertexSurfel;

layout(std140, binding = 1) uniform LightBlock {
    ShaderLight lights[12];
    int nLights;
};

layout(binding = 2, location = 10) uniform sampler2D MainLightShadowMap;
layout(location = 11) uniform mat4 lightSpaceMatrix;

void main() {
    vec3 irradiance = vec3(0.0);
    for (int i = 0; i < nLights; i++) {
        float shadow =
            computeShadow(lightSpaceMatrix, MainLightShadowMap, aPos.xyz);
        irradiance += (1.0 - shadow) *
                      computeSurfaceIrradiance(aPos.xyz, aNormal, lights[i]);
    }
    vertexSurfel =
        initSurfel(aPos.xyz, aNormal, aRadius, sigmaA, sigmaT, irradiance);
}