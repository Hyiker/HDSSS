#ifndef HDSSS_INCLUDE_HDSSS_HPP
#define HDSSS_INCLUDE_HDSSS_HPP
#include <loo/Framebuffer.hpp>
#include <loo/Light.hpp>
#include <loo/Scene.hpp>
#include <loo/Shader.hpp>
#include "Transforms.hpp"
struct HDSSSOptions {
    float minimalEffect{0.0001f};
    float maxDistance{0.0015f};
    float surfelizeScale{0.00085f};
    float splattingStrength{1.0f};
    float ssssPixelAreaScale{1e-4f};
    bool ssssSamplingMarker{false};
    glm::ivec2 ssssSamplingMarkerCenter{0, 0};
};
class HDSSS {

    void initTranslucencyPass();
    void initSurfelizePass();
    void initUpscalePass();
    void initSSSSPass();

    // fourth pass: subpass 1
    void surfelizePass(const loo::Scene& scene, MVP& mvp,
                       loo::UniformBuffer& mvpBuffer);
    // fourth pass: subpass 2
    void splattingPass(const loo::ShaderLight& mainLight,
                       const loo::Texture2D& GBufferPosition,
                       const loo::Texture2D& GBufferNormal,
                       const loo::Texture2D& mainLightShadowMap);

    // translucent pass
    loo::ShaderProgram m_translucencyshader;

    loo::ShaderProgram m_surfelizeshader;
    int m_surfelcount{0};

    GLuint m_surfelizetf, m_surfelizequery;
    struct SurfelBuffer {
        GLuint vao;
        GLuint vbo;
    } m_surfelbuffer;
    loo::Framebuffer m_translucencyfb;
    std::unique_ptr<loo::Texture2D> m_translucencytex;

    // scene surfelize
    loo::Framebuffer m_surfelizefb;

    // upscale pass
    loo::ShaderProgram m_upscaleshader;
    loo::Framebuffer m_upscalefb;
    std::unique_ptr<loo::Texture2D> m_upscaletex;

    // ssss pass
    loo::ShaderProgram m_ssssshader;
    loo::Framebuffer m_ssssfb;
    std::unique_ptr<loo::Texture2D> m_sssstex;

   public:
    HDSSS();
    void init();
    // fourth pass: translucency effect
    void translucencyPass(const loo::Scene& scene, MVP& mvp,
                          loo::UniformBuffer& mvpBuffer,
                          const loo::ShaderLight& mainLight,
                          const loo::Texture2D& GBufferPosition,
                          const loo::Texture2D& GBufferNormal,
                          const loo::Texture2D& mainLightShadowMap);

    // fifth pass: upscale transluency effect
    void upscaleTranslucencyPass();
    // sixth pass: screen space subsurface scattering effect
    void SSSSPass(loo::Texture2D& GBufferPosition,
                  loo::Texture2D& GBufferNormal, const loo::Texture2D& GBuffer3,
                  const loo::Texture2D& GBuffer4,
                  loo::Texture2D& transmittedIrradiance);
    int getSurfelCount() const { return m_surfelcount; }
    const auto& getUpscaleResult() { return *m_upscaletex; }
    const auto& getSSSSResult() { return *m_sssstex; }
    HDSSSOptions options;
    struct RdProfile {
        std::unique_ptr<loo::Texture2D> texture;
        float maxDistance, maxArea;
    } rdProfile;
};
#endif /* HDSSS_INCLUDE_HDSSS_HPP */
