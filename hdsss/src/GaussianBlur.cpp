#include "GaussianBlur.hpp"

#include "shaders/gaussianblur.frag.hpp"
#include "shaders/gaussianblur.vert.hpp"
using namespace loo;
GaussianBlur::GaussianBlur()
    : m_gbshader{Shader(GAUSSIANBLUR_VERT, ShaderType::Vertex),
                 Shader(GAUSSIANBLUR_FRAG, ShaderType::Fragment)} {}
// attention: `ping` should contain the original input
// while there's no additional requirements for `pong`
void GaussianBlur::init(std::shared_ptr<loo::Texture2DArray> ping,
                        std::shared_ptr<loo::Texture2DArray> pong) {
    this->m_pingpongtex[0] = ping;
    this->m_pingpongtex[1] = pong;
    m_fb.init();
    m_fb.attachTexture(*pong, GL_COLOR_ATTACHMENT0, 0);
}
void GaussianBlur::blur(const loo::Quad& quad, int layers,
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