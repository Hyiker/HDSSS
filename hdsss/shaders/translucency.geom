#version 460 core
#extension GL_GOOGLE_include_directive : enable
#extension GL_ARB_draw_instanced : enable
#include "include/math.glsl"

#include "include/surfel.glsl"

////////////////////////////
// Per-fragment computations
////////////////////////////

layout(points) in;
layout(points, max_vertices = 1) out;

layout(location = 0) flat in Surfel vertexSurfel[];

flat out Surfel geometrySurfel;
flat out float geometryInnerRadius;
flat out float geometryOuterRadius;
flat out float geometryMaxDistance;

layout(location = 0) uniform float minimalEffect = 0.001;
layout(location = 1) uniform float maxDistance = 0.01;
layout(location = 2) uniform vec3 cameraPos;
layout(location = 3) uniform mat4 viewMatrix;
layout(location = 4) uniform mat4 projectionMatrix;
layout(location = 5) uniform struct FB {
    ivec2 resolution;
} framebufferDeviceStep;
layout(location = 6) uniform float fov;

layout(binding = 0, location = 7) uniform sampler2D GBufferPosition;

bool boxIntersectsSphere(const in vec3 Bmin, const in vec3 Bmax,
                         const in vec3 C, const in float r) {
    float r2 = r * r;
    float dmin = 0;
    for (int i = 0; i < 3; i++) {
        if (C[i] < Bmin[i])
            dmin += sqr(C[i] - Bmin[i]);
        else if (C[i] > Bmax[i])
            dmin += sqr(C[i] - Bmax[i]);
    }
    return dmin <= r2;
}

float maximumDistance(const in Surfel surfel, const in float epsilon) {
    return surfel.radius * sqrt(1.0 / epsilon) *
           sqrt(max(surfel.light.x, max(surfel.light.y, surfel.light.z)));
}

float getLevelScale(const in int levelIndex) {
    return float(1 << levelIndex);
}

float getSplatRadius(const in int levelIndex, in Surfel surfel) {
    surfel.radius *= getLevelScale(levelIndex);
    return levelIndex < 0 ? 0.0 : (maximumDistance(surfel, minimalEffect));
}

// #define USE_MIN_MAX_MIP_MAP

void emit(const in vec3 position, const in float surfelRadius,
          const in float innerRadius, const in float outerRadius) {
    if (innerRadius > maxDistance)
        return;

    geometrySurfel = vertexSurfel[0];
    geometrySurfel.radius = surfelRadius;

    vec4 surfelPos = vec4(geometrySurfel.position, 1);
    surfelPos = projectionMatrix * viewMatrix * surfelPos;
    vec2 pixelUV = surfelPos.xy / surfelPos.w;
    pixelUV = 0.5 * pixelUV + 0.5;
    vec3 pixelPosition = texture(GBufferPosition, pixelUV).xyz;

    const vec3 viewDirection = normalize(cameraPos - pixelPosition.xyz);
    const bool isBackside =
        dot(geometrySurfel.position - pixelPosition.xyz, viewDirection) < 0.0 &&
        // adjust the criterion by equation(8)
        // compensate for the steep view angle triangles
        dot(normalize(geometrySurfel.normal), viewDirection) < 0.2;
    if (!isBackside)
        return;

    geometryInnerRadius = innerRadius;
    geometryOuterRadius = outerRadius;
    geometryMaxDistance = maxDistance;
    gl_Position = projectionMatrix * viewMatrix * vec4(position, 1);

    // Compute radius of the splat in pixels
    vec3 cameraZ = -vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
    float projectedDistance = dot(position - cameraPos, cameraZ);
    gl_PointSize = int(ceil(maximumDistance(vertexSurfel[0], minimalEffect) *
                            framebufferDeviceStep.resolution.y /
                            (projectedDistance * tan(0.5 * fov))));
    // Forward primitive id
    gl_PrimitiveID = gl_PrimitiveIDIn;

    EmitVertex();
}

void main() {
    // Draw the splat
    emit(vertexSurfel[0].position, getLevelScale(2) * vertexSurfel[0].radius, 0,
         getSplatRadius(2, vertexSurfel[0]));
}