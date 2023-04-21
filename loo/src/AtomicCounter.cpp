#include "loo/AtomicCounter.hpp"

#include <vcruntime_string.h>

#include <vector>

#include "loo/glError.hpp"
namespace loo {
AtomicCounter::AtomicCounter(int bindPoint, int nCounters)
    : m_bindpoint(bindPoint), m_ncounters(nCounters) {
#ifdef OGL_46
    glCreateBuffers(1, &m_handle);
    std::vector<GLuint> val(nCounters, 0);
    glNamedBufferData(m_handle, sizeof(GLuint) * nCounters, nullptr,
                      GL_DYNAMIC_DRAW);
    logPossibleGLError();
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, bindPoint, m_handle);
    logPossibleGLError();
#else
    NOT_IMPLEMENTED();
#endif
}

AtomicCounter::AtomicCounter(AtomicCounter&& other) {
    this->m_handle = other.m_handle;
    other.m_handle = GL_INVALID_INDEX;
}
void AtomicCounter::resetCounters() {
    auto ptr = (GLuint*)glMapNamedBufferRange(
        m_handle, 0, m_ncounters * sizeof(GLuint),
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT |
            GL_MAP_UNSYNCHRONIZED_BIT);
    memset(ptr, 0, m_ncounters * sizeof(GLuint));
    glUnmapNamedBuffer(m_handle);
}
void AtomicCounter::resetCounter(int index) {
    auto ptr = (GLuint*)glMapNamedBufferRange(m_handle, index * sizeof(GLuint),
                                              sizeof(GLuint), GL_MAP_WRITE_BIT);
    ptr[index] = 0;
    glUnmapNamedBuffer(m_handle);
}
void AtomicCounter::getCounters(GLuint* ptr) const {
    glGetNamedBufferSubData(m_handle, 0, m_ncounters * sizeof(GLuint), ptr);
}
GLuint AtomicCounter::getCounter(int index) const {
    int val;
    glGetNamedBufferSubData(m_handle, index * sizeof(GLuint), sizeof(GLuint),
                            &val);
    return val;
}

}  // namespace loo