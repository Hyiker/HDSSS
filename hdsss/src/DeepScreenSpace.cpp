#include "DeepScreenSpace.hpp"

#include <algorithm>
#include <loo/Application.hpp>
#include <loo/Camera.hpp>
#include <loo/Scene.hpp>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

#include "glog/logging.h"
#include "shaders/dssPositionNormalShuffler.frag.hpp"
#include "shaders/dssPositionNormalShuffler.vert.hpp"
#include "shaders/dssSplatting.frag.hpp"
#include "shaders/dssSplatting.geom.hpp"
#include "shaders/dssSplatting.vert.hpp"
#include "shaders/dssSplattingUnshuffle.frag.hpp"
#include "shaders/dssSplattingUnshuffle.vert.hpp"
#include "shaders/dssSumUp.frag.hpp"
#include "shaders/dssSumUp.vert.hpp"
#include "shaders/dssSurfelize.tesc.hpp"
#include "shaders/dssSurfelize.tese.hpp"
#include "shaders/dssSurfelize.vert.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext.hpp"
using namespace loo;
using namespace std;
using namespace glm;
DeepScreenSpace::DeepScreenSpace()
    : m_surfelizeshader(
          {Shader(DSSSURFELIZE_VERT, ShaderType::Vertex),
           // add tessellation here
           Shader(DSSSURFELIZE_TESC, ShaderType::TessellationControl),
           Shader(DSSSURFELIZE_TESE, ShaderType::TessellationEvaluation)},
          {"tePos", "teNormal", "teRadius", "teSigmaT", "teSigmaA"}),
      m_splattingshader{Shader(DSSSPLATTING_VERT, ShaderType::Vertex),
                        Shader(DSSSPLATTING_GEOM, ShaderType::Geometry),
                        Shader(DSSSPLATTING_FRAG, ShaderType::Fragment)},
      m_shuffleshader(
          {Shader(DSSPOSITIONNORMALSHUFFLER_VERT, ShaderType::Vertex),
           Shader(DSSPOSITIONNORMALSHUFFLER_FRAG, ShaderType::Fragment)}),
      m_unshuffleshader(
          {Shader(DSSSPLATTINGUNSHUFFLE_VERT, ShaderType::Vertex),
           Shader(DSSSPLATTINGUNSHUFFLE_FRAG, ShaderType::Fragment)}),
      m_sumupshader({Shader(DSSSUMUP_VERT, ShaderType::Vertex),
                     Shader(DSSSUMUP_FRAG, ShaderType::Fragment)}) {}
void DeepScreenSpace::copyFromUnshuffleToBlur() {
    auto app = Application::getContext();
    glCopyImageSubData(m_unshuffleresult->getId(), GL_TEXTURE_2D_ARRAY, 0, 0, 0,
                       0, m_blurer.getPingPongTex(0).getId(),
                       GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, app->getWidth(),
                       app->getHeight(), DSS_N_PARTITION_LAYERS);
}

