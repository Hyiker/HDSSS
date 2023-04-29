#include "SimpleMaterial.hpp"
#include <glog/logging.h>
#include "constants.hpp"
#include "loo/Material.hpp"

#include <memory>
#include <string>

#include <assimp/material.h>
#include <glm/fwd.hpp>
#include <loo/Shader.hpp>
using namespace std;
using namespace glm;
using namespace loo;
namespace fs = std::filesystem;

void SimpleMaterial::bind(const ShaderProgram& sp) {
    SimpleMaterial::uniformBuffer->updateData(&m_shadermaterial);
    sp.setTexture(SHADER_BINDING_PORT_SM_AMBIENT,
                  ambientTex ? *ambientTex : Texture2D::getWhiteTexture());
    sp.setTexture(SHADER_BINDING_PORT_SM_DIFFUSE,
                  diffuseTex ? *diffuseTex : Texture2D::getWhiteTexture());
    sp.setTexture(SHADER_BINDING_PORT_SM_SPECULAR,
                  specularTex ? *specularTex : Texture2D::getWhiteTexture());
    sp.setTexture(
        SHADER_BINDING_PORT_SM_DISPLACEMENT,
        displacementTex ? *displacementTex : Texture2D::getBlackTexture());
    sp.setTexture(SHADER_BINDING_PORT_MATERIAL_NORMAL,
                  normalTex ? *normalTex : Texture2D::getBlackTexture());
    sp.setTexture(SHADER_BINDING_PORT_SM_OPACITY,
                  opacityTex ? *opacityTex : Texture2D::getBlackTexture());
    sp.setTexture(SHADER_BINDING_PORT_SM_HEIGHT,
                  heightTex ? *heightTex : Texture2D::getBlackTexture());
}

shared_ptr<SimpleMaterial> SimpleMaterial::getDefault() {
    if (!defaultMaterial) {
        defaultMaterial = std::make_shared<SimpleMaterial>(
            glm::vec3(0, 0, 0), glm::vec3(1, 1, 1), glm::vec3(0, 0, 0),
            glm::vec3(0.0), 1.0, 200.0);
    }
    return defaultMaterial;
}
shared_ptr<SimpleMaterial> SimpleMaterial::defaultMaterial = nullptr;
unique_ptr<UniformBuffer> SimpleMaterial::uniformBuffer = nullptr;

std::shared_ptr<SimpleMaterial> convertSimpleMaterialFromBaseMaterial(
    const loo::BaseMaterial& baseMaterial) {
    const auto& blinnPhong = baseMaterial.bpWorkFlow;
    auto simpleMaterial = std::make_shared<SimpleMaterial>(
        blinnPhong.ambient, blinnPhong.diffuse, blinnPhong.specular,
        blinnPhong.transparent, blinnPhong.ior, blinnPhong.shininess);
    simpleMaterial->ambientTex = baseMaterial.ambientTex;
    simpleMaterial->diffuseTex = baseMaterial.diffuseTex;
    simpleMaterial->displacementTex = baseMaterial.displacementTex;
    simpleMaterial->normalTex = baseMaterial.normalTex;
    simpleMaterial->specularTex = baseMaterial.specularTex;
    simpleMaterial->opacityTex = baseMaterial.opacityTex;
    simpleMaterial->heightTex = baseMaterial.heightTex;
    return simpleMaterial;
}