#ifndef HDSSS_INCLUDE_TRANSFORMS_HPP
#define HDSSS_INCLUDE_TRANSFORMS_HPP
#include <glm/glm.hpp>
struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 normalMatrix;
};

#endif /* HDSSS_INCLUDE_TRANSFORMS_HPP */
