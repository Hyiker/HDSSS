#version 460

#extension GL_GOOGLE_include_directive : enable
// #include "include/dss.glsl"
#include "include/math.glsl"

layout(vertices = 3) out;  // Size of the output patch

layout(location = 0) in vec3 vPosition[];  // Vertex positions in world space
layout(location = 1) in vec3 vNormal[];    // Vertex normal in model space
// in int vMaterialId[];

layout(location = 0) out vec3 tcPosition[];  // Vertex positions in world space
layout(location = 1) out vec3 tcNormal[];    // Vertex normals in model space
layout(location = 2) patch out float tcRadius;  // Radius
layout(location = 3) patch out vec3 tcSigmaT;   // sigma_t
layout(location = 4) patch out vec3 tcSigmaA;   // sigma_a
// patch out int tcMaterialId;

#ifdef MATERIAL_PBR
layout(std140, binding = 3) uniform PBRMetallicMaterial {
    vec4 baseColorMetallic;
    // transmission(1) + sigmaT(3)
    vec4 transmissionSigmaT;
    // sigmaA(3) + roughness(1)
    vec4 sigmaARoughness;
};
#else
layout(std140, binding = 2) uniform SimpleMaterial {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    vec4 transparentIOR;
    float shininess;
};
#endif

float tessLevelFromRadius(in float radius, in float area) {
    return ceil(sqrt(12 * PI * area - 3 * PI_SQR * radius * radius) /
                    (3 * PI * radius) -
                1);
}
void setTessellationLevels(in float level) {
    gl_TessLevelOuter[0] = level;
    gl_TessLevelOuter[1] = level;
    gl_TessLevelOuter[2] = level;
    gl_TessLevelInner[0] = level;
    gl_TessLevelInner[1] = level;
}
float radiusFromTessLevel(in float tessLevel, in float area) {
    int i = int(tessLevel);
    float vi = (i & 1) == 1 ? (3 * sqr(ceil(tessLevel / 2.0)))
                            : (0.75 * sqr(tessLevel) + 1.5 * i + 1);
    return sqrt(area / (PI * vi));
}

void main() {
    if (
#ifdef MATERIAL_PBR
        transmissionSigmaT.r == 0
#else
        length(transparentIOR.rgb) == 0
#endif
    ) {
        // remove patches without subsurface effect
        setTessellationLevels(0);
        return;
    }
    // Triangle vertices
    const vec3 a = vPosition[0];
    const vec3 b = vPosition[1];
    const vec3 c = vPosition[2];

    const vec3 center = 0.3333333 * (a + b + c);

    const float area = triangleArea(a, b, c);

    // always use tessellation level 1
    const float tessLevel = 1;
    if (gl_InvocationID == 0)
        setTessellationLevels(tessLevel);

    // Save the actual radius that the surfel got
    const float actualRadius = radiusFromTessLevel(tessLevel, area);
    // const float actualRadius = idealRadius;
    tcRadius = actualRadius;

    // Pass the vertex-specific attributes to the tessellation evaluation
    // shader Move vertex towards triangle center to reduce overlapping at
    // the edges
    tcPosition[gl_InvocationID] =
        vPosition[gl_InvocationID] +
        actualRadius * normalize(center - vPosition[gl_InvocationID]);
    tcNormal[gl_InvocationID] = vNormal[gl_InvocationID];
#ifdef MATERIAL_PBR
    tcSigmaT = transmissionSigmaT.gba;
    tcSigmaA = sigmaARoughness.rgb;
#else
#endif
    // tcMaterialId = vMaterialId[gl_InvocationID];
}