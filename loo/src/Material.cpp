#include "loo/Material.hpp"

#include <glog/logging.h>

#include <memory>
#include <string>

#include <assimp/material.h>
#include <assimp/types.h>
#include <glm/fwd.hpp>
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
std::shared_ptr<BaseMaterial> createBaseMaterialFromAssimp(
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

    auto material = make_shared<BaseMaterial>(ambient, diffuse, specular,
                                              transparent, _ior, shininess);
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