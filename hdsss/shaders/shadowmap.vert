#version 460 core
layout(location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix;
layout(std140, binding = 0) uniform MVPMatrices {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalMatrix;
};

void main() {
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}