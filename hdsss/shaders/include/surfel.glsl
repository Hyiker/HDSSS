#ifndef HDSSS_SHADERS_INCLUDE_SURFEL_GLSL
#define HDSSS_SHADERS_INCLUDE_SURFEL_GLSL
struct SurfelData {
    vec4 position;
    vec3 normal;
    float radius;
};

struct Surfel {
    vec3 position;
    vec3 normal;
    float radius;
    vec3 light;
    // BSSRDF parameters in m^-1
    // absorption coefficient
    vec3 sigma_a;
    // reduced scattering coefficient
    vec3 sigma_s_prime;
    // int materialId;
};

Surfel initSurfel(vec3 position, vec3 normal, float radius, vec3 irradiance) {
    // using marble parameters from Jensen'01
    // TODO: make it configurable
    vec3 sigma_a = vec3(0.0021, 0.0041, 0.0071) * 1e3;
    vec3 sigma_s_prime = vec3(2.19, 4.62, 2.00) * 1e3;
    return Surfel(position, normal, radius, irradiance, sigma_a, sigma_s_prime);
}

#endif /* HDSSS_SHADERS_INCLUDE_SURFEL_GLSL */
