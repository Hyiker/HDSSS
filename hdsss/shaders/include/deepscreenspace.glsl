#ifndef HDSSS_SHADERS_INCLUDE_DEEPSCREENSPACE_GLSL
#define HDSSS_SHADERS_INCLUDE_DEEPSCREENSPACE_GLSL

// Helper functions to sample the individual deferred buffers
vec4 sampleBuffer(const in sampler2DArray s, const in vec2 texCoord,
                  const in int layer) {
    return texelFetch(s, ivec3(texCoord, layer), 0);
}

vec4 samplePosition(const sampler2DArray positionTexture,
                    const in vec2 texCoord, const in int layer) {
    return sampleBuffer(positionTexture, texCoord, layer);
}

vec3 sampleNormal(const sampler2DArray normalTexture, const in vec2 texCoord,
                  const in int layer) {
    return sampleBuffer(normalTexture, texCoord, layer).rgb;
}

#endif /* HDSSS_SHADERS_INCLUDE_DEEPSCREENSPACE_GLSL */
