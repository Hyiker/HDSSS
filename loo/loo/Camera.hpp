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
    glm::vec3 position{0.f, 0.f, 0.f};
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp{0.f, 1.f, 0.f};
    glm::vec3 lookat{0.f, 0.f, 0.f};
    float yaw{-90.0f};
    float pitch{0.0f};
    float speed{0.12f};
    float sensitivity{0.2f};
    float m_znear{0.01f};
    float m_zfar{50.f};
    float m_fov{float(M_PI) / 3.0f};
    float m_aspect{4.f / 3.f};

    Camera() { updateCameraVectors(); }

    Camera(glm::vec3 position, glm::vec3 lookat, float fovRad, float zNear,
           float zFar)
        : position(position),
          lookat(lookat),
          m_znear(zNear),
          m_zfar(zFar),
          m_fov(fovRad) {
        front = glm::normalize(lookat - position);
        updatePitchAndYaw();

        updateCameraVectors();
    }
    void updatePitchAndYaw() {

        pitch = glm::degrees(asin(front.y));
        yaw = glm::degrees(atan2(front.z, front.x));
    }
    void updateCameraVectors();

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
};
}  // namespace loo

#endif /* LOO_LOO_CAMERA_HPP */
