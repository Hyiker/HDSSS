#ifndef LOO_LOO_ATOMIC_COUNTER_HPP
#define LOO_LOO_ATOMIC_COUNTER_HPP
#include <glad/glad.h>

#include "loo.hpp"
namespace loo {

class LOO_EXPORT AtomicCounter {
   public:
    AtomicCounter(int bindPoint, int nCounters = 1);
    AtomicCounter(UniformBuffer&) = delete;
    AtomicCounter(AtomicCounter&& other);
    void resetCounters();
    void resetCounter(int index = 0);
    void getCounters(GLuint* ptr) const;
    GLuint getCounter(int index = 0) const;

    ~AtomicCounter() {
        if (m_handle != GL_INVALID_INDEX) glDeleteBuffers(1, &m_handle);
    }

   private:
    int m_bindpoint{-1};
    size_t m_ncounters{0};
    GLuint m_handle{GL_INVALID_INDEX};
};
}  // namespace loo
#endif /* LOO_LOO_ATOMIC_COUNTER_HPP */
