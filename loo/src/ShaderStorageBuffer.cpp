#include "loo/ShaderStorageBuffer.hpp"

#include "loo/glError.hpp"
namespace loo {
ShaderStorageBuffer::ShaderStorageBuffer(int bindPoint, size_t dataSize,
                                         void* dataPtr)
    : m_bindpoint(bindPoint), m_datasize(dataSize) {
#ifdef OGL_46
    glCreateBuffers(1, &m_handle);
    glNamedBufferData(m_handle, dataSize, dataPtr, GL_DYNAMIC_COPY);
    logPossibleGLError();
    // if we directly specify binding port,
    // we need glShaderStorageBlockBinding no more
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindPoint, m_handle);
    logPossibleGLError();
#else
    NOT_IMPLEMENTED();
#endif
}

ShaderStorageBuffer::ShaderStorageBuffer(ShaderStorageBuffer&& other) {
    this->m_handle = other.m_handle;
    other.m_handle = GL_INVALID_INDEX;
}

}  // namespace loo