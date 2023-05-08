#version 460 core

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec3 shuffledNormal;
layout(location = 1) out vec4 shuffledPosition;

layout(binding = 0) uniform sampler2D normalTexture;
layout(binding = 1) uniform sampler2D positionTexture;
layout(binding = 2) uniform isampler2DArray partitionTexture;
layout(location = 3) uniform int currentLayer;

void main() {
    const ivec2 originalCoords =
        texelFetch(partitionTexture, ivec3(gl_FragCoord.xy, currentLayer), 0)
            .rg;
    shuffledNormal = texelFetch(normalTexture, originalCoords, 0).xyz;
    shuffledPosition = texelFetch(positionTexture, originalCoords, 0).rgba;
}