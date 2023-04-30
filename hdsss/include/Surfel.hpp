#ifndef HDSSS_INCLUDE_SURFEL_HPP
#define HDSSS_INCLUDE_SURFEL_HPP
#include <glm/glm.hpp>
struct SurfelData {
    glm::vec3 position;
    glm::vec3 normal;
    float radius;
    glm::vec3 sigma_t;
    glm::vec3 sigma_a;
};
#endif /* HDSSS_INCLUDE_SURFEL_HPP */
