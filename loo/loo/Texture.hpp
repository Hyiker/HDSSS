#ifndef LOO_LOO_TEXTURE_HPP
#define LOO_LOO_TEXTURE_HPP
#include <glad/glad.h>

#include <array>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "loo/glError.hpp"
#include "predefs.hpp"

namespace loo {

template <GLenum Target>
class LOO_EXPORT Texture {
   protected:
    GLuint m_id{GL_INVALID_INDEX};
    GLsizei width{-1}, height{-1};

   public:
    GLsizei getWidth() { return width; }
    GLsizei getHeight() { return height; }
    void init() {
#ifdef OGL_46
        glCreateTextures(Target, 1, &m_id);
#else
        glGenTextures(1, &m_id);
#endif
    }
    // for opengl 4.5+ use glBindTextureUnit
    void bind() const {
#ifndef OGL_46
        glBindTexture(Target, m_id);
#endif
    }
    void unbind() const {
#ifndef OGL_46
        glBindTexture(Target, 0);
#endif
    }
    GLuint getId() const { return m_id; };
    constexpr GLenum getType() const { return Target; }
    int getMipmapLevels() const {
        unsigned int lvl = 0;
        int width = this->width, height = this->height;
        while ((width | height) >> 1) {
            width >>= 1;
            height >>= 1;
            lvl++;
        }
        return lvl;
    }
    void generateMipmap() {
#ifdef OGL_46
        glGenerateTextureMipmap(m_id);
#else
        bind();
        glGenerateMipmap(Target);
        unbind();
#endif
    }
    void setSizeFilter(GLenum min_filter, GLenum mag_filter) {
#ifdef OGL_46
        glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, min_filter);
        glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, mag_filter);
#else
        bind();
        glTexParameteri(Target, GL_TEXTURE_MIN_FILTER, min_filter);
        glTexParameteri(Target, GL_TEXTURE_MAG_FILTER, mag_filter);
        unbind();
#endif
    }
    void setWrapFilter(GLenum filter) {
#ifdef OGL_46
        glTextureParameteri(m_id, GL_TEXTURE_WRAP_S, filter);
        glTextureParameteri(m_id, GL_TEXTURE_WRAP_T, filter);
        glTextureParameteri(m_id, GL_TEXTURE_WRAP_R, filter);
#else
        bind();
        glTexParameteri(Target, GL_TEXTURE_WRAP_S, filter);
        glTexParameteri(Target, GL_TEXTURE_WRAP_T, filter);
        glTexParameteri(Target, GL_TEXTURE_WRAP_R, filter);
        unbind();
#endif
    }
    void setClampToBorderFilter(GLfloat* borderColor) {
#ifdef OGL_46
        glTextureParameteri(m_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(m_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTextureParameterfv(m_id, GL_TEXTURE_BORDER_COLOR, borderColor);
#else
        bind();
        glTexParameteri(Target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(Target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(Target, GL_TEXTURE_BORDER_COLOR, borderColor);
        unbind();
#endif
    }
};
constexpr unsigned int TEXTURE_OPTION_MIPMAP = 0x1,
                       TEXTURE_OPTION_CONVERT_TO_LINEAR = 0x2;
class LOO_EXPORT Texture2D : public Texture<GL_TEXTURE_2D> {
    static Texture2D whiteTexture;
    static Texture2D blackTexture;

   public:
    // storage only
    void setupStorage(GLsizei width, GLsizei height, GLenum internalformat,
                      GLsizei maxLevel);
    void setup(unsigned char* data, GLsizei width, GLsizei height,
               GLenum internalformat, GLenum format, GLenum type,
               GLint maxLevel = -1);
    static const Texture2D& getWhiteTexture();
    static const Texture2D& getBlackTexture();
};

LOO_EXPORT std::shared_ptr<Texture2D> createTexture2DFromFile(
    std::unordered_map<std::string, std::shared_ptr<Texture2D>>& uniqueTexture,
    const std::string& filename, unsigned int options);

// texture array is a special type of texture
// it contains multiple texture with only one texture name
class LOO_EXPORT Texture2DArray : public Texture<GL_TEXTURE_2D_ARRAY> {
    GLsizei depth{-1};

   public:
    void setupStorage(GLsizei width, GLsizei height, GLsizei depth,
                      GLenum internalformat, int maxLevel = -1);
    void setupLayer(GLsizei layer, const void* data, GLenum format,
                    GLenum type);
};

class LOO_EXPORT TextureCubeMap : public Texture<GL_TEXTURE_CUBE_MAP> {
   public:
    // auto builder = TextureCubeMap::builder().
    class TextureCubeMapBuilder {
        std::map<GLenum, std::string> m_face2filename;
        std::filesystem::path m_prefix{"."};
        std::string m_ext{".jpg"};

       public:
        TextureCubeMapBuilder& prefix(const std::string& prefixPath) {
            m_prefix = prefixPath;
            return *this;
        }
        TextureCubeMapBuilder& extension(const std::string& ext) {
            m_ext = ext;
            return *this;
        }
        TextureCubeMapBuilder& right(const std::string& filename) {
            m_face2filename[GL_TEXTURE_CUBE_MAP_POSITIVE_X] = filename;
            return *this;
        }
        TextureCubeMapBuilder& left(const std::string& filename) {
            m_face2filename[GL_TEXTURE_CUBE_MAP_NEGATIVE_X] = filename;
            return *this;
        }
        TextureCubeMapBuilder& top(const std::string& filename) {
            m_face2filename[GL_TEXTURE_CUBE_MAP_POSITIVE_Y] = filename;
            return *this;
        }
        TextureCubeMapBuilder& bottom(const std::string& filename) {
            m_face2filename[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y] = filename;
            return *this;
        }
        TextureCubeMapBuilder& back(const std::string& filename) {
            m_face2filename[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z] = filename;
            return *this;
        }
        TextureCubeMapBuilder& front(const std::string& filename) {
            m_face2filename[GL_TEXTURE_CUBE_MAP_POSITIVE_Z] = filename;
            return *this;
        }
        std::vector<std::string> build();
    };
    static TextureCubeMapBuilder builder() { return {}; }
    void setupStorage(GLsizei width, GLsizei height, GLenum internalformat,
                      int maxLevel = -1);
    // face is indexed [0, 5]
    void setupFace(int face, unsigned char* data, GLenum format, GLenum type);
};
// we assume cubemap texture doesn't need deduplicate
LOO_EXPORT std::shared_ptr<TextureCubeMap> createTextureCubeMapFromFiles(
    const std::vector<std::string>& filenames, unsigned int options);
}  // namespace loo
#endif /* LOO_LOO_TEXTURE_HPP */
