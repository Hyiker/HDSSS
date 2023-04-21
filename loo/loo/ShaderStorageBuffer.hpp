#ifndef LOO_LOO_SHADER_STORAGE_BUFFER_HPP
#define LOO_LOO_SHADER_STORAGE_BUFFER_HPP

#include <glad/glad.h>

#include "loo.hpp"
namespace loo {

class ShaderStorageBuffer {
   public:
    ShaderStorageBuffer(int bindPoint, size_t dataSize,
                        void* dataPtr = nullptr);
    ShaderStorageBuffer(ShaderStorageBuffer&) = delete;
    ShaderStorageBuffer(ShaderStorageBuffer&& other);
    void readData(int offset, size_t dataSize, void* dataPtr) const {
        glGetNamedBufferSubData(m_handle, offset, dataSize, dataPtr);
    }
    void readData(void* dataPtr) const {
        glGetNamedBufferSubData(m_handle, 0, m_datasize, dataPtr);
    }
    void updateData(int offset, size_t dataSize, void* dataPtr) const {
        glNamedBufferSubData(m_handle, offset, dataSize, dataPtr);
    }

    void updateData(void* dataPtr) const {
        glNamedBufferSubData(m_handle, 0, m_datasize, dataPtr);
    }
    void getData(void* dstPtr, int offset, int size) const {
        glGetNamedBufferSubData(m_handle, offset, size, dstPtr);
    }
    auto getId() const { return m_handle; }
    auto getSize() const { return m_datasize; }

    ~ShaderStorageBuffer() {
        if (m_handle != GL_INVALID_INDEX) glDeleteBuffers(1, &m_handle);
    }

   private:
    int m_bindpoint{-1};
    size_t m_datasize{0};
    GLuint m_handle{GL_INVALID_INDEX};
};
}  // namespace loo

#endif /* LOO_LOO_SHADER_STORAGE_BUFFER_HPP */
