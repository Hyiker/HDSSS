#version 460

#extension GL_GOOGLE_include_directive : enable

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

layout(location = 20) uniform float aspect;
layout(location = 21) uniform float scale;
layout(location = 22) uniform mat4 viewMatrix;
layout(location = 23) uniform float fov;
layout(location = 24) uniform vec3 cameraPosition;

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

// View frustum culling, "radar approach" (Kim Pallister - Game Programming Gems
// 5)
bool sphereInFrustum(const in vec3 p, const in float r) {
    // const float halfFov =  0.5 * degToRad(fov);
    // const float tanHalfFov = tan(halfFov);
    // const float invCosHalfFov = 1.0/cos(halfFov);
    // const float invCosHalfFovX = 1.0/cos(atan(tan(halfFov)*aspect));

    // vec3 cameraX = vec3(viewMatrix[0][0], viewMatrix[1][0],
    // viewMatrix[2][0]); //viewMatrix.block<1,3>(0,0); vec3 cameraY =
    // vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
    // //viewMatrix.block<1,3>(1,0); vec3 cameraZ = -vec3(viewMatrix[0][2],
    // viewMatrix[1][2], viewMatrix[2][2]); //-viewMatrix.block<1,3>(2,0);

    //// Parameters
    // const float nearDist = 0.5f;	// as in MouseViewDevice.cpp
    // const float farDist = 200.f;	// as in MouseViewDevice.cpp

    // const vec3 v = p - cameraPosition;
    // vec3 pc;

    //// Test in Z direction
    // pc.z = dot(v, cameraZ);
    // if (pc.z < nearDist - r || pc.z > farDist + r)
    //	return false;

    //// Test in Y direction
    // pc.y = dot(v, cameraY);
    // float d = r * invCosHalfFov;
    // const float h = pc.z * tanHalfFov;
    //	if (pc.y > h+d || pc.y < -h-d)
    //	return false;

    //// Test in X direction
    // pc.x = dot(v, cameraX);
    // d = r * invCosHalfFovX;
    // const float w = h * aspect;
    //	if (pc.x > w+d || pc.x < -w-d)
    //		return false;

    return true;
}
float computeIdealSurfelRadius(in float scale, in float d, in float fov) {
    return scale * tan(fov * 0.5) * d;
}
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

    const float boundingSphereRadius =
        max(length(center - a), max(length(center - b), length(center - c)));

    if (!sphereInFrustum(center, boundingSphereRadius)) {
        gl_TessLevelOuter[0] = 0;
    } else {
        // If this vertex is the first in its patch, compute tessellation levels
        // here and pass the patch-specific attributes

        const vec3 cameraZ =
            -vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);

        // Add near distance to have peak in surfel size there
        const float nearPlaneDistance = 0.01;
        const float dist =
            distance(vPosition[0],
                     cameraPosition + nearPlaneDistance * cameraZ) +
            nearPlaneDistance;

        const float area = triangleArea(a, b, c);

        // Calculate radius of surfel and resulting tessellation level
        const float idealRadius = computeIdealSurfelRadius(scale, dist, fov);
        const float tessLevel = area >= 3 * PI * idealRadius * idealRadius
                                    ? tessLevelFromRadius(idealRadius, area)
                                    : 1;
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
    }
}