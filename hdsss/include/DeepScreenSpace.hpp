#ifndef HDSSS_INCLUDE_DEEP_SCREEN_SPACE_HPP
#define HDSSS_INCLUDE_DEEP_SCREEN_SPACE_HPP
#include <loo/AtomicCounter.hpp>
#include <loo/Framebuffer.hpp>
#include <loo/Light.hpp>
#include <loo/Quad.hpp>
#include <loo/Shader.hpp>
#include <loo/Texture.hpp>
#include <memory>
#include "GaussianBlur.hpp"
#include "Surfel.hpp"
#include "Transforms.hpp"
#include "constants.hpp"

struct DSSOptions {
    float surfelScale{1.0f};
    float splattingStrength{15.0f};
    float minimalEffect{0.001f};
    float maxDistance{0.01f};
    int debugLayer{0};
};

class DeepScreenSpace {
    // mesh surfelize shader
    // only contains vertex & tessellation stages
    loo::ShaderProgram m_surfelizeshader;

    // splatting related
    loo::Framebuffer m_splattingfb;
    // make use of surfelize result
    loo::ShaderProgram m_splattingshader;

    std::shared_ptr<loo::Texture2DArray> m_splattingresult;
    // unshuffle the splatting result
    std::shared_ptr<loo::Texture2DArray> m_unshuffleresult;
    std::shared_ptr<loo::Texture2D> m_splattingresultdebug, m_blurresultdebug;

    GLuint m_surfelizetf, m_surfelizequery;
    struct SurfelBuffer {
        GLuint vao;
        GLuint vbo;
    } m_surfelbuffer;
    // this texture describes how framebuffer is partitioned(shuffled)
    // on the different layers
    // xy for shuffling
    // zw for unshuffling
    std::shared_ptr<loo::Texture2DArray> m_fbpartitiontex;
    // shuffled partition result
    std::shared_ptr<loo::Texture2DArray> m_partitionedposition,
        m_partitionednormal;
    // debug display
    std::shared_ptr<loo::Texture2D> m_partitionedpositiondebug,
        m_partitionednormaldebug;

    // framebuffer for
    loo::Framebuffer m_partitionfb, m_unshufflefb;
    loo::ShaderProgram m_shuffleshader;
    loo::ShaderProgram m_unshuffleshader;

    GaussianBlurMultilayer m_blurer;

    // sum up to get the final output
    loo::Framebuffer m_sumupfb;
    loo::ShaderProgram m_sumupshader;
    std::shared_ptr<loo::Texture2D> m_sumuptex;

    int m_surfelcount{0};

    void copyFromUnshuffleToBlur();

    void initSurfelizePass();

    void initPartition();

   public:
    DeepScreenSpace();
    void init();
    // pass 1: shuffle the gbuffer
    void shufflePartitionPass(const loo::Texture2D& GBufferPosition,
                              const loo::Texture2D& GBufferNormal);

    // pass 2: surfelize the scene
    void surfelizePass(const loo::Scene& scene, const loo::Camera& camera,
                       MVP& mvp, loo::UniformBuffer& mvpBuffer);

    // pass 3: splatting
    void splattingPass(const loo::Camera& camera,
                       const loo::ShaderLight& mainLight,
                       const loo::Texture2D& mainLightShadowMap);
    // pass 4: unshuffle the splatting result
    void unshufflePartitionPass();
    // pass 5: blur the splatting result
    void blurPass();
    // pass 6: sum up
    void sumUpPass();

    int getSurfelCount() const { return m_surfelcount; }
    auto getSplattingResult() const { return m_splattingresult; }
    auto getPartitionedPosition() const { return m_partitionedposition; }
    auto getPartitionedNormal() const { return m_partitionednormal; }
    const loo::Texture2D& getPartitionedPosition();
    const loo::Texture2D& getPartitionedNormal();
    const loo::Texture2D& getSplattingResult(bool unshuffle = false);
    const loo::Texture2D& getBlurResult();
    const auto& getSumUpResult() { return *m_sumuptex; }

    DSSOptions options;
};

#endif /* HDSSS_INCLUDE_DEEP_SCREEN_SPACE_HPP */
