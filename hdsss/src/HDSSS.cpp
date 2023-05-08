#include "HDSSS.hpp"
#include <loo/Application.hpp>
#include "HDSSSApplication.hpp"
#include "Surfel.hpp"
#include "constants.hpp"
#include "ssss.frag.hpp"
#include "ssss.vert.hpp"
#include "surfelize.tesc.hpp"
#include "surfelize.tese.hpp"
#include "surfelize.vert.hpp"
#include "translucency.frag.hpp"
#include "translucency.geom.hpp"
#include "translucency.vert.hpp"
#include "upscale.frag.hpp"
#include "upscale.vert.hpp"
using namespace std;
using namespace loo;

void HDSSS::initTranslucencyPass() {
    initSurfelizePass();

    m_translucencyfb.init();

    m_translucencytex = make_unique<Texture2D>();
    m_translucencytex->init();
    m_translucencytex->setupStorage(Application::getContext()->getWidth() >> 2,
                                    Application::getContext()->getHeight() >> 2,
                                    GL_RGB32F, 1);
    m_translucencytex->setSizeFilter(GL_LINEAR, GL_LINEAR);

    m_translucencyfb.attachTexture(*m_translucencytex, GL_COLOR_ATTACHMENT0, 0);
    m_translucencyfb.enableAttachments({GL_COLOR_ATTACHMENT0});

    panicPossibleGLError();
}
void HDSSS::initSurfelizePass() {
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenTransformFeedbacks(1, &m_surfelizetf);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_surfelizetf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SurfelData) * N_SURFELS_MAX, nullptr,
                 GL_DYNAMIC_DRAW);
    panicPossibleGLError();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SurfelData),
                          (GLvoid*)offsetof(SurfelData, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SurfelData),
                          (GLvoid*)(offsetof(SurfelData, normal)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(SurfelData),
                          (GLvoid*)(offsetof(SurfelData, radius)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(SurfelData),
                          (GLvoid*)(offsetof(SurfelData, sigma_t)));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(SurfelData),
                          (GLvoid*)(offsetof(SurfelData, sigma_a)));
    glEnableVertexAttribArray(4);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbo);
    panicPossibleGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_surfelbuffer.vao = vao;
    m_surfelbuffer.vbo = vbo;
    panicPossibleGLError();
    glGenQueries(1, &m_surfelizequery);
}
void HDSSS::initUpscalePass() {
    m_upscalefb.init();
    m_upscaletex = make_unique<Texture2D>();
    m_upscaletex->init();
    m_upscaletex->setupStorage(Application::getContext()->getWidth(),
                               Application::getContext()->getHeight(),
                               GL_RGB32F, 1);
    m_upscaletex->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_upscalefb.attachTexture(*m_upscaletex, GL_COLOR_ATTACHMENT0, 0);
    m_upscalefb.enableAttachments({GL_COLOR_ATTACHMENT0});
    panicPossibleGLError();
}
void HDSSS::initSSSSPass() {
    m_ssssfb.init();
    m_sssstex = make_unique<Texture2D>();
    m_sssstex->init();
    m_sssstex->setupStorage(Application::getContext()->getWidth(),
                            Application::getContext()->getHeight(), GL_RGB32F,
                            1);
    m_sssstex->setSizeFilter(GL_LINEAR, GL_LINEAR);
    m_ssssfb.attachTexture(*m_sssstex, GL_COLOR_ATTACHMENT0, 0);
    m_ssssfb.enableAttachments({GL_COLOR_ATTACHMENT0});
    panicPossibleGLError();
}

HDSSS::HDSSS()
    : m_translucencyshader{Shader(TRANSLUCENCY_VERT, ShaderType::Vertex),
                           Shader(TRANSLUCENCY_GEOM, ShaderType::Geometry),
                           Shader(TRANSLUCENCY_FRAG, ShaderType::Fragment)},
      m_surfelizeshader(
          {
              Shader(SURFELIZE_VERT, ShaderType::Vertex),
              Shader(SURFELIZE_TESC, ShaderType::TessellationControl),
              Shader(SURFELIZE_TESE, ShaderType::TessellationEvaluation),
          },
          {"tePos", "teNormal", "teRadius", "teSigmaT", "teSigmaA"}),
      m_upscaleshader{
          Shader(UPSCALE_VERT, ShaderType::Vertex),
          Shader(UPSCALE_FRAG, ShaderType::Fragment),
      },
      m_ssssshader{
          Shader(SSSS_VERT, ShaderType::Vertex),
          Shader(SSSS_FRAG, ShaderType::Fragment),
      } {}
