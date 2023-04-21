/**
 * Shader.cpp
 * Contributors:
 *      * Arthur Sonzogni (author)
 * Licence:
 *      * MIT
 */

#include "loo/Shader.hpp"

#include <glog/logging.h>

#include <cstdlib>
#include <format>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace loo {

using namespace std;
using namespace glm;

// file reading
void getFileContents(const char* filename, vector<char>& buffer) {
    ifstream file(filename, ios_base::binary);
    if (file) {
        file.seekg(0, ios_base::end);
        streamsize size = file.tellg();
        if (size > 0) {
            file.seekg(0, ios_base::beg);
            buffer.resize(static_cast<size_t>(size));
            file.read(&buffer[0], size);
        }
        buffer.push_back('\0');
    } else {
        throw std::invalid_argument(string("The file ") + filename +
                                    " doesn't exists");
    }
}

#ifdef OGL_46
// https://www.khronos.org/opengl/wiki/SPIR-V
Shader::Shader(const vector<unsigned char>& spirvBinary, GLenum type,
               const char* entryPoint) {
    // Create an empty vertex shader handle
    handle = glCreateShader(type);

    // Apply the vertex shader SPIR-V to the shader object.
    glShaderBinary(1, &handle, GL_SHADER_BINARY_FORMAT_SPIR_V,
                   spirvBinary.data(), spirvBinary.size());

    // Specialize the vertex shader.
    glSpecializeShader(handle, entryPoint, 0, nullptr, nullptr);

    // Specialization is equivalent to compilation.
    checkCompileStatus();
}

#endif

Shader::Shader(Shader&& other) {
    this->handle = other.getHandle();
    other.handle = GL_INVALID_INDEX;
}

Shader::Shader(const char* shaderContent, GLenum type) {
    // creation
    handle = glCreateShader(type);
    if (handle == 0)
        throw std::runtime_error("[Error] Impossible to create a new Shader");

    // code source assignation
    const char* shaderText(shaderContent);
    glShaderSource(handle, 1, (const GLchar**)&shaderText, nullptr);

    // compilation
    glCompileShader(handle);
    checkCompileStatus();
}

void Shader::checkCompileStatus() const {
    // compilation check
    GLint compile_status;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &compile_status);
    if (compile_status != GL_TRUE) {
        GLsizei logsize = 0;
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logsize);

        char* log = new char[logsize + 1];
        glGetShaderInfoLog(handle, logsize, &logsize, log);
        LOG(ERROR) << log << endl;
        throw ShaderCompileException(log);
    }
}

GLuint Shader::getHandle() const { return handle; }

Shader::~Shader() {
    if (handle != GL_INVALID_INDEX) glDeleteShader(handle);
}

Shader createShaderFromFile(const std::string& filename, GLenum type) {
    // file loading
    vector<char> fileContent;
    getFileContents(filename.c_str(), fileContent);

    try {
        Shader shader(fileContent.data(), type);
        return std::move(shader);
    } catch (ShaderCompileException& e) {
        LOG(FATAL) << format("Compile error: {}", e.what()) << endl;
    }
}

ShaderProgram::ShaderProgram() {
    handle = glCreateProgram();
    if (!handle)
        throw std::runtime_error("Impossible to create a new shader program");
}

ShaderProgram::ShaderProgram(std::initializer_list<Shader> shaderList)
    : ShaderProgram() {
    for (auto& s : shaderList) glAttachShader(handle, s.getHandle());

    link();
    for (auto& s : shaderList) glDetachShader(handle, s.getHandle());
}

ShaderProgram::ShaderProgram(ShaderProgram&& other)
    : uniforms{std::move(other.uniforms)},
      attributes{std::move(other.attributes)},
      handle{other.handle} {
    other.handle = GL_INVALID_INDEX;
}

void ShaderProgram::link() {
    glLinkProgram(handle);
    GLint result;
    glGetProgramiv(handle, GL_LINK_STATUS, &result);
    if (result != GL_TRUE) {
        LOG(ERROR) << "Linkage error" << endl;

        GLsizei logsize = 0;
        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &logsize);

        char* log = new char[logsize];
        glGetProgramInfoLog(handle, logsize, &logsize, log);

        LOG(ERROR) << log << endl;
    }
}

