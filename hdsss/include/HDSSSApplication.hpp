#ifndef HDSSS_INCLUDE_HDSSSAPPLICATION_HPP
#define HDSSS_INCLUDE_HDSSSAPPLICATION_HPP

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

#include "FinalProcess.hpp"
#include "HighDistanceSSS.hpp"
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

    void loop() override;
    void gui();
    void scene();
    void skyboxPass();
    void gbufferPass();
    void shadowMapPass();
    void deferredPass();
    void finalScreenPass();
    void highDistanceSSS();
    void clear();
    void keyboard();
    void mouse();
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
        std::shared_ptr<loo::Texture2D> albedo;
        std::shared_ptr<loo::Texture2D> sssMask;
        loo::Renderbuffer depthrb;
    } m_gbuffers;
    loo::Framebuffer m_gbufferfb;
    // deferred pass
    loo::ShaderProgram m_deferredshader;
    loo::Framebuffer m_deferredfb;
    std::shared_ptr<loo::Texture2D> m_deferredtex;

    // scene surfelize
    loo::Framebuffer m_surfelizefb;
    float m_surfelizescale{0.00085f}, m_splattingstrength{4.5f};

    // screen quad
    std::shared_ptr<loo::Quad> m_globalquad;

    // process
    FinalProcess m_finalprocess;

    // Deep screen space
    HighDistanceSSS m_hdsss;

    bool m_wireframe{};
    bool m_enablenormal{true};
    bool m_enableparallax{true};
    bool m_lodvisualize{false};
    bool m_applysss{false};
    // float m_displaceintensity{};
};

#endif /* HDSSS_INCLUDE_HDSSSAPPLICATION_HPP */