void HDSSS::init() {
    initTranslucencyPass();
    initUpscalePass();
    initSSSSPass();
}
// fourth pass: translucency effect
void HDSSS::translucencyPass(const loo::Scene& scene, MVP& mvp,
                             loo::UniformBuffer& mvpBuffer,
                             const loo::ShaderLight& mainLight,
                             const loo::Texture2D& GBufferPosition,
                             const loo::Texture2D& GBufferNormal,
                             const loo::Texture2D& mainLightShadowMap) {
    surfelizePass(scene, mvp, mvpBuffer);

    panicPossibleGLError();

    splattingPass(mainLight, GBufferPosition, GBufferNormal,
                  mainLightShadowMap);
}
// fourth pass: subpass 1
void HDSSS::surfelizePass(const Scene& scene, MVP& mvp,
                          loo::UniformBuffer& mvpBuffer) {
    m_translucencyfb.bind();
    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_surfelizequery);
    logPossibleGLError();
    glEnable(GL_RASTERIZER_DISCARD);

    m_surfelizeshader.use();

    glPatchParameteri(GL_PATCH_VERTICES, 3);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_surfelizetf);
    glBeginTransformFeedback(GL_POINTS);
    scene.draw(
        m_surfelizeshader,
        [&mvp, &mvpBuffer](const auto& scene, const auto& mesh) {
            mvp.model = scene.getModelMatrix() * mesh.objectMatrix;
            mvpBuffer.updateData(offsetof(MVP, model), sizeof(mvp.model),
                                 &mvp.model);
        },
        GL_FILL, DRAW_FLAG_TESSELLATION);
    glEndTransformFeedback();
    glFlush();
    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
    glGetQueryObjectuiv(m_surfelizequery, GL_QUERY_RESULT,
                        (GLuint*)&m_surfelcount);
    glDisable(GL_RASTERIZER_DISCARD);
    m_translucencyfb.unbind();
    panicPossibleGLError();
}
// fourth pass: subpass 2
void HDSSS::splattingPass(const loo::ShaderLight& mainLight,
                          const loo::Texture2D& GBufferPosition,
                          const loo::Texture2D& GBufferNormal,
                          const loo::Texture2D& mainLightShadowMap) {
    auto app = static_cast<HDSSSApplication*>(Application::getContext());
    m_translucencyfb.bind();
    app->storeViewport();
    glViewport(0, 0, app->getWidth() >> 2, app->getHeight() >> 2);
    app->clear();
    glEnable(GL_BLEND);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE);
    m_translucencyshader.use();
    if (getSurfelCount()) {
        const auto& cam = app->getCamera();
        m_translucencyshader.setUniform("cameraPos", cam.getPosition());
        m_translucencyshader.setUniform("viewMatrix", cam.getViewMatrix());
        m_translucencyshader.setUniform("projectionMatrix",
                                        cam.getProjectionMatrix());
        m_translucencyshader.setUniform("strength", options.splattingStrength);
        m_translucencyshader.setUniform("fov", cam.m_fov);
        m_translucencyshader.setUniform(
            "framebufferDeviceStep.resolution",
            glm::ivec2(app->getWidth() >> 2, app->getHeight() >> 2));
        m_translucencyshader.setUniform("lightSpaceMatrix",
                                        mainLight.getLightSpaceMatrix());
        m_translucencyshader.setUniform("minimalEffect", options.minimalEffect);
        m_translucencyshader.setUniform("maxDistance", options.maxDistance);

        m_translucencyshader.setUniform("RdMaxArea", rdProfile.maxArea);
        m_translucencyshader.setUniform("RdMaxDistance", rdProfile.maxDistance);
        m_translucencyshader.setTexture(0, GBufferPosition);
        m_translucencyshader.setTexture(1, GBufferNormal);
        m_translucencyshader.setTexture(2, mainLightShadowMap);
        m_translucencyshader.setTexture(3, *rdProfile.texture);
        glBindVertexArray(m_surfelbuffer.vao);
        glDrawArrays(GL_POINTS, 0, getSurfelCount());
        logPossibleGLError();
    }
    glDisable(GL_BLEND);
    app->restoreViewport();
    m_translucencyfb.unbind();
}

// fifth pass: upscale transluency effect
void HDSSS::upscaleTranslucencyPass() {
    m_upscalefb.bind();
    m_upscaleshader.use();
    m_upscaleshader.setTexture(0, *m_translucencytex);
    Quad::globalQuad().draw();
    m_upscalefb.unbind();
}
// sixth pass: screen space subsurface scattering effect
void HDSSS::SSSSPass(loo::Texture2D& GBufferPosition,
                     loo::Texture2D& GBufferNormal,
                     const loo::Texture2D& GBuffer3,
                     const loo::Texture2D& GBuffer4,
                     loo::Texture2D& transmittedIrradiance) {
    GBufferPosition.setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    GBufferNormal.setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    transmittedIrradiance.setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    GBufferPosition.generateMipmap();
    GBufferNormal.generateMipmap();
    transmittedIrradiance.generateMipmap();

    const auto cam =
        static_cast<HDSSSApplication*>(Application::getContext())->getCamera();

    m_ssssfb.bind();
    m_ssssshader.use();
    m_ssssshader.setUniform("pixelAreaScale", options.ssssPixelAreaScale);
    m_ssssshader.setUniform("RdMaxArea", rdProfile.maxArea);
    m_ssssshader.setUniform("RdMaxDistance", rdProfile.maxDistance);
    m_ssssshader.setUniform("samplingMarkerEnable", options.ssssSamplingMarker);
    m_ssssshader.setUniform("samplingMarkerCenter",
                            options.ssssSamplingMarkerCenter);
    m_ssssshader.setUniform("cameraPos", cam.getPosition());
    m_ssssshader.setTexture(0, GBufferPosition);
    m_ssssshader.setTexture(1, GBufferNormal);
    m_ssssshader.setTexture(2, GBuffer3);
    m_ssssshader.setTexture(3, GBuffer4);
    m_ssssshader.setTexture(4, transmittedIrradiance);

    m_ssssshader.setTexture(5, *rdProfile.texture);

    Quad::globalQuad().draw();
    m_ssssfb.unbind();
}