GLint ShaderProgram::uniform(const std::string& name) {
    auto it = uniforms.find(name);
    if (it == uniforms.end()) {
        // uniform that is not referenced
        GLint r = glGetUniformLocation(handle, name.c_str());
        if (r == GL_INVALID_OPERATION || r < 0)
            LOG(ERROR) << "Uniform " << name << " doesn't exist in program";
        // add it anyways
        uniforms[name] = r;

        return r;
    } else
        return it->second;
}

GLint ShaderProgram::attribute(const std::string& name) {
    GLint attrib = glGetAttribLocation(handle, name.c_str());
    if (attrib == GL_INVALID_OPERATION || attrib < 0)
        LOG(ERROR) << "Attribute " << name << " doesn't exist in program";

    return attrib;
}

void ShaderProgram::setAttribute(const std::string& name, GLint size,
                                 GLsizei stride, GLuint offset,
                                 GLboolean normalize, GLenum type) {
    GLint loc = attribute(name);
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, size, type, normalize, stride,
                          reinterpret_cast<void*>(offset));
}

void ShaderProgram::setAttribute(const std::string& name, GLint size,
                                 GLsizei stride, GLuint offset,
                                 GLboolean normalize) {
    setAttribute(name, size, stride, offset, normalize, GL_FLOAT);
}

void ShaderProgram::setAttribute(const std::string& name, GLint size,
                                 GLsizei stride, GLuint offset, GLenum type) {
    setAttribute(name, size, stride, offset, false, type);
}

void ShaderProgram::setAttribute(const std::string& name, GLint size,
                                 GLsizei stride, GLuint offset) {
    setAttribute(name, size, stride, offset, false, GL_FLOAT);
}

void ShaderProgram::setUniform(const std::string& name, float x, float y,
                               float z) {
    glUniform3f(uniform(name), x, y, z);
}

void ShaderProgram::setUniform(const std::string& name, const vec3& v) {
    glUniform3fv(uniform(name), 1, value_ptr(v));
}
void ShaderProgram::setUniform(const std::string& name, const vec2& v) {
    glUniform2fv(uniform(name), 1, value_ptr(v));
}

void ShaderProgram::setUniform(const std::string& name, const glm::ivec2& v) {
    glUniform2iv(uniform(name), 1, value_ptr(v));
}
void ShaderProgram::setUniform(const std::string& name, const glm::ivec3& v) {
    glUniform3iv(uniform(name), 1, value_ptr(v));
}

void ShaderProgram::setUniform(const std::string& name, const dvec3& v) {
    glUniform3dv(uniform(name), 1, value_ptr(v));
}

void ShaderProgram::setUniform(const std::string& name, const vec4& v) {
    glUniform4fv(uniform(name), 1, value_ptr(v));
}

void ShaderProgram::setUniform(const std::string& name, const dvec4& v) {
    glUniform4dv(uniform(name), 1, value_ptr(v));
}

void ShaderProgram::setUniform(const std::string& name, const dmat4& m) {
    glUniformMatrix4dv(uniform(name), 1, GL_FALSE, value_ptr(m));
}

void ShaderProgram::setUniform(const std::string& name, const mat4& m) {
    glUniformMatrix4fv(uniform(name), 1, GL_FALSE, value_ptr(m));
}

void ShaderProgram::setUniform(const std::string& name, const mat3& m) {
    glUniformMatrix3fv(uniform(name), 1, GL_FALSE, value_ptr(m));
}

void ShaderProgram::setUniform(const std::string& name, float val) {
    glUniform1f(uniform(name), val);
}

void ShaderProgram::setUniform(const std::string& name, int val) {
    glUniform1i(uniform(name), val);
}

void ShaderProgram::setTexture(const std::string& name, int index, int texId,
                               GLenum texType) {
    glActiveTexture(GL_TEXTURE0 + index);
    glUniform1i(uniform(name), index);
    glBindTexture(texType, texId);

    glActiveTexture(GL_TEXTURE0);
}

ShaderProgram::~ShaderProgram() { glDeleteProgram(handle); }

void ShaderProgram::use() const { glUseProgram(handle); }
void ShaderProgram::unuse() const { glUseProgram(0); }

GLuint ShaderProgram::getHandle() const { return handle; }

}  // namespace loo