#ifndef HDSSS_INCLUDE_HDSSSAPPLICATION_HPP
#define HDSSS_INCLUDE_HDSSSAPPLICATION_HPP

#include <filesystem>
#include <glm/glm.hpp>
#include <loo/Application.hpp>
#include <loo/Camera.hpp>
#include <loo/Light.hpp>
#include <loo/Quad.hpp>
#include <loo/Scene.hpp>
#include <loo/Shader.hpp>
#include <loo/Skybox.hpp>
#include <loo/UniformBuffer.hpp>
#include <loo/loo.hpp>
#include <memory>
#include <string>
#include <vector>
#include "BSSRDF.hpp"

#include "FinalProcess.hpp"
#include "constants.hpp"

class HDSSSApplication : public loo::Application {
   public:
    HDSSSApplication(int width, int height, const char* skyBoxPrefix = nullptr);
    // only load model
    void loadModel(const std::string& filename, float scaling = 1.0);
    void loadGLTF(const std::string& filename, float scaling = 1.0);
    loo::Camera& getCamera() { return m_maincam; }
    void afterCleanup() override;
    void convertMaterial();

   private:
    void initGBuffers();
    void initShadowMap();
    void initDeferredPass();

    void initTranslucencyPass();
    void initSurfelizePass();

    void initUpscalePass();

    void initSSSSPass();

    void loop() override;
    void gui();
    void scene();
    void skyboxPass();
    // first pass: gbuffer
    void gbufferPass();
    // second pass: shadow map
    void shadowMapPass();
    // third pass: deferred pass(illumination)
    void deferredPass();

    // fourth pass: translucency effect
    void translucencyPass();
    // fourth pass: subpass 1
    void surfelizePass();
    // fourth pass: subpass 2
    void splattingPass();

    // fifth pass: upscale transluency effect
    void upscaleTranslucencyPass();
    // sixth pass: screen space subsurface scattering effect
    void SSSSPass();
    // seventh pass: merge all effects
    void finalScreenPass();
    void clear();
    void keyboard();
    void mouse();
    void saveScreenshot(std::filesystem::path filename) const;
    struct MVP {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 normalMatrix;
    };
    MVP m_mvp;

    loo::ShaderProgram m_baseshader, m_skyboxshader;
    loo::Scene m_scene;
    std::shared_ptr<loo::TextureCubeMap> m_skyboxtex{};
    loo::Skybox m_skybox;
    loo::Camera m_maincam;
    loo::UniformBuffer m_mvpbuffer;
    loo::UniformBuffer m_lightsbuffer;
    std::vector<loo::ShaderLight> m_lights;

    std::shared_ptr<loo::Texture2D> m_mainlightshadowmap;
    loo::Framebuffer m_mainlightshadowmapfb;
    loo::ShaderProgram m_shadowmapshader;

    // gbuffer
    struct GBuffer {
        std::shared_ptr<loo::Texture2D> position;
        std::shared_ptr<loo::Texture2D> normal;
        // blinn-phong: diffuse(3) + specular(1)
        // pbr metallic-roughness: baseColor(3) + metallic(1)
        std::shared_ptr<loo::Texture2D> albedo;
        // simple material: transparent(3)(sss mask) + IOR(1)
        // pbr: transmission(1)(sss mask) + sigma_t(3)
        std::shared_ptr<loo::Texture2D> buffer3;
        // simple material: unused
        // pbr: sigma_a(3) + roughness(1)
        std::unique_ptr<loo::Texture2D> buffer4;
        // simple material: unused
        // pbr: occlusion(1) + unused(3)
        std::unique_ptr<loo::Texture2D> buffer5;
        loo::Renderbuffer depthrb;
    } m_gbuffers;
    loo::Framebuffer m_gbufferfb;
    // deferred pass
    loo::ShaderProgram m_deferredshader;
    loo::Framebuffer m_deferredfb;
    // diffuse
    std::shared_ptr<loo::Texture2D> m_diffuseresult;
    // transmitted_irradiance
    std::unique_ptr<loo::Texture2D> m_transmitted_irradiance;
    // specular
    std::unique_ptr<loo::Texture2D> m_reflected_radiance;
    // skybox
    std::unique_ptr<loo::Texture2D> m_skyboxresult;
    // translucent pass
    loo::ShaderProgram m_translucencyshader;

    loo::ShaderProgram m_surfelizeshader;
    int m_surfelcount{0};

    int getSurfelCount() const { return m_surfelcount; }

    GLuint m_surfelizetf, m_surfelizequery;
    struct SurfelBuffer {
        GLuint vao;
        GLuint vbo;
    } m_surfelbuffer;
    loo::Framebuffer m_translucencyfb;
    struct TranslucencyUniforms {
        float minimalEffect{0.0001f};
        float maxDistance{0.0015f};
    } m_translucencyuniforms;
    std::unique_ptr<loo::Texture2D> m_translucencytex;

    // scene surfelize
    loo::Framebuffer m_surfelizefb;
    float m_surfelizescale{0.00085f}, m_splattingstrength{1.0f};

    // upscale pass
    loo::ShaderProgram m_upscaleshader;
    loo::Framebuffer m_upscalefb;
    std::unique_ptr<loo::Texture2D> m_upscaletex;

    // ssss pass
    loo::ShaderProgram m_ssssshader;
    loo::Framebuffer m_ssssfb;
    std::unique_ptr<loo::Texture2D> m_sssstex;
    float m_ssss_pixelareascale{1e-4f};
    bool m_ssss_samplingmarker{false};
    glm::ivec2 m_ssss_samplingmarkercenter{0, 0};
    struct RdProfile {
        std::unique_ptr<loo::Texture2D> texture;
        float maxDistance, maxArea;
    } m_rdprofile;
    // screen quad
    std::shared_ptr<loo::Quad> m_globalquad;

    // process
    FinalProcess m_finalprocess;

    bool m_wireframe{};
    bool m_enablenormal{true};
    bool m_enableparallax{true};
    bool m_lodvisualize{false};
    bool m_screenshotflag{false};

    FinalPassOptions m_finalpassoptions;
    // float m_displaceintensity{};
};

#endif /* HDSSS_INCLUDE_HDSSSAPPLICATION_HPP */
