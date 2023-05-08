#version 460 core
#extension GL_ARB_shader_viewport_layer_array : enable
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out vec2 texCoord;

void main() {
    gl_Layer = gl_InstanceID;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    texCoord = aTexCoord;
}