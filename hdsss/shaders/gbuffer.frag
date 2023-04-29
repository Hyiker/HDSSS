#version 460 core
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;
// tangent space -> world space
layout(location = 3) in vec3 vTangent;
layout(location = 4) in vec3 vBitangent;

layout(location = 0) out vec4 FragPosition;
layout(location = 1) out vec3 FragNormal;
layout(location = 2) out vec4 FragAlbedo;
layout(location = 3) out vec4 FragTransparentIOR;
layout(location = 4) out vec2 OcclusionRoughness;

uniform vec3 uCameraPosition;

uniform bool enableNormal;
uniform bool enableParallax;
uniform bool enableLodVisualize;
uniform int meshLod;
uniform bool applySSS;
const float ambientIntensity = 0.01f;
#ifdef MATERIAL_PBR
layout(std140, binding = 3) uniform PBRMetallicMaterial {
    vec4 baseColorMetallic;
    float roughness;
}
simpleMaterial;
layout(binding = 10) uniform sampler2D baseColorTex;
layout(binding = 11) uniform sampler2D occlusionTex;
layout(binding = 12) uniform sampler2D metallicTex;
layout(binding = 13) uniform sampler2D roughnessTex;

void GBufferFromSimpleMaterial(in vec2 texCoord, in sampler2D baseColorTex,
                               in sampler2D occlusionTex,
                               in sampler2D metallicTex,
                               in sampler2D roughnessTex, in vec3 baseColor,
                               in float metallic, in float roughness,
                               out vec4 GBufferAlbedo,
                               out vec2 GBufferOcclusionRoughness) {
    GBufferAlbedo.rgb = texture(baseColorTex, texCoord).rgb * baseColor;
    GBufferAlbedo.a = texture(metallicTex, texCoord).r * metallic;
    GBufferOcclusionRoughness.r =
        texture(roughnessTex, texCoord).r * roughness;  // roughness
    GBufferOcclusionRoughness.g = texture(occlusionTex, texCoord).r;
}
#else
layout(std140, binding = 2) uniform SimpleMaterial {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    vec4 transparentIOR;
    float shininess;
}
simpleMaterial;
layout(binding = 3) uniform sampler2D ambientTex;
layout(binding = 4) uniform sampler2D diffuseTex;
layout(binding = 5) uniform sampler2D specularTex;
layout(binding = 6) uniform sampler2D displacementTex;
layout(binding = 8) uniform sampler2D opacityTex;
layout(binding = 9) uniform sampler2D heightTex;

void GBufferFromSimpleMaterial(in vec2 texCoord, in sampler2D diffuseTex,
                               in sampler2D specularTex, in vec3 diffuse,
                               in vec3 specular, in vec4 transparentIOR,
                               out vec4 GBufferAlbedo,
                               out vec4 GBufferTransparentIOR) {
    GBufferAlbedo.rgb = texture(diffuseTex, texCoord).rgb * diffuse.rgb;
    GBufferAlbedo.a = texture(specularTex, texCoord).a * specular.r;
    GBufferTransparentIOR.rgba = transparentIOR.rgba;
}

#endif

layout(binding = 7) uniform sampler2D normalTex;

// vec2 parallaxMapping(vec2 texCoord, vec3 viewTS) {
//     float height = texture(heightTex, texCoord).r;
//     vec2 p = viewTS.xy / viewTS.z * (height * 0.2);
//     return texCoord + p;
// }

void main() {
    if (enableLodVisualize) {
        switch (meshLod) {
            case 0:
                FragAlbedo.rgb = vec3(0.50, 0, 0.15);
                break;
            case 1:
                FragAlbedo.rgb = vec3(1, 0.31, 0.16);
                break;
            case 2:
                FragAlbedo.rgb = vec3(1, 1, 0.8);
                break;
        }
        return;
    }

    vec3 V = normalize(uCameraPosition - vPos);
    vec3 color = vec3(0);
    mat3 TBN =
        mat3(normalize(vTangent), normalize(vBitangent), normalize(vNormal));
    vec2 texCoord = vTexCoord;

    // shading normal
    vec3 sNormal = texture(normalTex, texCoord).rgb;
    sNormal = length(sNormal) == 0.0 ? vNormal : (TBN * (sNormal * 2.0 - 1.0));
    sNormal = normalize(enableNormal ? sNormal : vNormal);
    float opacity = texture(
#ifdef MATERIAL_PBR
                        baseColorTex
#else
                        diffuseTex
#endif
                        ,
                        texCoord)
                        .a;
    if (opacity == 0.0) {
        // just discard all transparent fragments
        discard;
    }
    FragPosition = vec4(vPos, 1);
    FragNormal = sNormal;
#ifdef MATERIAL_PBR
    GBufferFromSimpleMaterial(
        texCoord, baseColorTex, occlusionTex, metallicTex, roughnessTex,
        simpleMaterial.baseColorMetallic.rgb,
        simpleMaterial.baseColorMetallic.a, simpleMaterial.roughness,
        FragAlbedo, OcclusionRoughness);
#else
    GBufferFromSimpleMaterial(
        texCoord, diffuseTex, specularTex, simpleMaterial.diffuse.rgb,
        simpleMaterial.specular.rgb, simpleMaterial.transparentIOR, FragAlbedo,
        FragTransparentIOR);
#endif
}