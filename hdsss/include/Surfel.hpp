#ifndef HDSSS_INCLUDE_SURFEL_HPP
#define HDSSS_INCLUDE_SURFEL_HPP
#include <glm/glm.hpp>
struct SurfelData {
    // use vec4 to get it aligned
    glm::vec4 position;
    glm::vec3 normal;
    float radius;
};
#endif /* HDSSS_INCLUDE_SURFEL_HPP */
