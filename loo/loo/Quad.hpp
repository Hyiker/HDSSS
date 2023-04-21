#ifndef LOO_LOO_QUAD_HPP
#define LOO_LOO_QUAD_HPP
#include <glad/glad.h>

#include "predefs.hpp"

namespace loo {

class LOO_EXPORT Quad {
   private:
    GLuint vao, vbo;

   public:
    Quad();
    void draw() const;
    void drawInstances(int count) const;
    ~Quad();
};

}  // namespace loo

#endif /* LOO_LOO_QUAD_HPP */
