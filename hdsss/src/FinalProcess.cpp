#ifndef HDSSS_SRC_FINAL_PROCESS_HPP
#define HDSSS_SRC_FINAL_PROCESS_HPP
#include "FinalProcess.hpp"

#include <loo/glError.hpp>

#include "shaders/finalScreen.frag.hpp"
#include "shaders/finalScreen.vert.hpp"
using namespace loo;
FinalProcess::FinalProcess(int width, int height,
                           std::shared_ptr<loo::Quad> quad)
    : m_shader{{Shader(FINALSCREEN_VERT, GL_VERTEX_SHADER),
                Shader(FINALSCREEN_FRAG, GL_FRAGMENT_SHADER)}},
      m_width(width),
      m_height(height),
      m_quad(quad) {
    panicPossibleGLError();
}
void FinalProcess::init() { panicPossibleGLError(); }
void FinalProcess::render(const loo::Texture2D& screenTexture,
                          const loo::Texture2D& subsurfaceScattering,
                          bool directOutput) {
    Framebuffer::bindDefault();
    glClearColor(0, 0, 0, 1);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    // force fill the quad in the final step
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_shader.use();
    m_shader.setUniform("directOutput", directOutput);
    m_shader.setTexture(0, screenTexture);
    m_shader.setTexture(1, subsurfaceScattering);

    m_quad->draw();

    logPossibleGLError();
}


#endif /* HDSSS_SRC_FINAL_PROCESS_HPP */
