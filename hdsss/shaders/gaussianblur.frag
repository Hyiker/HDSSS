#version 460 core

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform sampler2DArray image;

uniform bool axisX;
#define N_WEIGHTS 8

uniform float weight[N_WEIGHTS] =
    float[](0.1974, 0.1747, 0.1210, 0.0656, 0.0278, 0.0092, 0.0024, 0.0005);

void main() {
    vec2 tex_offset =
        1.0 / textureSize(image, 0).xy;  // gets size of single texel
    vec3 texCoord3D = vec3(texCoord, gl_Layer);
    vec3 result = texture(image, texCoord3D).rgb *
                  weight[0];  // current fragment's contribution
    if (axisX) {
        for (int i = 1; i < N_WEIGHTS; ++i) {
            result +=
                texture(image, texCoord3D + vec3(tex_offset.x * i, 0.0, 0))
                    .rgb *
                weight[i];
            result +=
                texture(image, texCoord3D - vec3(tex_offset.x * i, 0.0, 0))
                    .rgb *
                weight[i];
        }
    } else {
        for (int i = 1; i < N_WEIGHTS; ++i) {
            result +=
                texture(image, texCoord3D + vec3(0.0, tex_offset.y * i, 0))
                    .rgb *
                weight[i];
            result +=
                texture(image, texCoord3D - vec3(0.0, tex_offset.y * i, 0))
                    .rgb *
                weight[i];
        }
    }
    fragColor = result;
}