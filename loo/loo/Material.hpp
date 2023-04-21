#ifndef LOO_LOO_MATERIAL_HPP
#define LOO_LOO_MATERIAL_HPP
#include "Shader.hpp"

#include <assimp/material.h>
#include <glog/logging.h>

#include <filesystem>
#include <glm/glm.hpp>
#include <memory>

#include <assimp/types.h>
#include <loo/Texture.hpp>
#include <loo/UniformBuffer.hpp>
#include <loo/loo.hpp>
namespace loo {
class LOO_EXPORT Material {
   public:
    // setup uniforms and textures for shader program
    virtual void bind(const ShaderProgram& sp) = 0;
};

struct BaseMaterial : public Material {

    void bind(const ShaderProgram& sp) override { NOT_IMPLEMENTED_RUNTIME(); }
    BaseMaterial(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular,
                 glm::vec3 ior, float shininess)
        : ambient(ambient),
          diffuse(diffuse),
          specular(specular),
          ior(ior),
          shininess(shininess) {}
    std::shared_ptr<loo::Texture2D> ambientTex{};
    std::shared_ptr<loo::Texture2D> diffuseTex{};
    std::shared_ptr<loo::Texture2D> displacementTex{};
    std::shared_ptr<loo::Texture2D> normalTex{};
    std::shared_ptr<loo::Texture2D> specularTex{};
    std::shared_ptr<loo::Texture2D> opacityTex{};
    std::shared_ptr<loo::Texture2D> heightTex{};
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    glm::vec3 ior;
    float shininess;
};
std::shared_ptr<loo::BaseMaterial> createBaseMaterialFromAssimp(
    const aiMaterial* aMaterial, std::filesystem::path objParent);
}  // namespace loo
#endif /* LOO_LOO_MATERIAL_HPP */
