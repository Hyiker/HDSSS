#ifndef DEEPSCREENSPACE_SHADERS_INCLUDE_MATH_GLSL
#define DEEPSCREENSPACE_SHADERS_INCLUDE_MATH_GLSL
#define PI 3.14159265359
#define PI_SQR 9.8696044011

float triangleArea(in vec3 a, in vec3 b, in vec3 c) {
    vec3 e1 = b - a, e2 = c - a;
    return length(cross(e1, e2)) / 2.0;
}
float sqr(in float v) {
    return v * v;
}
float degToRad(float deg) {
    return deg * 0.0174532925;
}
#endif /* DEEPSCREENSPACE_SHADERS_INCLUDE_MATH_GLSL */
