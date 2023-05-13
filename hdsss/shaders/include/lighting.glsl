#ifndef HDSSS_SHADERS_INCLUDE_LIGHTING_HPP
#define HDSSS_SHADERS_INCLUDE_LIGHTING_HPP
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

struct SurfaceParamsBlinnPhong {
    vec3 normal;
    vec4 albedo;
    float shininess;
};

struct SurfaceParamsPBRMetallicRoughness {
    vec3 viewDirection;

    vec3 normal;
    vec3 baseColor;
    float metallic;
    float roughness;
};

const int LIGHT_TYPE_SPOT = 0, LIGHT_TYPE_POINT = 1, LIGHT_TYPE_DIRECTIONAL = 2;

float DistributionGGX(in vec3 N, in float roughness, in vec3 H) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

vec3 FresnelSchlickApprox(in vec3 F0, in vec3 V, in vec3 H) {
    float VoH = max(0.001, dot(V, H));
    float F = pow(1.0 - VoH, 5.0);
    return F0 + (1.0 - F0) * F;
}
float SchlickGGXGeometry(in float roughness, in vec3 L, in vec3 V, in vec3 N) {
    float alpha = (roughness + 1) * 0.5;
    alpha *= alpha;
    float k = alpha / 2.0;
    float VoN = max(0.001, dot(V, N));
    float LoN = max(0.001, dot(L, N));
    float G1V = VoN / (VoN * (1.0 - k) + k);
    float G1L = LoN / (LoN * (1.0 - k) + k);
    return G1V * G1L;
}

float PBRCookTorranceBRDF(in SurfaceParamsPBRMetallicRoughness surface,
                          in ShaderLight light, in vec3 L) {
    vec3 H = safeNormalize(L + surface.viewDirection);
    float NDF = DistributionGGX(surface.normal, surface.roughness, H);
    float G = SchlickGGXGeometry(surface.roughness, L, surface.viewDirection,
                                 surface.normal);
    return NDF * G /
           (4.0 * max(0.001, dot(surface.normal, L)) *
            max(0.001, dot(surface.normal, surface.viewDirection)));
}
float PBRDiffuseStrength(in SurfaceParamsPBRMetallicRoughness surface,
                         in ShaderLight light, in vec3 L) {
    vec3 h = safeNormalize(L + surface.viewDirection);
    float F90 = 0.5 + 2 * surface.roughness * dot(h, L);
    float f_diff =
        (1 + (F90 - 1) * pow(1 - dot(surface.normal, L), 5)) *
        (1 +
         (F90 - 1) * pow(1 - dot(surface.normal, surface.viewDirection), 5)) /
        3.1415926;
    return f_diff;
}

vec3 SpecularStrength(in vec3 N, in float roughness, in vec3 L, in vec3 H) {
    float nh2 = sqr(clamp01(dot(N, H)));
    float lh2 = sqr(clamp01(dot(L, H)));
    float r2 = sqr(roughness);
    float d2 = sqr(nh2 * (r2 - 1.0) + 1.00001);
    float normalization = roughness * 4.0 + 2.0;
    return r2 / (d2 * max(vec3(0.1), lh2) * normalization);
}

void computePBRMetallicRoughnessLocalLighting(
    in SurfaceParamsPBRMetallicRoughness surface, in ShaderLight light,
    in vec3 V, in vec3 L, in float intensity, out vec3 diffuse,
    out vec3 specular) {
    vec3 N = surface.normal;
    vec3 baseColor = surface.baseColor;
    vec3 H = normalize(V + L);

    vec3 F0 = mix(vec3(0.04), surface.baseColor, surface.metallic);
    vec3 F = FresnelSchlickApprox(F0, surface.viewDirection, H);

    float NdotL = max(0.0, dot(N, L));
    vec3 kD = (1.0 - F) * (1.0 - surface.metallic);
    vec3 radiance = light.color.rgb * intensity;
    // diffuse = kD * baseColor * radiance * NdotL * PI_INV;
    diffuse = PBRDiffuseStrength(surface, light, L) * radiance * NdotL * PI_INV;
    // specular = PBRCookTorranceBRDF(surface, light, L) * radiance;
    specular = SpecularStrength(N, surface.roughness, L, H) * radiance * 0.02;
}

void computeBlinnPhongLocalLighting(in SurfaceParamsBlinnPhong surfaceParams,
                                    in ShaderLight light, in vec3 V, in vec3 L,
                                    in float intensity, out vec3 diffuse,
                                    out vec3 specular) {
    vec3 N = surfaceParams.normal;
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

#endif /* HDSSS_SHADERS_INCLUDE_LIGHTING_HPP */
