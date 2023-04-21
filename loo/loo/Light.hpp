#ifndef LOO_LOO_LIGHT_HPP
#define LOO_LOO_LIGHT_HPP
#include <glm/glm.hpp>

#include "UniformBuffer.hpp"
#include "loo.hpp"
namespace loo {
enum LightType { SPOT = 0, POINT = 1, DIRECTIONAL = 2 };
struct ShaderLight {
    // spot, point
    glm::vec4 position;
    // spot, directional
    // directional base orientation (0, 1, 0)
    glm::vec4 direction;
    // all
    glm::vec4 color;
    float intensity;
    // point, spot
    // negative value stands for INF
    float range;
    // spot
    float spotAngle;
    int type;
    void setPosition(const glm::vec3& p) { position = glm::vec4(p, 1); }
    void setDirection(const glm::vec3& d);
    void setColor(const glm::vec3& c) { color = glm::vec4(c, 1); }
};
// TODO
// ShaderLight createSpotLight(const glm::vec3& p, const glm::vec3& o,
//                             const glm::vec3& c, float intensity = 1.0,
//                             float range = -1.0, float spotAngle = 45.f);

// ShaderLight createPointLight(const glm::vec3& p, const glm::vec3& c,
//                              float intensity = 1.0, float range = -1.0);

ShaderLight createDirectionalLight(const glm::vec3& d, const glm::vec3& c,
                                   float intensity = 1.0);
}  // namespace loo

#endif /* LOO_LOO_LIGHT_HPP */
