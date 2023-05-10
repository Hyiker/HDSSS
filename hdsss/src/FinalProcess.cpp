#ifndef HDSSS_SRC_FINAL_PROCESS_HPP
#define HDSSS_SRC_FINAL_PROCESS_HPP
#include "FinalProcess.hpp"

#include <loo/glError.hpp>

#include "shaders/finalScreen.frag.hpp"
#include "shaders/finalScreen.vert.hpp"
using namespace loo;
FinalProcess::FinalProcess(int width, int height)
    : m_shader{{Shader(FINALSCREEN_VERT, GL_VERTEX_SHADER),
                Shader(FINALSCREEN_FRAG, GL_FRAGMENT_SHADER)}},
      m_width(width),
      m_height(height) {
    panicPossibleGLError();
}
void FinalProcess::init() {
    panicPossibleGLError();
}
void FinalProcess::render(const loo::Texture2D& diffuseTexture,
                          const loo::Texture2D& specularTexture,
                          const loo::Texture2D& translucencyTexture,
                          const loo::Texture2D& sssTexture,
                          const loo::Texture2D& GBuffer3,
                          const loo::Texture2D& skyboxTexture,
                          const FinalPassOptions& options) {
    Framebuffer::bindDefault();
    glClearColor(0, 0, 0, 1);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    // force fill the quad in the final step
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_shader.use();
    m_shader.setUniform("directOutput", options.directOutput);
    m_shader.setUniform("enableDiffuse", options.diffuse);
    m_shader.setUniform("enableSpecular", options.specular);
    m_shader.setUniform("enableTranslucency", options.translucency);
    m_shader.setUniform("enableSSS", options.SSS);
    m_shader.setUniform("SSSStrength", options.SSSStrength);
    m_shader.setTexture(0, diffuseTexture);
    m_shader.setTexture(1, specularTexture);
    m_shader.setTexture(2, translucencyTexture);
    m_shader.setTexture(3, sssTexture);
    m_shader.setTexture(4, skyboxTexture);
    m_shader.setTexture(5, GBuffer3);

    Quad::globalQuad().draw();

    logPossibleGLError();
}

#endif /* HDSSS_SRC_FINAL_PROCESS_HPP */
