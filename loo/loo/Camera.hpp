#ifndef LOO_LOO_CAMERA_HPP
#define LOO_LOO_CAMERA_HPP

#include <glad/glad.h>

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "predefs.hpp"

namespace loo {
enum class CameraMovement { FORWARD, BACKWARD, LEFT, RIGHT };

class LOO_EXPORT Camera {
   public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    float yaw;
    float pitch;
    float speed{0.12f};
    float sensitivity{0.2f};
    float m_znear{0.01f};
    float m_zfar{50.f};
    float m_fov{float(M_PI) / 3.0f};
    float m_aspect;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f,
           float pitch = 0.0f, float aspect = 4.f / 3.f)
        : position(position),
          front(glm::vec3(0.0f, 0.0f, -1.0f)),
          worldUp{up},
          yaw(yaw),
          pitch(pitch),
          m_aspect{aspect} {
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const;

    glm::mat4 getProjectionMatrix() const;

    void getViewMatrix(glm::mat4& view) const;

    void getProjectionMatrix(glm::mat4& projection) const;

    glm::vec3 getPosition() const;

    void processKeyboard(CameraMovement direction, float deltaTime);

    void processMouseMovement(float xoffset, float yoffset,
                              GLboolean constrainpitch = true);

    void processMouseScroll(float xoffset, float yoffset);

   private:
    void updateCameraVectors();
};
}  // namespace loo

#endif /* LOO_LOO_CAMERA_HPP */
