#ifndef LOO_LOO_SKYBOX_HPP
#define LOO_LOO_SKYBOX_HPP
#include <glad/glad.h>

#include "loo.hpp"
namespace loo {
class LOO_EXPORT Skybox {
   private:
    GLuint vao, vbo;

   public:
    Skybox();
    void draw() const;
    ~Skybox();
};
}  // namespace loo
#endif /* LOO_LOO_SKYBOX_HPP */