void DeepScreenSpace::initPartition() {
    auto app = Application::getContext();
    int width = app->getWidth(), height = app->getHeight();
    m_fbpartitiontex = make_shared<Texture2DArray>();
    m_fbpartitiontex->init();
    m_fbpartitiontex->setupStorage(
        width, height, DSS_N_PARTITION_LAYERS, GL_RGBA32I, 1  // no mipmap, plz
    );
    m_fbpartitiontex->setSizeFilter(GL_NEAREST, GL_NEAREST);

    panicPossibleGLError();
    // the partition indexing only need do once on the cpu-end
    vector<ivec4> basebuffer(width * height, ivec4(0)),
        buffer(width * height, ivec4(-1));
    for (int i = 0; i < basebuffer.size(); i++) {
        basebuffer[i] = ivec4(i % width, i / width, i % width, i / width);
    }
    m_fbpartitiontex->setupLayer(0, basebuffer.data(), GL_RGBA_INTEGER, GL_INT);
    std::random_device rd;
    std::mt19937 g(rd());
    for (int j = 1, s = 2; j < DSS_N_PARTITION_LAYERS; j++, s *= 2) {
        int width_s = width / s, height_s = height / s;
        vector<int> shuffleIndex(s * s);
        std::iota(shuffleIndex.begin(), shuffleIndex.end(), 0);
        for (int i = 0; i < basebuffer.size() / (s * s); i++) {
            ivec2 in_offset(i % width_s, i / width_s);
            ivec2 buffer_base(in_offset.x, in_offset.y);
            buffer_base *= s;
            // shuffle for each subblock
            std::shuffle(shuffleIndex.begin(), shuffleIndex.end(), g);
            for (int k = 0; k < s * s; k++) {
                ivec2 out_offset((k % s) * width_s, (k / s) * height_s);
                ivec2 sub_pos(out_offset + in_offset);
                int m = shuffleIndex[k];
                ivec2 buffer_offset(m % s, m / s);
                ivec2 buffer_pos = buffer_base + buffer_offset;
                auto base_pos = basebuffer[buffer_pos.x + buffer_pos.y * width];
                buffer[sub_pos.x + sub_pos.y * width].x = base_pos.x;
                buffer[sub_pos.x + sub_pos.y * width].y = base_pos.y;
                buffer[base_pos.x + base_pos.y * width].z = sub_pos.x;
                buffer[base_pos.x + base_pos.y * width].w = sub_pos.y;
            }
        }
        m_fbpartitiontex->setupLayer(j, buffer.data(), GL_RGBA_INTEGER, GL_INT);
    }
    m_partitionfb.init();
    m_partitionednormal = make_shared<Texture2DArray>();
    m_partitionednormal->init();
    m_partitionednormal->setupStorage(width, height, DSS_N_PARTITION_LAYERS,
                                      GL_RGB8, 1);

    m_partitionedposition = make_shared<Texture2DArray>();
    m_partitionedposition->init();
    m_partitionedposition->setupStorage(width, height, DSS_N_PARTITION_LAYERS,
                                        GL_RGBA32F, 1);

    m_partitionednormaldebug = make_shared<Texture2D>();
    m_partitionednormaldebug->init();
    m_partitionednormaldebug->setupStorage(width, height, GL_RGB8, 1);

    m_partitionedpositiondebug = make_shared<Texture2D>();
    m_partitionedpositiondebug->init();
    m_partitionedpositiondebug->setupStorage(width, height, GL_RGB32F, 1);
}
void DeepScreenSpace::shufflePartitionPass(
    const loo::Texture2D& GBufferPosition,
    const loo::Texture2D& GBufferNormal) {
    m_partitionfb.bind();
    m_shuffleshader.use();
    for (int i = 0; i < DSS_N_PARTITION_LAYERS; i++) {
        m_partitionfb.attachTextureLayer(*m_partitionednormal,
                                         GL_COLOR_ATTACHMENT0, 0, i);
        m_partitionfb.attachTextureLayer(*m_partitionedposition,
                                         GL_COLOR_ATTACHMENT1, 0, i);
        m_partitionfb.enableAttachments(
            {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        m_shuffleshader.setTexture(0, GBufferNormal);
        m_shuffleshader.setTexture(1, GBufferPosition);
        m_shuffleshader.setTexture(2, *m_fbpartitiontex);
        m_shuffleshader.setUniform("currentLayer", i);
        Quad::globalQuad().draw();
    }
}

void DeepScreenSpace::initSurfelizePass() {

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
void DeepScreenSpace::init() {
    auto app = Application::getContext();
    int width = app->getWidth(), height = app->getHeight();
    {
        // splatting
        m_splattingfb.init();
        panicPossibleGLError();
        m_splattingresult = make_shared<Texture2DArray>();
        m_splattingresult->init();
        m_splattingresult->setupStorage(width, height, DSS_N_PARTITION_LAYERS,
                                        GL_RGB16F, 1);
        m_splattingresult->setSizeFilter(GL_LINEAR, GL_LINEAR);
        m_splattingfb.attachTexture(*m_splattingresult, GL_COLOR_ATTACHMENT0,
                                    0);

        m_splattingresultdebug = make_shared<Texture2D>();
        m_splattingresultdebug->init();
        m_splattingresultdebug->setupStorage(width, height, GL_RGB16F, 1);
        m_splattingresultdebug->setSizeFilter(GL_LINEAR, GL_LINEAR);

        m_blurresultdebug = make_shared<Texture2D>();
        m_blurresultdebug->init();
        m_blurresultdebug->setupStorage(width, height, GL_RGB16F, 1);
        m_blurresultdebug->setSizeFilter(GL_LINEAR, GL_LINEAR);

        panicPossibleGLError();
    }
    {
        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferStorage(GL_ARRAY_BUFFER, sizeof(SurfelData) * N_SURFELS_MAX,
                        nullptr, GL_DYNAMIC_STORAGE_BIT);
        SurfelData sd{};
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(SurfelData), &sd);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SurfelData),
                              (GLvoid*)offsetof(SurfelData, position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SurfelData),
                              (GLvoid*)(offsetof(SurfelData, normal)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(SurfelData),
                              (GLvoid*)(offsetof(SurfelData, radius)));
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        m_surfelbuffer.vao = vao;
        m_surfelbuffer.vbo = vbo;
    }
    {
        m_unshufflefb.init();
        m_unshuffleresult = make_shared<Texture2DArray>();
        m_unshuffleresult->init();
        m_unshuffleresult->setupStorage(width, height, DSS_N_PARTITION_LAYERS,
                                        GL_RGB16F, 1);
        m_unshuffleresult->setSizeFilter(GL_LINEAR, GL_LINEAR);
        m_unshufflefb.attachTexture(*m_unshuffleresult, GL_COLOR_ATTACHMENT0,
                                    0);
        m_unshufflefb.enableAttachments({GL_COLOR_ATTACHMENT0});
    }
    {
        unique_ptr<Texture2DArray> pingpong[2];
        for (int i = 0; i < 2; i++) {
            pingpong[i] = make_unique<Texture2DArray>();
            pingpong[i]->init();
            pingpong[i]->setupStorage(width, height, DSS_N_PARTITION_LAYERS,
                                      GL_RGB16F, 1);
            pingpong[i]->setSizeFilter(GL_LINEAR, GL_LINEAR);
        }
        m_blurer.init(std::move(pingpong[0]), std::move(pingpong[1]));
    }
    {
        m_sumupfb.init();
        m_sumuptex = make_shared<Texture2D>();
        m_sumuptex->init();
        m_sumuptex->setupStorage(width, height, GL_RGB16F, 1);
        m_sumuptex->setSizeFilter(GL_LINEAR, GL_LINEAR);
        m_sumupfb.attachTexture(*m_sumuptex, GL_COLOR_ATTACHMENT0, 0);
        m_sumupfb.enableAttachments({GL_COLOR_ATTACHMENT0});
    }
    initSurfelizePass();
    initPartition();
}
void DeepScreenSpace::surfelizePass(const Scene& scene,
                                    const loo::Camera& camera, MVP& mvp,
                                    loo::UniformBuffer& mvpBuffer) {

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_surfelizequery);
    logPossibleGLError();
    glEnable(GL_RASTERIZER_DISCARD);

    m_surfelizeshader.use();
    m_surfelizeshader.setUniform("aspect", camera.m_aspect);
    m_surfelizeshader.setUniform("scale", options.surfelScale);
    m_surfelizeshader.setUniform("viewMatrix", camera.getViewMatrix());
    m_surfelizeshader.setUniform("fov", camera.m_fov);
    m_surfelizeshader.setUniform("cameraPosition", camera.getPosition());

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
    panicPossibleGLError();
}

