#include "loo/Light.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "loo/glError.hpp"

namespace loo {
using namespace std;
using namespace glm;
void ShaderLight::setDirection(const glm::vec3& d) {
    direction = glm::vec4(glm::normalize(d), 1);
}
glm::mat4 ShaderLight::getLightSpaceMatrix() const {
    switch (type) {
        case DIRECTIONAL: {
            glm::vec3 up{0.0f, 1.0f, 0.0f};
            if (direction.x == 0.0f && direction.z == 0.0f)
                up = glm::vec3(1.0f, 0.0f, 0.0f);
            return glm::ortho<float>(-0.13f, 0.13f, -0.13f, 0.13f, -0.2f,
                                     0.2f) *
                   glm::lookAt(glm::vec3(0, 0, 0),
                               glm::normalize(glm::vec3(direction)), up);
        }
        default: {
            NOT_IMPLEMENTED_RUNTIME();
        }
    }
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