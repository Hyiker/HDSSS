#version 460 core
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

layout(location = 0) out vec3 vPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vTexCoord;
layout(location = 3) out vec3 vTangent;
layout(location = 4) out vec3 vBitangent;

layout(std140, binding = 0) uniform MVPMatrices {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalMatrix;
};

void main() {
    mat3 model3 = mat3(model);
    vNormal = normalize(mat3(normalMatrix) * aNormal);
    vTangent = normalize(model3 * aTangent);
    vBitangent = normalize(model3 * aBitangent);
    vTexCoord = aTexCoord;
    vPos = (model * vec4(aPos, 1.0)).xyz;
    gl_Position = projection * view * vec4(vPos, 1.0);
}