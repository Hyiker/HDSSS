#ifndef HDSSS_INCLUDE_FINAL_PROCESS_HPP
#define HDSSS_INCLUDE_FINAL_PROCESS_HPP
#include <loo/Framebuffer.hpp>
#include <loo/Quad.hpp>
#include <loo/Shader.hpp>
#include <loo/Texture.hpp>
#include <memory>

struct FinalPassOptions {
    bool diffuse{false};
    bool specular{true};
    bool translucency{true};
    bool SSS{true};
    float SSSStrength{1.0f};
    bool directOutput{false};
};
class FinalProcess {
    loo::ShaderProgram m_shader;
    int m_width, m_height;

   public:
    FinalProcess(int width, int height);
    void init();
    // if use direct output, this pass will just out put the previous rendering
    // result without doing any addition postprocessing
    void render(const loo::Texture2D& diffuseTexture,
                const loo::Texture2D& specularTexture,
                const loo::Texture2D& translucencyTexture,
                const loo::Texture2D& sssTexture,
                const loo::Texture2D& GBuffer3,
                const loo::Texture2D& skyboxTexture,
                const FinalPassOptions& options);
};

#endif /* HDSSS_INCLUDE_FINAL_PROCESS_HPP */
