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
#include "DeepScreenSpace.hpp"
#include "HDSSS.hpp"
#include "Transforms.hpp"

#include "FinalProcess.hpp"
#include "constants.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"

struct HDSSSConfig {
    struct CameraConfig {
        glm::vec3 position{glm::vec3(0.0f, 0.0f, 0.0f)};
        glm::vec3 lookat{glm::vec3(0.0f, 0.0f, -1.0f)};
        float fov{glm::radians(60.0f)};
        float zNear{0.01f};
        float zFar{50.0f};
    } camera;
    struct LightConfig {
        glm::vec3 direction{glm::vec3(-1.0f, -1.0f, 0.0f)};
        glm::vec3 color{glm::vec3(1.0f)};
        float intensity{1.0f};
    } light;
    struct ModelConfig {
        glm::mat4 transform{glm::identity<glm::mat4>()};
    } model;
    struct BSSRDFConfig {
        glm::vec3 sigma_t{glm::vec3(4.0f)};
        glm::vec3 albedo{glm::vec3(0, 1, 0)};
    } bssrdf;
    struct Animation {
        float cameraRotationY{0.0f};
        float modelRotationY{0.0f};
    } animation;
};

class HDSSSApplication : public loo::Application {
   public:
    HDSSSApplication(int width, int height, const HDSSSConfig& config,
                     const char* skyBoxPrefix = nullptr);
    // only load model
    void loadModel(const std::string& filename, glm::mat4 transform);
    void loadGLTF(const std::string& filename, glm::mat4 transform);
    loo::Camera& getCamera() { return m_maincam; }
    void afterCleanup() override;
    void convertMaterial();
    void clear();

   private:
    void initGBuffers();
    void initShadowMap();
    void initDeferredPass();

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

    // seventh pass: merge all effects
    void finalScreenPass();
    void keyboard();
    void mouse();
    void saveScreenshot(std::filesystem::path filename) const;
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

    enum class SubsurfaceMethod { HDSSS, DSS };
    SubsurfaceMethod m_method{SubsurfaceMethod::HDSSS};

    HDSSS m_hdsss;

    DeepScreenSpace m_dss;

    // process
    FinalProcess m_finalprocess;

    bool m_wireframe{false};
    bool m_enablenormal{true};
    bool m_enableparallax{true};
    bool m_lodvisualize{false};
    bool m_screenshotflag{false};

    float m_modelrotationy{0.0f};
    float m_camerarotationy{0.0f};

    bool m_guienable{true};

    FinalPassOptions m_finalpassoptions;
    // float m_displaceintensity{};
   public:
};

#endif /* HDSSS_INCLUDE_HDSSSAPPLICATION_HPP */
