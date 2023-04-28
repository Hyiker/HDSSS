#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/lighting.glsl"
#include "include/surfel.glsl"

layout(location = 0) in vec4 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in float aRadius;

layout(location = 0) flat out Surfel vSurfel;

layout(std140, binding = 1) uniform LightBlock {
    ShaderLight lights[12];
    int nLights;
};

void main() {
    vec3 irradiance = vec3(0.0);
    for (int i = 0; i < nLights; i++) {
        irradiance += computeSurfaceIrradiance(aPos.xyz, aNormal, lights[i]);
    }
    vSurfel = initSurfel(aPos.xyz, aNormal, aRadius, irradiance);
}