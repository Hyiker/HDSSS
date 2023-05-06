#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/sampling.glsl"

out vec3 FragColor;
in vec2 texCoord;

layout(binding = 0) uniform sampler2D translucencyTexture;
void main() {
    FragColor =
        textureCubic(translucencyTexture, texCoord, SAMPLING_CATROM).rgb;
}