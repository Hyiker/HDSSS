#ifndef LOO_LOO_FRAMEBUFFER_HPP
#define LOO_LOO_FRAMEBUFFER_HPP
#include <glad/glad.h>

#include <set>

#include "Texture.hpp"
#include "glog/logging.h"
#include "predefs.hpp"

namespace loo {

class LOO_EXPORT Renderbuffer {
    GLuint m_rbo{GL_INVALID_INDEX};

   public:
    Renderbuffer() = default;
    Renderbuffer(const Renderbuffer&) = delete;
    Renderbuffer& operator=(const Renderbuffer&) = delete;
    Renderbuffer(Renderbuffer&& buffer) noexcept : m_rbo(buffer.m_rbo) {
        buffer.m_rbo = GL_INVALID_INDEX;
    }
    void bind() const { glBindRenderbuffer(GL_RENDERBUFFER, m_rbo); }
    void unbind() const { glBindRenderbuffer(GL_RENDERBUFFER, 0); }
    void init(GLenum internalformat, GLsizei width, GLsizei height) {
#ifdef OGL_46
        glCreateRenderbuffers(1, &m_rbo);
        glNamedRenderbufferStorage(m_rbo, internalformat, width, height);
#else
        glGenRenderbuffers(1, &m_rbo);
        bind();
        glRenderbufferStorage(GL_RENDERBUFFER, internalformat, width, height);
        unbind();
#endif
    }
    GLuint getId() const { return m_rbo; }
    ~Renderbuffer() { glDeleteRenderbuffers(1, &m_rbo); }
};
class Framebuffer {
    GLuint m_fbo{GL_INVALID_INDEX};
    std::set<GLenum> m_attachments;

   public:
    Framebuffer() = default;
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    static void bindDefault() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
    Framebuffer(Framebuffer&& buffer) noexcept : m_fbo(buffer.m_fbo) {
        buffer.m_fbo = GL_INVALID_INDEX;
    }
    void init() {
#ifdef OGL_46
        glCreateFramebuffers(1, &m_fbo);
#else
        glGenFramebuffers(1, &m_fbo);
#endif
    }
    GLuint getId() const { return m_fbo; }

    void bind() const { glBindFramebuffer(GL_FRAMEBUFFER, m_fbo); }
    void unbind() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
    template <GLenum TextureType>
    void attachTexture(const Texture<TextureType>& tex, GLenum attachment,
                       GLint level) {
#ifdef OGL_46
        glNamedFramebufferTexture(m_fbo, attachment, tex.getId(), level);
#else
        bind();
        tex.bind();
        glFramebufferTexture(GL_FRAMEBUFFER, attachment, tex.getId(), level);
        tex.unbind();
        unbind();
#endif
        m_attachments.insert(attachment);
    }
    template <GLenum TextureType>
    void attachTextureLayer(const Texture<TextureType>& tex, GLenum attachment,
                            GLint level, GLint layer) {
#ifdef OGL_46
        glNamedFramebufferTextureLayer(m_fbo, attachment, tex.getId(), level,
                                       layer);
#else
        NOT_IMPLEMENTED();
#endif
        m_attachments.insert(attachment);
    }
    void attachRenderbuffer(const Renderbuffer& rb, GLenum attachment) {
#ifdef OGL_46
        glNamedFramebufferRenderbuffer(m_fbo, attachment, GL_RENDERBUFFER,
                                       rb.getId());
#else
        bind();
        rb.bind();
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER,
                                  rb.getId());
        rb.unbind();
        unbind();
#endif
    }
    // call glDrawBuffers to enable attachments
    void enableAttachments(const std::vector<GLenum>& attachments) {
        for (auto attachment : attachments) {
            CHECK_NE(m_attachments.count(attachment), 0);
        }
#ifdef OGL_46
        glNamedFramebufferDrawBuffers(m_fbo, attachments.size(),
                                      attachments.data());
#else
        bind();
        glDrawBuffers(attachments.size(), attachments.data());
        unbind();
#endif
    }
    ~Framebuffer() { glDeleteFramebuffers(1, &m_fbo); }
};
}  // namespace loo

#endif /* LOO_LOO_FRAMEBUFFER_HPP */
