#version 460 core
layout(location = 0) in vec3 aPos;

layout(location = 0) out vec3 vTexCoord;

layout(std140, binding = 0) uniform MVPMatrices {
    mat4 model;
    mat4 view;
    mat4 projection;
}
mvp;

void main() {
    vTexCoord = aPos;
    gl_Position = (mvp.projection * mvp.view * vec4(aPos, 1.0)).xyww;
}