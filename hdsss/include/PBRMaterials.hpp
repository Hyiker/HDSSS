#ifndef HDSSS_INCLUDE_SIMPLE_MATERIAL_20COPY_HPP
#define HDSSS_INCLUDE_SIMPLE_MATERIAL_20COPY_HPP
#ifndef HDSSS_INCLUDE_SIMPLE_MATERIAL_HPP
#define HDSSS_INCLUDE_SIMPLE_MATERIAL_HPP
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
    float roughness;
    ShaderPBRMetallicMaterial(glm::vec3 baseColor, float metallic,
                              float roughness)
        : baseColorMetallic(baseColor, metallic), roughness(roughness) {}
};
class PBRMetallicMaterial : public loo::Material {
    ShaderPBRMetallicMaterial m_shadermaterial;
    static std::shared_ptr<PBRMetallicMaterial> defaultMaterial;
    // global uniform buffer for SimpleMaterial
    static std::unique_ptr<loo::UniformBuffer> uniformBuffer;

   public:
    ShaderPBRMetallicMaterial& getShaderMaterial() { return m_shadermaterial; }
    PBRMetallicMaterial(glm::vec3 baseColor, float metallic, float roughness)
        : m_shadermaterial(baseColor, metallic, roughness) {
        if (PBRMetallicMaterial::uniformBuffer == nullptr) {
            PBRMetallicMaterial::uniformBuffer =
                std::make_unique<loo::UniformBuffer>(
                    SHADER_BINDING_PORT_MR_PARAM, sizeof(PBRMetallicMaterial));
        }
    }
    static std::shared_ptr<PBRMetallicMaterial> getDefault();
    void bind(const loo::ShaderProgram& sp) override;
    std::shared_ptr<loo::Texture2D> baseColorTex{};
    std::shared_ptr<loo::Texture2D> occlusionTex{};
    std::shared_ptr<loo::Texture2D> metallicTex{};
    std::shared_ptr<loo::Texture2D> roughnessTex{};
    std::shared_ptr<loo::Texture2D> normalTex{};
};
std::shared_ptr<PBRMetallicMaterial> convertPBRMetallicMaterialFromBaseMaterial(
    const loo::BaseMaterial& baseMaterial);
#endif /* HDSSS_INCLUDE_SIMPLE_MATERIAL_HPP */

#endif /* HDSSS_INCLUDE_SIMPLE_MATERIAL_20COPY_HPP */
