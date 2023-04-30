#include "loo/Material.hpp"

#include <glog/logging.h>

#include <memory>
#include <string>

#include <assimp/material.h>
#include <assimp/types.h>
#include <glm/fwd.hpp>
#include <glm/gtx/string_cast.hpp>
#include <loo/Shader.hpp>

namespace loo {

using namespace std;
using namespace glm;
using namespace loo;
namespace fs = std::filesystem;

static unordered_map<string, shared_ptr<Texture2D>> uniqueTexture;
static inline glm::vec3 aiColor3D2Glm(const aiColor3D& aColor) {
    return {aColor.r, aColor.g, aColor.b};
}

static shared_ptr<Texture2D> createMaterialTextures(
    const aiMaterial* mat, aiTextureType type, fs::path objParent,
    unsigned int options = TEXTURE_OPTION_MIPMAP |
                           TEXTURE_OPTION_CONVERT_TO_LINEAR) {
    vector<Texture2D> textures;
    if (mat->GetTextureCount(type)) {
        // TODO: support multilayer texture
        aiString str;
        mat->GetTexture(type, 0, &str);
        return createTexture2DFromFile(
            uniqueTexture, (objParent / str.C_Str()).string(), options);
    } else {
        return nullptr;
    }
}

static BlinnPhongWorkFlow createBlinnPhongWorkFlowFromAssimp(
    const aiMaterial* aMaterial, fs::path objParent) {
    aiColor3D color(0, 0, 0);
    aMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color);
    glm::vec3 ambient = aiColor3D2Glm(color);
    aMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    glm::vec3 diffuse = aiColor3D2Glm(color);
    aMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color);
    glm::vec3 specular = aiColor3D2Glm(color);
    color = aiColor3D(0, 0, 0);
    // using emissive to store transparent color
    // a hack for wavefront obj
    aMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color);
    glm::vec3 transparent = aiColor3D2Glm(color);
    float shininess, _ior;
    aMaterial->Get(AI_MATKEY_SHININESS, shininess);
    aMaterial->Get(AI_MATKEY_REFRACTI, _ior);
    return BlinnPhongWorkFlow(ambient, diffuse, specular, transparent, _ior,
                              shininess);
}

static MetallicRoughnessWorkFlow createMetallicRoughnessWorkFlowFromAssimp(
    const aiMaterial* aMaterial, fs::path objParent) {
    aiColor3D color(0, 0, 0);
    aMaterial->Get(AI_MATKEY_BASE_COLOR, color);
    glm::vec3 baseColor = aiColor3D2Glm(color);

    float metallic, roughness;
    aMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
    aMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);

    // TODO: AI_MATKEY_TRANSMISSION_FACTOR, AI_MATKEY_VOLUME_ATTENUATION_COLOR
    float transmission = 0.0, mfp = 0.0;
    aMaterial->Get(AI_MATKEY_TRANSMISSION_FACTOR, transmission);
    aMaterial->Get(AI_MATKEY_VOLUME_ATTENUATION_COLOR, color);
    glm::vec3 sigma_a = aiColor3D2Glm(color);
    aMaterial->Get(AI_MATKEY_VOLUME_ATTENUATION_DISTANCE, mfp);

    auto baseColorTex =
        createMaterialTextures(aMaterial, aiTextureType_BASE_COLOR, objParent);
    auto occlusionTex = createMaterialTextures(
        aMaterial, aiTextureType_AMBIENT_OCCLUSION, objParent);
    auto metallicTex =
        createMaterialTextures(aMaterial, aiTextureType_METALNESS, objParent);
    auto roughnessTex = createMaterialTextures(
        aMaterial, aiTextureType_DIFFUSE_ROUGHNESS, objParent);

    auto workflow =
        MetallicRoughnessWorkFlow(baseColor, metallic, roughness, transmission,
                                  glm::vec3(1 / mfp), sigma_a);
    workflow.baseColorTex = baseColorTex;
    workflow.occlusionTex = occlusionTex;
    workflow.metallicTex = metallicTex;
    workflow.roughnessTex = roughnessTex;
    return workflow;
}

std::shared_ptr<BaseMaterial> createBaseMaterialFromAssimp(
    const aiMaterial* aMaterial, fs::path objParent) {
    auto blinnPhong = createBlinnPhongWorkFlowFromAssimp(aMaterial, objParent);
    auto metallicRoughness =
        createMetallicRoughnessWorkFlowFromAssimp(aMaterial, objParent);
    auto material = make_shared<BaseMaterial>(blinnPhong, metallicRoughness);

    // read common textures
    material->ambientTex =
        createMaterialTextures(aMaterial, aiTextureType_AMBIENT, objParent);

    material->diffuseTex =
        createMaterialTextures(aMaterial, aiTextureType_DIFFUSE, objParent);

    material->specularTex =
        createMaterialTextures(aMaterial, aiTextureType_SPECULAR, objParent);

    material->displacementTex = createMaterialTextures(
        aMaterial, aiTextureType_DISPLACEMENT, objParent, 0x0);
    // material->displacementTex->setSizeFilter(GL_NEAREST, GL_NEAREST);
    // obj file saves normal map as bump maps
    // FUCK YOU, wavefront obj
    material->normalTex = createMaterialTextures(
        aMaterial, aiTextureType_NORMALS, objParent, TEXTURE_OPTION_MIPMAP);
    material->opacityTex = createMaterialTextures(
        aMaterial, aiTextureType_OPACITY, objParent, TEXTURE_OPTION_MIPMAP);
    material->heightTex = createMaterialTextures(
        aMaterial, aiTextureType_HEIGHT, objParent, TEXTURE_OPTION_MIPMAP);

    return material;
}
}  // namespace loo