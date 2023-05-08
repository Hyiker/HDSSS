#version 460 core

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec3 unshuffleColor;

layout(binding = 0) uniform isampler2DArray partitionTexture;
layout(binding = 1) uniform sampler2DArray shuffleResult;

void main() {
    ivec2 originalCoords =
        texelFetch(partitionTexture, ivec3(gl_FragCoord.xy, gl_Layer), 0).zw;
    unshuffleColor =
        texelFetch(shuffleResult, ivec3(originalCoords, gl_Layer), 0).rgb;
}