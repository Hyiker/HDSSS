#version 460 core

out vec4 FragColor;
in vec2 texCoord;

layout(binding = 0) uniform sampler2D screenTexture;
layout(binding = 1) uniform sampler2D subsurfaceScattering;
uniform bool directOutput;
vec3 gammaCorrection(in vec3 color) {
    const float gamma = 2.2;
    return pow(color, vec3(1.0 / gamma));
}

void main() {
    vec3 color = texture(screenTexture, texCoord).rgb;
    vec3 sss = texture(subsurfaceScattering, texCoord).rgb;
    color += sss;
    if (directOutput) {
        FragColor = vec4(color, 1.0);
        return;
    }
    color = gammaCorrection(color);
    FragColor = vec4(color, 1.0);
}