void DeepScreenSpace::splattingPass(const loo::Camera& camera,
                                    const loo::ShaderLight& mainLight,
                                    const loo::Texture2D& mainLightShadowMap) {
    m_splattingfb.bind();
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glBlendFunc(GL_ONE, GL_ONE);
    m_splattingshader.use();
    if (getSurfelCount()) {
        m_splattingshader.setUniform("cameraPos", camera.getPosition());
        m_splattingshader.setUniform("viewMatrix", camera.getViewMatrix());
        m_splattingshader.setUniform("projectionMatrix",
                                     camera.getProjectionMatrix());
        m_splattingshader.setUniform("framebufferDeviceStep.resolution",
                                     ivec2(Application::getContextWidth(),
                                           Application::getContextHeight()));
        m_splattingshader.setUniform("minimalEffect", options.minimalEffect);
        m_splattingshader.setUniform("maxDistance", options.maxDistance);
        m_splattingshader.setUniform("strength", options.splattingStrength);
        m_splattingshader.setUniform("fov", camera.m_fov);
        m_splattingshader.setUniform("lightSpaceMatrix",
                                     mainLight.getLightSpaceMatrix());

        m_splattingshader.setTexture(0, *m_partitionedposition);
        m_splattingshader.setTexture(1, *m_partitionednormal);
        m_splattingshader.setTexture(2, mainLightShadowMap);
        glBindVertexArray(m_surfelbuffer.vao);
        glDrawArrays(GL_POINTS, 0, getSurfelCount());
        logPossibleGLError();
    }
    glDisable(GL_BLEND);
    m_splattingfb.unbind();
}

