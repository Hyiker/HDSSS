#ifndef HDSSS_INCLUDE_CONSTANTS_HPP
#define HDSSS_INCLUDE_CONSTANTS_HPP

constexpr int SHADER_BINDING_PORT_SKYBOX = 0;

// light binding
constexpr int SHADER_BINDING_LIGHTS = 1;
constexpr int SHADER_BINDING_PORT_NLIGHTS = 114;

// simple material binding
constexpr int SHADER_BINDING_PORT_SM_PARAMS = 2;
constexpr int SHADER_BINDING_PORT_SM_AMBIENT = 3;
constexpr int SHADER_BINDING_PORT_SM_DIFFUSE = 4;
constexpr int SHADER_BINDING_PORT_SM_SPECULAR = 5;
constexpr int SHADER_BINDING_PORT_SM_DISPLACEMENT = 6;
constexpr int SHADER_BINDING_PORT_SM_NORMAL = 7;
constexpr int SHADER_BINDING_PORT_SM_OPACITY = 8;
constexpr int SHADER_BINDING_PORT_SM_HEIGHT = 9;

constexpr int SHADER_LIGHTS_MAX = 12;

constexpr int N_SURFELS_MAX = 20000000;

#endif /* HDSSS_INCLUDE_CONSTANTS_HPP */
