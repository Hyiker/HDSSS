#include "HDSSSApplication.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <fstream>
#include <loo/glError.hpp>
#include <memory>
#include <vector>
#include "PBRMaterials.hpp"
#include "SimpleMaterial.hpp"

#include "Surfel.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/quaternion_geometric.hpp"
#include "glm/fwd.hpp"
#include "glog/logging.h"
#include "imgui_impl_glfw.h"
#include "shaders/deferred.frag.hpp"
#include "shaders/deferred.vert.hpp"
#include "shaders/gbuffer.frag.hpp"
#include "shaders/gbuffer.vert.hpp"
#include "shaders/shadowmap.frag.hpp"
#include "shaders/shadowmap.vert.hpp"
#include "shaders/skybox.frag.hpp"
#include "shaders/skybox.vert.hpp"
#include "shaders/surfelize.tesc.hpp"
#include "shaders/surfelize.tese.hpp"
#include "shaders/surfelize.vert.hpp"
#include "shaders/translucency.frag.hpp"
#include "shaders/translucency.geom.hpp"
#include "shaders/translucency.vert.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext.hpp"
using namespace loo;
using namespace std;

static constexpr int SHADOWMAP_RESOLUION[2]{2048, 2048};

static void mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    ImGui_ImplGlfw_CursorPosCallback(window, xposIn, yposIn);
    static bool firstMouse = true;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) {
        firstMouse = true;
        return;
    }
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    auto myapp =
        static_cast<HDSSSApplication*>(glfwGetWindowUserPointer(window));
    static float lastX = myapp->getWidth() / 2.0;
    static float lastY = myapp->getHeight() / 2.0;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;
    myapp->getCamera().processMouseMovement(xoffset, yoffset);
}

static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    auto myapp =
        static_cast<HDSSSApplication*>(glfwGetWindowUserPointer(window));
    myapp->getCamera().processMouseScroll(xOffset, yOffset);
}

void HDSSSApplication::loadModel(const std::string& filename, float scaling) {
    LOG(INFO) << "Loading model from " << filename << endl;
    glm::mat4 transform = glm::scale(glm::identity<glm::mat4>(),
                                     glm::vec3(scaling, scaling, scaling));
    auto meshes = createMeshFromFile(filename, transform);
    m_scene.addMeshes(std::move(meshes));

    m_scene.prepare();
    LOG(INFO) << "Load done" << endl;
}

void HDSSSApplication::loadGLTF(const std::string& filename, float scaling) {
    LOG(INFO) << "Loading scene from " << filename << endl;
    // TODO: m_scene = createSceneFromFile(filename);
    glm::mat4 transform = glm::scale(glm::identity<glm::mat4>(),
                                     glm::vec3(scaling, scaling, scaling));
    auto meshes = createMeshFromFile(filename, transform);
    m_scene.addMeshes(std::move(meshes));

    m_scene.prepare();
    LOG(INFO) << "Load done" << endl;
}

