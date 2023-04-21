#ifndef HDSSS_INCLUDE_GAUSSIAN_BLUR_HPP
#define HDSSS_INCLUDE_GAUSSIAN_BLUR_HPP
#include <glad/glad.h>

#include <loo/Framebuffer.hpp>
#include <loo/Quad.hpp>
#include <loo/Shader.hpp>
#include <loo/Texture.hpp>
#include <memory>
enum class GaussianBlurDirection { X = 0, Y = 1 };
class GaussianBlur {
    loo::ShaderProgram m_gbshader;
    loo::Framebuffer m_fb;
    std::shared_ptr<loo::Texture2DArray> m_pingpongtex[2];
    // specifying which texture contains the final output currently
    int m_outputindex{0};

   public:
    GaussianBlur();
    // attention: `ping` should contain the original input
    // while there's no additional requirements for `pong`
    void init(std::shared_ptr<loo::Texture2DArray> ping,
              std::shared_ptr<loo::Texture2DArray> pong);
    void blur(const loo::Quad& quad, int layers,
              GaussianBlurDirection direction);
    auto getBlurResult() { return m_pingpongtex[m_outputindex]; }
};

#endif /* HDSSS_INCLUDE_GAUSSIAN_BLUR_HPP */
