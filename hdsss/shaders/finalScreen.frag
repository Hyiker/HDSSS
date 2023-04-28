#version 460 core

out vec4 FragColor;
in vec2 texCoord;

layout(binding = 0) uniform sampler2D diffuseTexture;
layout(binding = 1) uniform sampler2D specularTexture;
layout(binding = 2) uniform sampler2D translucencyTexture;
layout(binding = 3) uniform sampler2D sssTexture;
layout(binding = 4) uniform sampler2D skyboxTexture;
layout(binding = 5) uniform sampler2D transparentIORTexture;

uniform bool directOutput;
uniform bool enableDiffuse;
uniform bool enableSpecular;
uniform bool enableTranslucency;
uniform bool enableSSS;

vec3 gammaCorrection(in vec3 color) {
    const float gamma = 2.2;
    return pow(color, vec3(1.0 / gamma));
}

void main() {
    vec3 diffuse = texture(diffuseTexture, texCoord).rgb;
    vec3 specular = texture(specularTexture, texCoord).rgb;
    vec3 translucency = texture(translucencyTexture, texCoord).rgb;
    vec3 sss = texture(sssTexture, texCoord).rgb;
    vec3 skyboxTexture = texture(skyboxTexture, texCoord).rgb;
    vec3 transparent = texture(transparentIORTexture, texCoord).rgb;
    bool sssEnabled = length(sss) > 0.0;
    vec3 color = vec3(0.0);
    if (sssEnabled) {
        if (enableDiffuse) {
            color += diffuse;
        }
        if (enableSpecular) {
            color += specular;
        }
        if (enableTranslucency) {
            color += translucency;
        }
        if (enableSSS) {
            color += sss;
        }
    } else {
        color = diffuse + specular;
    }
    color += skyboxTexture;
    if (directOutput) {
        FragColor = vec4(color, 1.0);
        return;
    }
    color = gammaCorrection(color);
    FragColor = vec4(color, 1.0);
}