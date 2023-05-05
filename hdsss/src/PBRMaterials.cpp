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
        defaultMaterial = std::make_shared<PBRMetallicMaterial>(
            vec3(0.5, 0, 0.5), 0.5, 0, vec3(0), vec3(1.0), 1);
    }
    return defaultMaterial;
}
shared_ptr<PBRMetallicMaterial> PBRMetallicMaterial::defaultMaterial = nullptr;
unique_ptr<UniformBuffer> PBRMetallicMaterial::uniformBuffer = nullptr;

std::shared_ptr<PBRMetallicMaterial> convertPBRMetallicMaterialFromBaseMaterial(
    const loo::BaseMaterial& baseMaterial) {
    const auto& pbrMetallic = baseMaterial.mrWorkFlow;
    const vec3 marblesigmaA = vec3(0.0021, 0.0041, 0.0071),
               marblesigmaS = vec3(2.19, 2.62, 2.00);
    auto metallicMaterial = std::make_shared<PBRMetallicMaterial>(
        pbrMetallic.baseColor, pbrMetallic.metallic, pbrMetallic.transmission,
        // TODO: use measured data, currently using the marble data
        length(pbrMetallic.sigma_t) == 0 ? pbrMetallic.sigma_t
                                         : marblesigmaA + marblesigmaS,
        length(pbrMetallic.sigma_a) == 0 ? pbrMetallic.sigma_a : marblesigmaA,
        // pbrMetallic.sigma_t, pbrMetallic.sigma_a,
        pbrMetallic.roughness);
    metallicMaterial->baseColorTex = pbrMetallic.baseColorTex;
    metallicMaterial->normalTex = baseMaterial.normalTex;
    metallicMaterial->metallicTex = pbrMetallic.metallicTex;
    metallicMaterial->roughnessTex = pbrMetallic.roughnessTex;
    metallicMaterial->occlusionTex = pbrMetallic.occlusionTex;

    return metallicMaterial;
}