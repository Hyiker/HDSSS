#version 460

#extension GL_GOOGLE_include_directive : enable

#include "include/constants.glsl"
#include "include/surfel.glsl"

layout(triangles, equal_spacing,
       point_mode) in;  // triangles, equal spacing of subdivisions, only one
                        // vertex per new coordinate

layout(location = 0) in vec3 tcPosition[];  // Vertex positions in world space
layout(location = 1) in vec3 tcNormal[];    // Vertex normals in model space
layout(location = 2) patch in float tcRadius;  // Radius

layout(binding = 0) uniform atomic_uint surfelCounter;
layout(std430, binding = 1) buffer surfelSSBO {
    SurfelData surfels[];
};

float computeRandomOffset(vec3 pos) {
    float a = sin(pos.x) * 1203.f;
    float b = sin(pos.y + 0.123f) * 321.f;
    float c = sin(pos.z + 0.42f) * 78.7f;
    uint d = uint(a * a + 1.23f * b * b + 4.32f * c * c);
    d = d * 1103515245u + 12345u;
    return float(d) / 4294967296.0;
}

void main() {
    // Compute new position
    const vec3 p0 = gl_TessCoord.x * tcPosition[0];
    const vec3 p1 = gl_TessCoord.y * tcPosition[1];
    const vec3 p2 = gl_TessCoord.z * tcPosition[2];
    vec3 position = p0 + p1 + p2;

    // Compute new normal
    const vec3 n0 = gl_TessCoord.x * tcNormal[0];
    const vec3 n1 = gl_TessCoord.y * tcNormal[1];
    const vec3 n2 = gl_TessCoord.z * tcNormal[2];
    vec3 normal = normalize(n0 + n1 + n2);

    // Apply jitter
    const vec3 t0 = normalize(tcPosition[2] - tcPosition[0]);
    const vec3 t1 = cross(t0, normal);
    position +=
        (tcRadius
         //  * jitter
         ) *
        ((2.0 * computeRandomOffset(position + tcPosition[1]) - 1.0) * t0 +
         (2.0 * computeRandomOffset(position + tcPosition[2]) - 1.0) * t1);

    float radius = tcRadius;
    // teMaterialId = tcMaterialId;
    uint index = atomicCounterIncrement(surfelCounter);
    if (index >= N_SURFELS_MAX) {
        return;
    }
    surfels[index] = SurfelData(vec4(position, 0), normal, radius);
}