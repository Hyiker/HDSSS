#ifndef HDSSS_SHADERS_INCLUDE_LIGHTING_GLSL
#define HDSSS_SHADERS_INCLUDE_LIGHTING_GLSL
#include "./math.glsl"
struct ShaderLight {
    // spot, point
    vec4 position;
    // spot, directional
    vec4 direction;
    // all
    vec4 color;
    float intensity;
    // point, spot
    // negative value stands for INF
    float range;
    // spot
    float spotAngle;
    int type;
};

struct SurfaceParams {
    vec3 normal;
    vec3 viewDir;
    vec3 lightDir;

    vec4 albedo;
    float shininess;
};
const int LIGHT_TYPE_SPOT = 0, LIGHT_TYPE_POINT = 1, LIGHT_TYPE_DIRECTIONAL = 2;

void computeBlinnPhongLocalLighting(in SurfaceParams surfaceParams,
                                    in ShaderLight light, in float intensity,
                                    out vec3 diffuse, out vec3 specular) {
    vec3 N = normalize(surfaceParams.normal),
         V = normalize(surfaceParams.viewDir),
         L = normalize(surfaceParams.lightDir);
    vec4 albedo = surfaceParams.albedo;

    vec3 H = normalize(V + L);

    vec3 Ld = albedo.rgb * intensity * max(dot(L, N), 0.0);
    vec3 Ls =
        max(vec3(0.0), vec3(albedo.a) * intensity *
                           pow(max(0.0, dot(H, N)), surfaceParams.shininess));
    diffuse = light.color.rgb * Ld;
    specular = light.color.rgb * Ls;
}

vec3 computeSurfaceIrradiance(in vec3 position, in vec3 normal,
                              in ShaderLight light) {
    vec3 N = normalize(normal);
    vec3 L;
    float distance = 1.0;
    switch (light.type) {
        case LIGHT_TYPE_DIRECTIONAL:
            L = normalize(-light.direction.xyz);
            break;
        default:
            break;
    }
    return light.color.rgb * light.intensity * max(0.0, dot(L, N)) /
           (distance * distance) * PI_INV;
}

#define PCF_KERNEL_SIZE 5

float computeShadow(in mat4 lightMatrix, in sampler2D shadowMap,
                    in vec3 positionWS) {
    vec4 positionLS = lightMatrix * vec4(positionWS, 1.0);
    vec3 positionNDC = positionLS.xyz / positionLS.w;
    positionNDC = positionNDC * 0.5 + 0.5;
    float shadow = 0.0;
    float bias = 0.005;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0).xy;
    for (int i = -PCF_KERNEL_SIZE / 2; i <= PCF_KERNEL_SIZE / 2; i++) {
        for (int j = -PCF_KERNEL_SIZE / 2; j <= PCF_KERNEL_SIZE / 2; j++) {
            float shadowMapDepth =
                texture(shadowMap, positionNDC.xy + vec2(i, j) * texelSize).r;
            shadow += positionNDC.z - bias > shadowMapDepth ? 1.0 : 0.0;
        }
    }
    return shadow / float(PCF_KERNEL_SIZE * PCF_KERNEL_SIZE);
}

#endif /* HDSSS_SHADERS_INCLUDE_LIGHTING_GLSL */
