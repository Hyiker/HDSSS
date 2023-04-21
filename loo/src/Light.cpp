#include "loo/Light.hpp"

namespace loo {
using namespace std;
using namespace glm;
void ShaderLight::setDirection(const glm::vec3& d) {
    direction = glm::vec4(glm::normalize(d), 1);
}
ShaderLight createDirectionalLight(const glm::vec3& d, const glm::vec3& c,
                                   float intensity) {
    ShaderLight direction;
    direction.type = DIRECTIONAL;
    direction.setDirection(d);
    direction.setColor(c);
    direction.intensity = intensity;
    return direction;
}
}  // namespace loo