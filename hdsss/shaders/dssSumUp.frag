#version 460 core

layout(location = 0) in vec2 texCoord;
layout(location = 1) flat in int layer;
layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform sampler2DArray multiLayerTextures;

void main() {
    fragColor = texture(multiLayerTextures, vec3(texCoord, layer)).rgb;
}