void DeepScreenSpace::unshufflePartitionPass() {
    m_unshufflefb.bind();
    m_unshuffleshader.use();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    // unshuffle according to the original shuffle ordering
    m_unshuffleshader.setTexture(0, *m_fbpartitiontex);
    m_unshuffleshader.setTexture(1, *m_splattingresult);
    Quad::globalQuad().drawInstances(DSS_N_PARTITION_LAYERS);
    m_unshufflefb.unbind();
}
void DeepScreenSpace::blurPass() {
    copyFromUnshuffleToBlur();
    m_blurer.blur(Quad::globalQuad(), DSS_N_PARTITION_LAYERS,
                  GaussianBlurDirection::X);
    m_blurer.blur(Quad::globalQuad(), DSS_N_PARTITION_LAYERS,
                  GaussianBlurDirection::Y);
}

void DeepScreenSpace::sumUpPass() {
    m_sumupfb.bind();
    m_sumupshader.use();
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    m_sumupshader.setTexture(0, m_blurer.getBlurResult());
    Quad::globalQuad().drawInstances(DSS_N_PARTITION_LAYERS);
    glDisable(GL_BLEND);
}

const loo::Texture2D& DeepScreenSpace::getPartitionedPosition() {
    glCopyImageSubData(m_partitionedposition->getId(), GL_TEXTURE_2D_ARRAY, 0,
                       0, 0, options.debugLayer,
                       m_partitionedpositiondebug->getId(), GL_TEXTURE_2D, 0, 0,
                       0, 0, Application::getContextWidth(),
                       Application::getContextHeight(), 1);
    logPossibleGLError();
    return *m_partitionedpositiondebug;
}
const loo::Texture2D& DeepScreenSpace::getPartitionedNormal() {
    glCopyImageSubData(m_partitionednormal->getId(), GL_TEXTURE_2D_ARRAY, 0, 0,
                       0, options.debugLayer, m_partitionednormaldebug->getId(),
                       GL_TEXTURE_2D, 0, 0, 0, 0,
                       Application::getContextWidth(),
                       Application::getContextHeight(), 1);
    logPossibleGLError();
    return *m_partitionednormaldebug;
}
const loo::Texture2D& DeepScreenSpace::getSplattingResult(bool unshuffle) {
    glCopyImageSubData(
        unshuffle ? m_unshuffleresult->getId() : m_splattingresult->getId(),
        GL_TEXTURE_2D_ARRAY, 0, 0, 0, options.debugLayer,
        m_splattingresultdebug->getId(), GL_TEXTURE_2D, 0, 0, 0, 0,
        Application::getContextWidth(), Application::getContextHeight(), 1);
    logPossibleGLError();
    return *m_splattingresultdebug;
}

const loo::Texture2D& DeepScreenSpace::getBlurResult() {
    glCopyImageSubData(
        m_blurer.getBlurResult().getId(), GL_TEXTURE_2D_ARRAY, 0, 0, 0,
        options.debugLayer, m_blurresultdebug->getId(), GL_TEXTURE_2D, 0, 0, 0,
        0, Application::getContextWidth(), Application::getContextHeight(), 1);
    logPossibleGLError();
    return *m_blurresultdebug;
}
