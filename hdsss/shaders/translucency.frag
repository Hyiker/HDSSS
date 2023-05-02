#version 460 core
#extension GL_GOOGLE_include_directive : enable
#include "include/math.glsl"
#include "include/subsurface.glsl"

layout(location = 0) out vec3 fragColor;

in Surfel geometrySurfel;

layout(binding = 0, location = 7) uniform sampler2D GBufferPosition;
layout(binding = 1, location = 8) uniform sampler2D GBufferNormal;
layout(location = 9) uniform float strength;
layout(location = 5) uniform struct FB {
    ivec2 resolution;
} framebufferDeviceStep;

flat in float geometryInnerRadius;
flat in float geometryOuterRadius;
flat in float geometryMaxDistance;

void main() {
    // Read position and normal of the pixel
    vec2 uv = gl_FragCoord.xy / vec2(framebufferDeviceStep.resolution);
    const vec3 pixelNormal = texture(GBufferNormal, uv).xyz;
    const vec4 pixelPosition = texture(GBufferPosition, uv);
    const vec3 xo = pixelPosition.xyz;
    const vec3 xi = geometrySurfel.position;
    const float distanceToSurfel = length(xo - xi);

    if (pixelPosition.a > 0.0 && distanceToSurfel < geometryMaxDistance &&
        distanceToSurfel < geometryOuterRadius &&
        distanceToSurfel > geometryInnerRadius) {
        vec3 adjustedXi = xi;
        vec3 ni = normalize(geometrySurfel.normal);
        vec3 xi_ = xo + ni * dot(xo - xi, ni);
        vec3 xi_xi_ = xi_ - xi;
        float rDisk = SQRT_3 * geometrySurfel.radius;
        adjustedXi += normalize(xi_xi_) * min(rDisk, length(xi_xi_));
        fragColor = computeEffect(geometrySurfel, adjustedXi,
                                  SplatReceiver(xo, pixelNormal)) *
                    strength;
    } else {
        fragColor = vec3(0.0);
    }
}