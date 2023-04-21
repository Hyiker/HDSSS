#include "loo/UniformBuffer.hpp"

#include "loo/glError.hpp"
namespace loo {
UniformBuffer::UniformBuffer(int bindPoint, size_t dataSize, void* dataPtr)
    : m_bindpoint(bindPoint), m_datasize(dataSize) {
#ifdef OGL_46
    glCreateBuffers(1, &m_handle);
    glNamedBufferData(m_handle, dataSize, dataPtr, GL_STATIC_DRAW);
    logPossibleGLError();
    glBindBufferBase(GL_UNIFORM_BUFFER, bindPoint, m_handle);
    logPossibleGLError();
#else
    NOT_IMPLEMENTED();
#endif
}

UniformBuffer::UniformBuffer(UniformBuffer&& other) {
    this->m_handle = other.m_handle;
    other.m_handle = GL_INVALID_INDEX;
}

}  // namespace loo