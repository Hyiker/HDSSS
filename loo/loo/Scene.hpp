#ifndef LOO_LOO_SCENE_HPP
#define LOO_LOO_SCENE_HPP
#include <glad/glad.h>

#include <functional>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <string>
#include <vector>

#include "Mesh.hpp"
#include "Shader.hpp"
#include "predefs.hpp"

namespace loo {
constexpr int DRAW_FLAG_UPDATE_LOD = 0x1, DRAW_FLAG_TESSELLATION = 0x2;
class LOO_EXPORT Scene {
    std::vector<std::shared_ptr<Mesh>> m_meshes;
    glm::mat4 m_modelmat{1.0};
    GLuint m_queryid;

   public:
    void scale(glm::vec3 ratio);
    void translate(glm::vec3 pos);
    void prepare() const;
    glm::mat4 getModelMatrix() const;
    auto& getMeshes() const { return m_meshes; }

    // +++++ debug use +++++
    size_t countMesh() const;
    size_t countTriangle() const;
    size_t countVertex() const;
    // +++++ debug use +++++

    void addMeshes(std::vector<std::shared_ptr<Mesh>>&& meshes);

    void draw(ShaderProgram& sp,
              std::function<void(const Scene&, const Mesh&)> beforeDraw,
              GLenum drawMode = GL_FILL,
              int drawFlags = DRAW_FLAG_UPDATE_LOD) const;

    void draw(ShaderProgram& sp, GLenum drawMode = GL_FILL,
              int drawFlags = DRAW_FLAG_UPDATE_LOD) const;
    Scene();
    ~Scene();
};

LOO_EXPORT Scene createSceneFromFile(const std::string& filename);

LOO_EXPORT glm::mat4 getLightSpaceTransform(glm::vec3 lightPosition);
}  // namespace loo

#endif /* LOO_LOO_SCENE_HPP */
