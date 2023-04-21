#ifndef LOO_LOO_UNIFORM_BUFFER_HPP
#define LOO_LOO_UNIFORM_BUFFER_HPP
#include <glad/glad.h>

#include "loo.hpp"
namespace loo {

class LOO_EXPORT UniformBuffer {
   public:
    UniformBuffer(int bindPoint, size_t dataSize, void* dataPtr = nullptr);
    UniformBuffer(UniformBuffer&) = delete;
    UniformBuffer(UniformBuffer&& other);
    void updateData(int offset, size_t dataSize, void* dataPtr) const {
        glNamedBufferSubData(m_handle, offset, dataSize, dataPtr);
    }

    void updateData(void* dataPtr) const {
        glNamedBufferSubData(m_handle, 0, m_datasize, dataPtr);
    }

    ~UniformBuffer() {
        if (m_handle != GL_INVALID_INDEX) glDeleteBuffers(1, &m_handle);
    }

   private:
    int m_bindpoint{-1};
    size_t m_datasize{0};
    GLuint m_handle{GL_INVALID_INDEX};
};
}  // namespace loo

#endif /* LOO_LOO_UNIFORM_BUFFER_HPP */
