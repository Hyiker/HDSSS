#include "PBRMaterials.hpp"
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

void PBRMetallicMaterial::bind(const ShaderProgram& sp) {
    PBRMetallicMaterial::uniformBuffer->updateData(&m_shadermaterial);
    sp.setTexture(SHADER_BINDING_PORT_MR_BASECOLOR,
                  baseColorTex ? *baseColorTex : Texture2D::getWhiteTexture());
    sp.setTexture(SHADER_BINDING_PORT_MATERIAL_NORMAL,
                  normalTex ? *normalTex : Texture2D::getBlackTexture());
    sp.setTexture(SHADER_BINDING_PORT_MR_METALLIC,
                  metallicTex ? *metallicTex : Texture2D::getWhiteTexture());
    sp.setTexture(SHADER_BINDING_PORT_MR_ROUGHNESS,
                  roughnessTex ? *roughnessTex : Texture2D::getWhiteTexture());
    sp.setTexture(SHADER_BINDING_PORT_MR_OCCLUSION,
                  occlusionTex ? *occlusionTex : Texture2D::getWhiteTexture());
}

shared_ptr<PBRMetallicMaterial> PBRMetallicMaterial::getDefault() {
    if (!defaultMaterial) {
        defaultMaterial =
            std::make_shared<PBRMetallicMaterial>(vec3(0.5, 0, 0.5), 0.5, 0.5);
    }
    return defaultMaterial;
}
shared_ptr<PBRMetallicMaterial> PBRMetallicMaterial::defaultMaterial = nullptr;
unique_ptr<UniformBuffer> PBRMetallicMaterial::uniformBuffer = nullptr;

std::shared_ptr<PBRMetallicMaterial> convertPBRMetallicMaterialFromBaseMaterial(
    const loo::BaseMaterial& baseMaterial) {
    const auto& pbrMetallic = baseMaterial.mrWorkFlow;
    auto metallicMaterial = std::make_shared<PBRMetallicMaterial>(
        pbrMetallic.baseColor, pbrMetallic.metallic, pbrMetallic.roughness);
    metallicMaterial->baseColorTex = pbrMetallic.baseColorTex;
    metallicMaterial->normalTex = baseMaterial.normalTex;
    metallicMaterial->metallicTex = pbrMetallic.metallicTex;
    metallicMaterial->roughnessTex = pbrMetallic.roughnessTex;
    metallicMaterial->occlusionTex = pbrMetallic.occlusionTex;

    return metallicMaterial;
}