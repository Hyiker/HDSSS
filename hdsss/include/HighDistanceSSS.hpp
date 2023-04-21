#ifndef HDSSS_INCLUDE_HIGH_DISTANCE_SSS_HPP
#define HDSSS_INCLUDE_HIGH_DISTANCE_SSS_HPP
#include <loo/AtomicCounter.hpp>
#include <loo/Framebuffer.hpp>
#include <loo/Quad.hpp>
#include <loo/Shader.hpp>
#include <loo/ShaderStorageBuffer.hpp>
#include <loo/Texture.hpp>
#include <memory>

#include "GaussianBlur.hpp"
#include "Surfel.hpp"

class HighDistanceSSS {

    int m_width, m_height;

   public:
    HighDistanceSSS(int width, int height);
};

#endif /* HDSSS_INCLUDE_HIGH_DISTANCE_SSS_HPP */
