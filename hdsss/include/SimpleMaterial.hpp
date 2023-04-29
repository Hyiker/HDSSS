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
struct ShaderSimpleMaterial {
    // std140 pad vec3 to 4N(N = 4B)
    // using vec4 to save your day
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;

    // vec3 transparent + float ior
    glm::vec4 transparentIOR;
    float shininess;
    ShaderSimpleMaterial(glm::vec3 ambient, glm::vec3 diffuse,
                         glm::vec3 specular, glm::vec3 transparent, float ior,
                         float shininess)
        : ambient(ambient, 1),
          diffuse(diffuse, 1),
          specular(specular, 1),
          transparentIOR(transparent, ior),
          shininess(shininess) {}
};
class SimpleMaterial : public loo::Material {
    ShaderSimpleMaterial m_shadermaterial;
    static std::shared_ptr<SimpleMaterial> defaultMaterial;
    // global uniform buffer for SimpleMaterial
    static std::unique_ptr<loo::UniformBuffer> uniformBuffer;

   public:
    ShaderSimpleMaterial& getShaderMaterial() { return m_shadermaterial; }
    SimpleMaterial(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular,
                   glm::vec3 transparent, float ior, float shininess)
        : m_shadermaterial(ambient, diffuse, specular, transparent, ior,
                           shininess) {
        if (SimpleMaterial::uniformBuffer == nullptr) {
            SimpleMaterial::uniformBuffer =
                std::make_unique<loo::UniformBuffer>(
                    SHADER_BINDING_PORT_SM_PARAM, sizeof(ShaderSimpleMaterial));
        }
    }
    static std::shared_ptr<SimpleMaterial> getDefault();
    void bind(const loo::ShaderProgram& sp) override;
    std::shared_ptr<loo::Texture2D> ambientTex{};
    std::shared_ptr<loo::Texture2D> diffuseTex{};
    std::shared_ptr<loo::Texture2D> displacementTex{};
    std::shared_ptr<loo::Texture2D> normalTex{};
    std::shared_ptr<loo::Texture2D> specularTex{};
    std::shared_ptr<loo::Texture2D> opacityTex{};
    std::shared_ptr<loo::Texture2D> heightTex{};
};
std::shared_ptr<SimpleMaterial> convertSimpleMaterialFromBaseMaterial(
    const loo::BaseMaterial& baseMaterial);
#endif /* HDSSS_INCLUDE_SIMPLE_MATERIAL_HPP */