HDSSSApplication::HDSSSApplication(int width, int height,
                                   const char* skyBoxPrefix)
    : Application(width, height, "High distance subsurface scattering"),
      m_baseshader{Shader(GBUFFER_VERT, ShaderType::Vertex),
                   Shader(GBUFFER_FRAG, ShaderType::Fragment)},
      m_skyboxshader{Shader(SKYBOX_VERT, ShaderType::Vertex),
                     Shader(SKYBOX_FRAG, ShaderType::Fragment)},
      m_scene(),
      m_maincam(),
      m_mvpbuffer(0, sizeof(MVP)),
      m_lightsbuffer(SHADER_BINDING_LIGHTS,
                     sizeof(ShaderLight) * SHADER_LIGHTS_MAX + sizeof(GLint)),
      m_shadowmapshader{Shader(SHADOWMAP_VERT, ShaderType::Vertex),
                        Shader(SHADOWMAP_FRAG, ShaderType::Fragment)},
      m_deferredshader{Shader(DEFERRED_VERT, ShaderType::Vertex),
                       Shader(DEFERRED_FRAG, ShaderType::Fragment)},
      m_translucencyshader{Shader(TRANSLUCENCY_VERT, ShaderType::Vertex),
                           Shader(TRANSLUCENCY_GEOM, ShaderType::Geometry),
                           Shader(TRANSLUCENCY_FRAG, ShaderType::Fragment)},
      m_surfelizeshader(
          {
              Shader(SURFELIZE_VERT, ShaderType::Vertex),
              Shader(SURFELIZE_TESC, ShaderType::TessellationControl),
              Shader(SURFELIZE_TESE, ShaderType::TessellationEvaluation),
          },
          {"tePos", "teNormal", "teRadius", "teSigmaT", "teSigmaA"}),
      m_globalquad(make_shared<Quad>()),
      m_finalprocess(getWidth(), getHeight(), m_globalquad) {
    ifstream ifs("camera.bin", ios::binary);
    if (!ifs.fail()) {
        ifs.read(reinterpret_cast<char*>(&m_maincam), sizeof(m_maincam));
    }
    ifs.close();
    if (skyBoxPrefix) {
        // skybox setup
        auto skyboxFilenames = TextureCubeMap::builder()
                                   .front("front")
                                   .back("back")
                                   .left("left")
                                   .right("right")
                                   .top("top")
                                   .bottom("bottom")
                                   .prefix(skyBoxPrefix)
                                   .build();
        m_skyboxtex = createTextureCubeMapFromFiles(
            skyboxFilenames,
            TEXTURE_OPTION_CONVERT_TO_LINEAR | TEXTURE_OPTION_MIPMAP);
    }
    // scene light
    {
        m_lights.push_back(
            createDirectionalLight(glm::vec3(-1, -1, 0), glm::vec3(1, 1, 1)));
    }

    initGBuffers();
    initShadowMap();
    initDeferredPass();
    initTranslucencyPass();

    initUpscalePass();

    // final pass related
    { m_finalprocess.init(); }
    panicPossibleGLError();
}
void HDSSSApplication::initGBuffers() {
    m_gbufferfb.init();

    int mipmapLevel = mipmapLevelFromSize(getWidth(), getHeight());

    m_gbuffers.position = make_shared<Texture2D>();
    m_gbuffers.position->init();
    m_gbuffers.position->setupStorage(getWidth(), getHeight(), GL_RGBA32F,
                                      mipmapLevel);
    m_gbuffers.position->setSizeFilter(GL_NEAREST, GL_NEAREST);

    m_gbuffers.normal = make_shared<Texture2D>();
    m_gbuffers.normal->init();
    m_gbuffers.normal->setupStorage(getWidth(), getHeight(), GL_RGB8,
                                    mipmapLevel);
    m_gbuffers.normal->setSizeFilter(GL_NEAREST, GL_NEAREST);

    m_gbuffers.albedo = make_shared<Texture2D>();
    m_gbuffers.albedo->init();
    m_gbuffers.albedo->setupStorage(getWidth(), getHeight(), GL_RGBA32F, 1);
    m_gbuffers.albedo->setSizeFilter(GL_LINEAR, GL_LINEAR);

    m_gbuffers.buffer3 = make_shared<Texture2D>();
    m_gbuffers.buffer3->init();
    m_gbuffers.buffer3->setupStorage(getWidth(), getHeight(), GL_RGBA32F, 1);
    m_gbuffers.buffer3->setSizeFilter(GL_NEAREST, GL_NEAREST);

    m_gbuffers.buffer4 = make_unique<Texture2D>();
    m_gbuffers.buffer4->init();
    m_gbuffers.buffer4->setupStorage(getWidth(), getHeight(), GL_RGBA16F, 1);
    m_gbuffers.buffer4->setSizeFilter(GL_NEAREST, GL_NEAREST);

    m_gbuffers.buffer5 = make_unique<Texture2D>();
    m_gbuffers.buffer5->init();
    m_gbuffers.buffer5->setupStorage(getWidth(), getHeight(), GL_RGBA16F, 1);
    m_gbuffers.buffer5->setSizeFilter(GL_NEAREST, GL_NEAREST);

    panicPossibleGLError();

    m_gbuffers.depthrb.init(GL_DEPTH_COMPONENT32, getWidth(), getHeight());

    m_gbufferfb.attachTexture(*m_gbuffers.position, GL_COLOR_ATTACHMENT0, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.normal, GL_COLOR_ATTACHMENT1, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.albedo, GL_COLOR_ATTACHMENT2, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.buffer3, GL_COLOR_ATTACHMENT3, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.buffer4, GL_COLOR_ATTACHMENT4, 0);
    m_gbufferfb.attachTexture(*m_gbuffers.buffer5, GL_COLOR_ATTACHMENT5, 0);
    m_gbufferfb.attachRenderbuffer(m_gbuffers.depthrb, GL_DEPTH_ATTACHMENT);
}

