#version 460 core
#extension GL_GOOGLE_include_directive : enable
#extension GL_ARB_draw_instanced : enable

#include "include/math.glsl"
#include "include/surfel.glsl"

layout(invocations = 4) in;
layout(points) in;
layout(points, max_vertices = 1) out;

flat in Surfel vertexSurfel[];

flat out Surfel geometrySurfel;
flat out float geometryInnerRadius;
flat out float geometryOuterRadius;
flat out float geometryMaxDistance;
flat out ivec4 geometrySubBufferBounds;

layout(location = 0) uniform float minimalEffect = 0.001;
layout(location = 1) uniform float maxDistance = 0.01;
layout(location = 2) uniform vec3 cameraPos;
layout(location = 3) uniform mat4 viewMatrix;
layout(location = 4) uniform mat4 projectionMatrix;
layout(location = 5) uniform struct FB {
    ivec2 resolution;
} framebufferDeviceStep;
layout(location = 6) uniform float fov;

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

float getLevelScale(const in int levelIndex) {
    return float(1 << levelIndex);
}

float maximumDistance(const in Surfel surfel, const in float epsilon) {
    return surfel.radius * sqrt(1.0 / epsilon) *
           sqrt(max(surfel.light.x, max(surfel.light.y, surfel.light.z)));
}

float getSplatRadius(const in int levelIndex, in Surfel surfel) {
    surfel.radius *= getLevelScale(levelIndex);
    return levelIndex < 0 ? 0.0 : (maximumDistance(surfel, minimalEffect));
}

bool hasVisibleEffect(in Surfel surfel) {
    return true;
    // return sphereInFrustum(
    //     surfel.position,
    //     min(maxDistance, getSplatRadius(gl_InvocationID, surfel)),
    //     viewMatrix);
}

// Selects the sub-buffer with coordinates 'coords' on level 'level' for drawing
void useSubBuffer(const in int level, const in ivec2 coords, out vec4 mipScale,
                  out vec4 mipTranslate) {
    const float subBuffersPerRow = float(1 << level);
    const float subBufferWidth = 2.0 / subBuffersPerRow;
    mipScale = vec4(0.5 * subBufferWidth, 0.5 * subBufferWidth, 1.0, 1.0);
    mipTranslate = vec4((coords.x + 0.5) * subBufferWidth - 1.0,
                        (coords.y + 0.5) * subBufferWidth - 1.0, 0.0, 0.0);
}

// #define USE_MIN_MAX_MIP_MAP

void emit(const in vec3 position, const in float surfelRadius,
          const in float innerRadius, const in float outerRadius,
          const in vec4 mipScale, const in vec4 mipTranslate,
          const in ivec4 subBufferBounds) {
    if (innerRadius > maxDistance)
        return;

    geometrySurfel = vertexSurfel[0];
    geometrySurfel.radius = surfelRadius;
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

    // Move vertex to sub buffer
    gl_Position *= mipScale;
    gl_Position += mipTranslate * gl_Position.w;

    // Forward primitive id
    gl_PrimitiveID = gl_PrimitiveIDIn;

    // Forward sub buffer bounds
    geometrySubBufferBounds = subBufferBounds;

    EmitVertex();
}

// #define MOVE_FIRST_LEVEL_TO_SECOND 1

void main() {
    //// Draw only a range of primitives (for debugging purposes)
    // if (gl_PrimitiveIDIn < firstSurfelIndex || gl_PrimitiveIDIn >=
    // lastSurfelIndex) 	return;

#ifdef MOVE_FIRST_LEVEL_TO_SECOND
    if (gl_InvocationID == 0)
        return;
#endif

    // Ignore surfels which are too far outside the view frustum to have a
    // visible effect on what is inside the frustum
    if (!hasVisibleEffect(vertexSurfel[0]))
        return;

    // The gl_InvocationID will range over [0..levelCount-1]
    gl_Layer = gl_InvocationID;

    // Select sub-buffer in a round robin manner
    const int buffersPerRow = 1 << gl_InvocationID;
    const int buffersPerLevel = buffersPerRow * buffersPerRow;
    const int index = gl_PrimitiveIDIn % buffersPerLevel;
    const ivec2 subBufferCoord =
        ivec2(index % buffersPerRow, index / buffersPerRow);

    // Compute 2D bounding box of the sub-buffer
    const ivec2 subBufferResolution =
        framebufferDeviceStep.resolution / buffersPerRow;
    const ivec4 subBufferBounds =
        ivec4(subBufferCoord * subBufferResolution,
              (subBufferCoord + ivec2(1)) * subBufferResolution);

    // Adjust scaling and translation vectors for the chosen sub-buffer
    vec4 mipScale, mipTranslate;
    useSubBuffer(gl_InvocationID, subBufferCoord, mipScale, mipTranslate);

    // Draw the splat
    emit(vertexSurfel[0].position,
         getLevelScale(gl_InvocationID) * vertexSurfel[0].radius,
#ifdef MOVE_FIRST_LEVEL_TO_SECOND
         getSplatRadius((gl_InvocationID == 1 ? 0 : gl_InvocationID) - 1,
                        vertexSurfel[0]),
#else
         getSplatRadius(gl_InvocationID - 1, vertexSurfel[0]),
#endif
         getSplatRadius(gl_InvocationID + 0, vertexSurfel[0]), mipScale,
         mipTranslate, subBufferBounds);
}