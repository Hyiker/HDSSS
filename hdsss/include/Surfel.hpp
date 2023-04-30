#ifndef HDSSS_INCLUDE_SURFEL_HPP
#define HDSSS_INCLUDE_SURFEL_HPP
#include <glm/glm.hpp>
struct SurfelData {
    glm::vec4 position;
    glm::vec3 normal;
    float radius;
    // glm::vec3 sigma_t;
    // float pad0;
    // glm::vec3 sigma_a;
    // float pad1;
};
#endif /* HDSSS_INCLUDE_SURFEL_HPP */
