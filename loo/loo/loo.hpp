#ifndef LOO_LOO_LOO_HPP
#define LOO_LOO_LOO_HPP
#include <algorithm>

#include <filesystem>
#include <functional>
#include <memory>
#include "predefs.hpp"

namespace loo {
class Shader;
class ShaderProgram;
class Application;
class Camera;
class Scene;
class UniformBuffer;
class Material;

// Initialize libraries include:
// * glog
// * shaderMaterialBindingPort

LOO_EXPORT void initialize(const char* argv0);

}  // namespace loo

#endif /* LOO_LOO_LOO_HPP */
