#version 460 core
#extension GL_GOOGLE_include_directive : enable

layout(binding = 0) uniform samplerCube skyboxTex;
layout(location = 0) in vec3 vTexCoord;

out vec4 FragColor;
void main() {
    FragColor = texture(skyboxTex, vTexCoord);
}