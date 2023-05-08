#version 460 core
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

layout(location = 0) out vec3 vPos;
layout(location = 1) out vec3 vNormal;

layout(std140, binding = 0) uniform MVPMatrices {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalMatrix;
};

void main() {
    mat3 model3 = mat3(model);
    vNormal = normalize(mat3(normalMatrix) * aNormal);
    vPos = (model * vec4(aPos, 1.0)).xyz;
}