#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "include/deepscreenspace.glsl"
#include "include/math.glsl"
#include "include/subsurface.glsl"
#include "include/surfel.glsl"

layout(location = 0) out vec3 fragColor;

layout(binding = 0, location = 7) uniform sampler2DArray positionShuffled;
layout(binding = 1, location = 8) uniform sampler2DArray normalShuffled;
layout(location = 9) uniform float strength;

in Surfel geometrySurfel;

flat in float geometryInnerRadius;
flat in float geometryOuterRadius;
flat in float geometryMaxDistance;
flat in ivec4 geometrySubBufferBounds;

void main() {
    // Read position and normal of the pixel

    const vec3 pixelNormal =
        normalize(sampleNormal(normalShuffled, gl_FragCoord.xy, gl_Layer));
    const vec4 pixelPosition =
        samplePosition(positionShuffled, gl_FragCoord.xy, gl_Layer);
    const float distanceToSurfel =
        length(pixelPosition.xyz - geometrySurfel.position);

    if (pixelPosition.a > 0.0 && distanceToSurfel < geometryOuterRadius &&
        distanceToSurfel > geometryInnerRadius) {
        fragColor =
            computeEffect(geometrySurfel, geometrySurfel.position,
                          SplatReceiver(pixelPosition.xyz, pixelNormal)) *
            strength;
    } else {
        fragColor = vec3(0.0);
    }
}
