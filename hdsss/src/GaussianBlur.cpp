#include "GaussianBlur.hpp"

#include "shaders/gaussianblur.frag.hpp"
#include "shaders/gaussianblur.vert.hpp"
#include "shaders/gaussianblurmultilayer.frag.hpp"
#include "shaders/gaussianblurmultilayer.vert.hpp"
using namespace loo;
GaussianBlur::GaussianBlur()
    : m_gbshader{Shader(GAUSSIANBLUR_VERT, ShaderType::Vertex),
                 Shader(GAUSSIANBLUR_FRAG, ShaderType::Fragment)} {}
// attention: `ping` should contain the original input
// while there's no additional requirements for `pong`
void GaussianBlur::init(std::unique_ptr<loo::Texture2D> ping,
                        std::unique_ptr<loo::Texture2D> pong) {
    this->m_pingpongtex[0] = std::move(ping);
    this->m_pingpongtex[1] = std::move(pong);
    m_fb.init();
    m_fb.attachTexture(*this->m_pingpongtex[1], GL_COLOR_ATTACHMENT0, 0);
}
void GaussianBlur::blur(const loo::Quad& quad,
                        GaussianBlurDirection direction) {
    int ping = m_outputindex, pong = 1 ^ m_outputindex;
    m_fb.bind();
    m_gbshader.use();
    m_gbshader.setTexture(0, *m_pingpongtex[ping]);
    m_gbshader.setUniform("axisX", direction == GaussianBlurDirection::X);
    m_fb.attachTexture(*m_pingpongtex[pong], GL_COLOR_ATTACHMENT0, 0);
    m_fb.enableAttachments({GL_COLOR_ATTACHMENT0});
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    quad.draw();
    m_outputindex = pong;
}

void GaussianBlur::blurFrom(const loo::Quad& quad,
                            const loo::Texture2D& originalTex,
                            GaussianBlurDirection direction) {
    int pong = 0;
    m_fb.bind();
    m_gbshader.use();
    m_gbshader.setTexture(0, originalTex);
    m_gbshader.setUniform("axisX", direction == GaussianBlurDirection::X);
    m_fb.attachTexture(*m_pingpongtex[pong], GL_COLOR_ATTACHMENT0, 0);
    m_fb.enableAttachments({GL_COLOR_ATTACHMENT0});
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    quad.draw();
    m_outputindex = pong;
}

GaussianBlurMultilayer::GaussianBlurMultilayer()
    : m_gbshader{Shader(GAUSSIANBLURMULTILAYER_VERT, ShaderType::Vertex),
                 Shader(GAUSSIANBLURMULTILAYER_FRAG, ShaderType::Fragment)} {}
// attention: `ping` should contain the original input
// while there's no additional requirements for `pong`
void GaussianBlurMultilayer::init(std::unique_ptr<loo::Texture2DArray> ping,
                                  std::unique_ptr<loo::Texture2DArray> pong) {
    this->m_pingpongtex[0] = std::move(ping);
    this->m_pingpongtex[1] = std::move(pong);
    m_fb.init();
    m_fb.attachTexture(*this->m_pingpongtex[1], GL_COLOR_ATTACHMENT0, 0);
}
void GaussianBlurMultilayer::blur(const loo::Quad& quad, int layers,
                                  GaussianBlurDirection direction) {
    int ping = m_outputindex, pong = 1 ^ m_outputindex;
    m_fb.bind();
    m_gbshader.use();
    m_gbshader.setTexture(0, *m_pingpongtex[ping]);
    m_gbshader.setUniform("axisX", direction == GaussianBlurDirection::X);
    m_fb.attachTexture(*m_pingpongtex[pong], GL_COLOR_ATTACHMENT0, 0);
    m_fb.enableAttachments({GL_COLOR_ATTACHMENT0});
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    quad.drawInstances(layers);
    m_outputindex = pong;
}