void HDSSSApplication::initShadowMap() {
    m_mainlightshadowmapfb.init();
    m_mainlightshadowmap = make_shared<Texture2D>();
    m_mainlightshadowmap->init();
    m_mainlightshadowmap->setupStorage(SHADOWMAP_RESOLUION[0],
                                       SHADOWMAP_RESOLUION[1],
                                       GL_DEPTH_COMPONENT32, 1);
    m_mainlightshadowmap->setSizeFilter(GL_NEAREST, GL_NEAREST);
    // m_mainlightshadowmap->setWrapFilter(GL_CLAMP_TO_EDGE);
    m_mainlightshadowmapfb.attachTexture(*m_mainlightshadowmap,
                                         GL_DEPTH_ATTACHMENT, 0);
    glNamedFramebufferDrawBuffer(m_mainlightshadowmapfb.getId(), GL_NONE);
    glNamedFramebufferReadBuffer(m_mainlightshadowmapfb.getId(), GL_NONE);
    panicPossibleGLError();
}
void HDSSSApplication::initDeferredPass() {
    m_deferredfb.init();
    m_transmitted_irradiance = make_shared<Texture2D>();
    m_transmitted_irradiance->init();
    m_transmitted_irradiance->setupStorage(getWidth(), getHeight(), GL_RGBA32F,
                                           -1);
    m_transmitted_irradiance->setSizeFilter(GL_LINEAR, GL_LINEAR);

    m_reflected_radiance = make_unique<Texture2D>();
    m_reflected_radiance->init();
    m_reflected_radiance->setupStorage(getWidth(), getHeight(), GL_RGBA32F, 1);
    m_reflected_radiance->setSizeFilter(GL_LINEAR, GL_LINEAR);

    m_skyboxresult = make_unique<Texture2D>();
    m_skyboxresult->init();
    m_skyboxresult->setupStorage(getWidth(), getHeight(), GL_RGBA32F, 1);
    m_skyboxresult->setSizeFilter(GL_LINEAR, GL_LINEAR);

    m_deferredfb.attachTexture(*m_transmitted_irradiance, GL_COLOR_ATTACHMENT0,
                               0);
    m_deferredfb.attachTexture(*m_reflected_radiance, GL_COLOR_ATTACHMENT1, 0);
    m_deferredfb.attachTexture(*m_skyboxresult, GL_COLOR_ATTACHMENT2, 0);
    m_deferredfb.attachRenderbuffer(m_gbuffers.depthrb, GL_DEPTH_ATTACHMENT);
    panicPossibleGLError();
}

void HDSSSApplication::initTranslucencyPass() {
    initSurfelizePass();

    m_translucencyfb.init();

    m_translucencytex = make_unique<Texture2D>();
    m_translucencytex->init();
    m_translucencytex->setupStorage(getWidth() >> 2, getHeight() >> 2,
                                    GL_RGB32F, 1);
    m_translucencytex->setSizeFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    m_translucencyfb.attachTexture(*m_translucencytex, GL_COLOR_ATTACHMENT0, 0);
    m_translucencyfb.enableAttachments({GL_COLOR_ATTACHMENT0});

    panicPossibleGLError();
}

