#include "loo/Quad.hpp"
namespace loo {

Quad::Quad() {
    constexpr float quadVertices[] = {-1.0f, 1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,
                                      0.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f,

                                      -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  -1.0f,
                                      1.0f,  0.0f, 1.0f, 1.0f,  1.0f,  1.0f};
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

void Quad::draw() const {
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Quad::drawInstances(int count) const {
    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, count);
    glBindVertexArray(0);
}

Quad::~Quad() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

}  // namespace loo