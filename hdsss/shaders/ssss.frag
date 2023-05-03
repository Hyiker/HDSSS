#version 460 core
#extension GL_GOOGLE_include_directive : enable
#include "include/math.glsl"
#include "include/subsurface.glsl"

out vec3 FragColor;
in vec2 texCoord;

layout(binding = 0) uniform sampler2D GBufferPosition;
layout(binding = 1) uniform sampler2D GBufferNormal;
// simple material: transparent(3)(sss mask) + IOR(1)
// pbr: transmission(1)(sss mask) + sigma_t(3)
layout(binding = 2) uniform sampler2D GBuffer3;
// simple material: unused
// pbr: sigma_a(3) + roughness(1)
layout(binding = 3) uniform sampler2D GBuffer4;
layout(binding = 4) uniform sampler2D TransmittedIrradiance;

#define INNER_LAYER_N 2
#define OUTER_LAYER_CNT 5
#define N_LAYERS 10
float gridWidths[N_LAYERS] = {1,
                              1.6666666666666667,
                              2.777777777777778,
                              4.629629629629631,
                              7.716049382716051,
                              12.86008230452675,
                              21.433470507544584,
                              35.72245084590764,
                              59.53741807651273,
                              99.22903012752123};

uniform float pixelAreaScale;

float gridIndexToPosition1D(int s, int layer) {
    return 0.5 * (2 * INNER_LAYER_N + 1) *
           pow(OUTER_LAYER_CNT / (OUTER_LAYER_CNT - 2), layer - 1) *
           ((2 * s - 1) / (OUTER_LAYER_CNT - 2) - 1);
}
vec4 sampleMipmap(sampler2D tex, vec2 uv, float width) {
    float level = log2(width);
    int level0 = int(level), level1 = level0 + 1;
    vec4 sample0 = textureLod(tex, uv, level0),
         sample1 = textureLod(tex, uv, level1);
    return mix(sample0, sample1, level - level0);
}
ivec2 gridShellIndicies[16] = {
    ivec2(0, 0), ivec2(1, 0), ivec2(2, 0), ivec2(3, 0),
    ivec2(4, 0), ivec2(4, 1), ivec2(4, 2), ivec2(4, 3),
    ivec2(4, 4), ivec2(3, 4), ivec2(2, 4), ivec2(1, 4),
    ivec2(0, 4), ivec2(0, 3), ivec2(0, 2), ivec2(0, 1),
};
struct FragData {
    vec3 position;
    vec3 normal;
    vec2 uv;
    vec3 sigma_a;
    vec3 sigma_s;
};

void computeLayerEffect(in int layer, in vec2 baseTexSize, in FragData fragData,
                        out vec3 color) {
    float gridWidth = gridWidths[layer],
          gridSize = gridWidth * gridWidth * pixelAreaScale;
    for (int s = 0; s < 16; s++) {
        vec2 uvOffset =
            vec2(gridIndexToPosition1D(gridShellIndicies[s].x, layer),
                 gridIndexToPosition1D(gridShellIndicies[s].y, layer));
        vec2 uv = fragData.uv + uvOffset / baseTexSize;
        vec3 position = sampleMipmap(GBufferPosition, uv, baseTexSize.x).rgb;
        vec3 transmitted_irradiance =
            sampleMipmap(TransmittedIrradiance, uv, baseTexSize.x).rgb;

        color += radianceFactor(vec3(0.0)) *
                 computeRadiantExitance(position, fragData.position, gridSize,
                                        fragData.sigma_s, fragData.sigma_a) *
                 transmitted_irradiance;
    }
}
void main() {
    float sssMask;
#ifdef MATERIAL_PBR
    sssMask = texture(GBuffer3, texCoord).r > 0.0 ? 1.0 : 0.0;
#else
    sssMask = length(texture(GBuffer3, texCoord).rgb) > 0.0 ? 1.0 : 0.0;
#endif
    if (sssMask <= 0.0) {
        FragColor = vec3(0, 0, 0);
        return;
    }
    vec3 color = vec3(0.0);

    vec2 texelSize = 1.0 / textureSize(GBufferPosition, 0).xy;
    vec3 sigma_t = texture(GBuffer3, texCoord).gba;
    vec3 sigma_a = texture(GBuffer4, texCoord).rgb;
    vec3 sigma_s = sigma_t - sigma_a;

    const vec3 fragPositionWS = texture(GBufferPosition, texCoord).rgb;
    const vec3 fragNormalWS = normalize(texture(GBufferNormal, texCoord).rgb);
    FragData fragData =
        FragData(fragPositionWS, fragNormalWS, texCoord, sigma_a, sigma_s);
    // sampling layer 0
    for (int i = -INNER_LAYER_N; i <= INNER_LAYER_N; i++) {
        for (int j = -INNER_LAYER_N; j <= INNER_LAYER_N; j++) {
            vec2 offset = vec2(i, j) * texelSize;
            vec2 uv = texCoord + offset;
            vec3 position = texture(GBufferPosition, uv).rgb;
            vec3 normal = texture(GBufferNormal, uv).rgb;
            vec3 transmitted_irradiance =
                texture(TransmittedIrradiance, uv).rgb;

            color += radianceFactor(vec3(0.0)) *
                     computeRadiantExitance(position, fragPositionWS,
                                            pixelAreaScale, sigma_s, sigma_a) *
                     transmitted_irradiance;
        }
    }
    vec2 baseTexSize = textureSize(GBufferPosition, 0).xy;
    // sampling layer 1-9
    for (int i = 1; i < N_LAYERS; i++) {
        computeLayerEffect(i, baseTexSize, fragData, color);
    }
    FragColor = color;
}