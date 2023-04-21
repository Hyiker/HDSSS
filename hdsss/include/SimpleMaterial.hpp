#ifndef HDSSS_INCLUDE_SIMPLE_MATERIAL_HPP
#define HDSSS_INCLUDE_SIMPLE_MATERIAL_HPP
#include <glog/logging.h>
#include <loo/Material.hpp>

#include <filesystem>
#include <glm/glm.hpp>
#include <memory>

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

    glm::vec4 ior;
    float shininess;
    ShaderSimpleMaterial(glm::vec3 ambient, glm::vec3 diffuse,
                         glm::vec3 specular, glm::vec3 ior, float shininess)
        : ambient(ambient, 1),
          diffuse(diffuse, 1),
          specular(specular, 1),
          ior(ior, 1),
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
                   glm::vec3 ior, float shininess)
        : m_shadermaterial(ambient, diffuse, specular, ior, shininess) {
        if (SimpleMaterial::uniformBuffer == nullptr) {
            SimpleMaterial::uniformBuffer =
                std::make_unique<loo::UniformBuffer>(
                    2, sizeof(ShaderSimpleMaterial));
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