void HDSSSApplication::initSurfelizePass() {
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
void HDSSSApplication::initUpscalePass() {
    std::unique_ptr<Texture2D> upscalePingpong[2];
    for (int i = 0; i < 2; i++) {
        upscalePingpong[i] = make_unique<Texture2D>();
        upscalePingpong[i]->init();
        upscalePingpong[i]->setupStorage(getWidth(), getHeight(), GL_RGB32F, 1);
        upscalePingpong[i]->setSizeFilter(GL_LINEAR, GL_LINEAR);
    }
    m_upscaleblur.init(std::move(upscalePingpong[0]),
                       std::move(upscalePingpong[1]));
    panicPossibleGLError();
}

void HDSSSApplication::gui() {
    auto& io = ImGui::GetIO();
    float h = io.DisplaySize.y;
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove;
    {
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::SetNextWindowSize(ImVec2(300, h * 0.3));
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        if (ImGui::Begin("Dashboard", nullptr, windowFlags)) {
            // OpenGL option
            if (ImGui::CollapsingHeader("General info",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text(
                    "Frame generation delay: %.3f ms/frame\n"
                    "FPS: %.1f",
                    1000.0f / io.Framerate, io.Framerate);

                const GLubyte* renderer = glGetString(GL_RENDERER);
                const GLubyte* version = glGetString(GL_VERSION);
                ImGui::TextWrapped("Renderer: %s", renderer);
                ImGui::TextWrapped("OpenGL Version: %s", version);
                int triangleCount = m_scene.countTriangle();
                const char* base = "";
                if (triangleCount > 5000) {
                    triangleCount /= 1000;
                    base = "k";
                }
                if (triangleCount > 5000) {
                    triangleCount /= 1000;
                    base = "M";
                }
                ImGui::Text(
                    "Scene meshes: %d\n"
                    "Scene triangles: %d%s",
                    (int)m_scene.countMesh(), triangleCount, base);
                ImGui::TextWrapped("Camera position: %.2f %.2f %.2f",
                                   m_maincam.getPosition().x,
                                   m_maincam.getPosition().y,
                                   m_maincam.getPosition().z);
            }
            if (ImGui::CollapsingHeader("High distance SSS info",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                string postfix = "";
                int nSurfel = getSurfelCount();
                if (nSurfel > 1000) {
                    nSurfel /= 1000;
                    postfix = "k";
                }
                if (nSurfel > 1000) {
                    nSurfel /= 1000;
                    postfix = "M";
                }
                ImGui::Text("Surfel count: %d%s", nSurfel, postfix.c_str());
            }
        }
    }

    {
        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::SetNextWindowSize(ImVec2(300, h * 0.5));
        ImGui::SetNextWindowPos(ImVec2(0, h * 0.3), ImGuiCond_Always);
        if (ImGui::Begin("Options", nullptr, windowFlags)) {
            // Sun
            if (ImGui::CollapsingHeader("Sun",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorPicker3("Color", (float*)&m_lights[0].color);
                ImGui::SliderFloat3("Direction", (float*)&m_lights[0].direction,
                                    -1, 1);
                ImGui::SliderFloat("Intensity", (float*)&m_lights[0].intensity,
                                   0.0, 100.0);
            }

            // OpenGL option
            if (ImGui::CollapsingHeader("OpenGL options",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Wire frame mode", &m_wireframe);
                ImGui::Checkbox("Normal mapping", &m_enablenormal);
                ImGui::Checkbox("Parallax mapping", &m_enableparallax);
                ImGui::Checkbox("Visualize lod", &m_lodvisualize);
            }

            if (ImGui::CollapsingHeader("HDSSS options",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.45f);

                ImGui::SliderFloat("Splatting strength", &m_splattingstrength,
                                   1.0f, 1e2f, "%.1f",
                                   ImGuiSliderFlags_Logarithmic);
                ImGui::TextWrapped(
                    "Below fields only effect materials with SSS masks");
                ImGui::Checkbox("Use diffuse texture",
                                &m_finalpassoptions.diffuse);
                ImGui::Checkbox("Use specular texture",
                                &m_finalpassoptions.specular);
                ImGui::Checkbox("Use translucency texture",
                                &m_finalpassoptions.translucency);
                ImGui::Checkbox("Use SSS texture", &m_finalpassoptions.SSS);

                ImGui::PopItemWidth();
            }
        }
    }
    {
        float h_img = h * 0.2,
              w_img = h_img / io.DisplaySize.y * io.DisplaySize.x;
        ImGui::SetNextWindowBgAlpha(1.0f);
        vector<GLuint> textures{m_gbuffers.albedo->getId(),
                                m_gbuffers.normal->getId(),
                                m_translucencytex->getId()};
        ImGui::SetNextWindowSize(ImVec2(w_img * textures.size() + 40, h_img));
        ImGui::SetNextWindowPos(ImVec2(0, h * 0.8), ImGuiCond_Always);
        if (ImGui::Begin("Textures", nullptr,
                         windowFlags | ImGuiWindowFlags_NoDecoration)) {
            for (auto texId : textures) {
                ImGui::Image((void*)(intptr_t)texId, ImVec2(w_img, h_img),
                             ImVec2(0, 1), ImVec2(1, 0));
                ImGui::SameLine();
            }
        }
    }
}

void HDSSSApplication::finalScreenPass() {
    m_finalpassoptions.directOutput = m_lodvisualize;
    m_finalprocess.render(*m_transmitted_irradiance, *m_reflected_radiance,
                          m_upscaleblur.getBlurResult(),
                          Texture2D::getBlackTexture(), *m_gbuffers.buffer3,
                          *m_skyboxresult, m_finalpassoptions);
}

void HDSSSApplication::convertMaterial() {
    for (auto& mesh : m_scene.getMeshes()) {
        // Now default material is PBR material
        if (mesh->material) {
#ifdef MATERIAL_PBR
            LOG(INFO) << "Converting material to PBR material";
            mesh->material = convertPBRMetallicMaterialFromBaseMaterial(
                *static_pointer_cast<BaseMaterial>(mesh->material));
#else
            LOG(INFO) << "Converting material to simple(blinn-phong) material";
            mesh->material = convertSimpleMaterialFromBaseMaterial(
                *static_pointer_cast<BaseMaterial>(mesh->material));
#endif
        }
    }
}

void HDSSSApplication::skyboxPass() {
    if (m_skyboxtex) {
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glm::mat4 view;
        m_maincam.getViewMatrix(view);
        m_skyboxshader.use();
        m_mvp.view = glm::mat4(glm::mat3(view));
        m_mvpbuffer.updateData(0, sizeof(MVP), &m_mvp);
        m_skyboxshader.setTexture(SHADER_BINDING_PORT_SKYBOX, *m_skyboxtex);
        m_skybox.draw();
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
    }
}
void HDSSSApplication::gbufferPass() {
    // render gbuffer here
    m_gbufferfb.bind();

    m_gbufferfb.enableAttachments({GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                   GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
                                   GL_COLOR_ATTACHMENT4});

    clear();

    scene();

    m_gbufferfb.unbind();
}

void HDSSSApplication::shadowMapPass() {
    // render shadow map here
    m_mainlightshadowmapfb.bind();
    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    glViewport(0, 0, SHADOWMAP_RESOLUION[0], SHADOWMAP_RESOLUION[1]);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    m_shadowmapshader.use();
    // directional light matrix
    const auto& mainLight = m_lights[0];
    CHECK_EQ(mainLight.type, LightType::DIRECTIONAL);
    glm::mat4 lightSpaceMatrix = mainLight.getLightSpaceMatrix();

    m_shadowmapshader.setUniform("lightSpaceMatrix", lightSpaceMatrix);

    m_scene.draw(
        m_shadowmapshader,
        [this](const auto& scene, const auto& mesh) {
            m_mvp.model = scene.getModelMatrix() * mesh.objectMatrix;
            m_mvpbuffer.updateData(offsetof(MVP, model), sizeof(m_mvp.model),
                                   &m_mvp.model);
        },
        GL_FILL, 0);

    m_mainlightshadowmapfb.unbind();
    glViewport(vp[0], vp[1], vp[2], vp[3]);
}

void HDSSSApplication::deferredPass() {
    // render gbuffer here

    m_deferredfb.bind();
    m_deferredfb.enableAttachments(
        {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});

    glClearColor(0.f, 0.f, 0.f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    logPossibleGLError();

    m_deferredshader.use();
    m_deferredshader.setTexture(0, *m_gbuffers.position);
    m_deferredshader.setTexture(1, *m_gbuffers.normal);
    m_deferredshader.setTexture(2, *m_gbuffers.albedo);
    m_deferredshader.setTexture(3, *m_gbuffers.buffer3);
    m_deferredshader.setTexture(4, *m_gbuffers.buffer4);
    m_deferredshader.setTexture(5, *m_gbuffers.buffer5);
    m_deferredshader.setTexture(6, *m_mainlightshadowmap);
    m_deferredshader.setUniform("mainLightMatrix",
                                m_lights[0].getLightSpaceMatrix());

    m_deferredshader.setUniform("cameraPosition", m_maincam.getPosition());
    {
        m_lightsbuffer.updateData(0, sizeof(ShaderLight) * m_lights.size(),
                                  m_lights.data());
        GLint nLights = m_lights.size();
        m_lightsbuffer.updateData(sizeof(ShaderLight) * SHADER_LIGHTS_MAX,
                                  sizeof(GLint), &nLights);
    }

    glDisable(GL_DEPTH_TEST);
    m_globalquad->draw();

    m_deferredfb.enableAttachments({GL_COLOR_ATTACHMENT2});
    glClearColor(0.f, 0.f, 0.f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    skyboxPass();
    m_gbufferfb.unbind();
}

void HDSSSApplication::surfelizePass() {
    m_translucencyfb.bind();
    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_surfelizequery);
    logPossibleGLError();
    glEnable(GL_RASTERIZER_DISCARD);

    m_surfelizeshader.use();

    glPatchParameteri(GL_PATCH_VERTICES, 3);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_surfelizetf);
    glBeginTransformFeedback(GL_POINTS);
    m_scene.draw(
        m_surfelizeshader,
        [this](const auto& scene, const auto& mesh) {
            m_mvp.model = scene.getModelMatrix() * mesh.objectMatrix;
            m_mvpbuffer.updateData(offsetof(MVP, model), sizeof(m_mvp.model),
                                   &m_mvp.model);
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

void HDSSSApplication::splattingPass() {
    m_translucencyfb.bind();
    storeViewport();
    glViewport(0, 0, getWidth() >> 2, getHeight() >> 2);
    clear();
    glEnable(GL_BLEND);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE);
    m_translucencyshader.use();
    if (getSurfelCount()) {
        m_translucencyshader.setUniform("cameraPos", m_maincam.getPosition());
        m_translucencyshader.setUniform("viewMatrix",
                                        m_maincam.getViewMatrix());
        m_translucencyshader.setUniform("projectionMatrix",
                                        m_maincam.getProjectionMatrix());
        m_translucencyshader.setUniform("strength", m_splattingstrength);
        m_translucencyshader.setUniform("fov", m_maincam.m_fov);
        m_translucencyshader.setUniform(
            "framebufferDeviceStep.resolution",
            glm::ivec2(getWidth() >> 2, getHeight() >> 2));
        m_translucencyshader.setUniform("lightSpaceMatrix",
                                        m_lights[0].getLightSpaceMatrix());
        m_translucencyshader.setTexture(0, *m_gbuffers.position);
        m_translucencyshader.setTexture(1, *m_gbuffers.normal);
        m_translucencyshader.setTexture(2, *m_mainlightshadowmap);
        glBindVertexArray(m_surfelbuffer.vao);
        glDrawArrays(GL_POINTS, 0, getSurfelCount());
        logPossibleGLError();
    }
    glDisable(GL_BLEND);
    restoreViewport();
    m_translucencyfb.unbind();
}

void HDSSSApplication::translucencyPass() {
    surfelizePass();

    // m_gbuffers.position->generateMipmap();
    // m_gbuffers.normal->generateMipmap();
    // m_transmitted_irradiance->generateMipmap();

    panicPossibleGLError();

    splattingPass();
    m_translucencytex->generateMipmap();
}

void HDSSSApplication::upscaleTranslucencyPass() {
    m_upscaleblur.blurFrom(*m_globalquad, *m_translucencytex,
                           GaussianBlurDirection::X);
    m_upscaleblur.blur(*m_globalquad, GaussianBlurDirection::Y);
}

void HDSSSApplication::SSSSPass() {}

void HDSSSApplication::scene() {
    glEnable(GL_DEPTH_TEST);
    m_maincam.getViewMatrix(m_mvp.view);
    m_maincam.getProjectionMatrix(m_mvp.projection);

    logPossibleGLError();
    m_baseshader.use();

    m_baseshader.setUniform("uCameraPosition", m_maincam.getPosition());
    m_baseshader.setUniform("enableNormal", m_enablenormal);
    m_baseshader.setUniform("enableParallax", m_enableparallax);
    m_baseshader.setUniform("enableLodVisualize", (int)m_lodvisualize);
    logPossibleGLError();

    m_scene.draw(
        m_baseshader,
        [this](const auto& scene, const auto& mesh) {
            m_mvp.model = scene.getModelMatrix() * mesh.objectMatrix;
            m_mvp.normalMatrix = glm::transpose(glm::inverse(m_mvp.model));
            m_mvpbuffer.updateData(0, sizeof(MVP), &m_mvp);
        },
        m_wireframe ? GL_LINE : GL_FILL, 0);
    logPossibleGLError();
}

void HDSSSApplication::clear() {
    glClearColor(0.f, 0.f, 0.f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    logPossibleGLError();
}
void HDSSSApplication::keyboard() {
    if (keyForward())
        m_maincam.processKeyboard(CameraMovement::FORWARD, getFrameDeltaTime());
    if (keyBackward())
        m_maincam.processKeyboard(CameraMovement::BACKWARD,
                                  getFrameDeltaTime());
    if (keyLeft())
        m_maincam.processKeyboard(CameraMovement::LEFT, getFrameDeltaTime());
    if (keyRight())
        m_maincam.processKeyboard(CameraMovement::RIGHT, getFrameDeltaTime());
    if (glfwGetKey(getWindow(), GLFW_KEY_R)) {
        m_maincam = Camera();
    }
}
void HDSSSApplication::mouse() {
    glfwSetCursorPosCallback(getWindow(), mouseCallback);
    glfwSetScrollCallback(getWindow(), scrollCallback);
}
void HDSSSApplication::loop() {
    m_maincam.m_aspect = getWindowRatio();
    // render
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    {
        gbufferPass();

        shadowMapPass();

        deferredPass();

        translucencyPass();

        upscaleTranslucencyPass();

        SSSSPass();

        finalScreenPass();
    }

    keyboard();

    mouse();

    gui();
}

void HDSSSApplication::afterCleanup() {
    ofstream ofs("camera.bin", ios::binary);
    ofs.write((char*)&m_maincam, sizeof(Camera));
    ofs.close();
}
