#version 460 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out vec2 texCoord;
layout(location = 1) flat out int layer;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    texCoord = aTexCoord;
    layer = gl_InstanceID;
}