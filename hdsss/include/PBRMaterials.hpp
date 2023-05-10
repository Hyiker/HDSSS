#ifndef HDSSS_INCLUDE_PBRMATERIALS_HPP
#define HDSSS_INCLUDE_PBRMATERIALS_HPP

#include <glog/logging.h>
#include <filesystem>
#include <glm/glm.hpp>
#include <loo/Material.hpp>
#include <memory>
#include "constants.hpp"

#include <assimp/types.h>
#include <loo/Texture.hpp>
#include <loo/UniformBuffer.hpp>
#include <loo/loo.hpp>
struct ShaderPBRMetallicMaterial {
    // std140 pad vec3 to 4N(N = 4B)
    // using vec4 to save your day
    glm::vec4 baseColorMetallic;
    // transmission(1) + sigmaT(3)
    glm::vec4 transmissionSigmaT;
    // sigmaA(3) + roughness(1)
    glm::vec4 sigmaARoughness;
    ShaderPBRMetallicMaterial(glm::vec3 baseColor, float metallic,
                              float transmission, glm::vec3 sigmaT,
                              glm::vec3 sigmaA, float roughness)
        : baseColorMetallic(baseColor, metallic),
          transmissionSigmaT(transmission, sigmaT),
          sigmaARoughness(sigmaA, roughness) {}
};
class PBRMetallicMaterial : public loo::Material {
    ShaderPBRMetallicMaterial m_shadermaterial;
    static std::shared_ptr<PBRMetallicMaterial> defaultMaterial;
    static std::shared_ptr<PBRMetallicMaterial> defaultSubsurfaceMaterial;
    // global uniform buffer for SimpleMaterial
    static std::unique_ptr<loo::UniformBuffer> uniformBuffer;

   public:
    static struct BSSRDFConfig {
        glm::vec3 sigma_t;
        glm::vec3 albedo;
    } bssrdf;
    ShaderPBRMetallicMaterial& getShaderMaterial() { return m_shadermaterial; }
    const ShaderPBRMetallicMaterial& getShaderMaterial() const {
        return m_shadermaterial;
    }
    PBRMetallicMaterial(glm::vec3 baseColor, float metallic, float transmission,
                        glm::vec3 sigmaT, glm::vec3 sigmaA, float roughness)
        : m_shadermaterial(baseColor, metallic, transmission, sigmaT, sigmaA,
                           roughness) {
        if (PBRMetallicMaterial::uniformBuffer == nullptr) {
            PBRMetallicMaterial::uniformBuffer =
                std::make_unique<loo::UniformBuffer>(
                    SHADER_BINDING_PORT_MR_PARAM, sizeof(PBRMetallicMaterial));
        }
    }
    static std::shared_ptr<PBRMetallicMaterial> getDefault();
    static std::shared_ptr<PBRMetallicMaterial> getDefaultSubsurface();
    void bind(const loo::ShaderProgram& sp) override;
    std::shared_ptr<loo::Texture2D> baseColorTex{};
    std::shared_ptr<loo::Texture2D> occlusionTex{};
    std::shared_ptr<loo::Texture2D> metallicTex{};
    std::shared_ptr<loo::Texture2D> roughnessTex{};
    std::shared_ptr<loo::Texture2D> normalTex{};
};
std::shared_ptr<PBRMetallicMaterial> convertPBRMetallicMaterialFromBaseMaterial(
    const loo::BaseMaterial& baseMaterial);

#endif /* HDSSS_INCLUDE_PBRMATERIALS_HPP */
