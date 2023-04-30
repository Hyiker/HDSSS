#ifndef HDSSS_SHADERS_INCLUDE_SURFEL_GLSL
#define HDSSS_SHADERS_INCLUDE_SURFEL_GLSL

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

Surfel initSurfel(vec3 position, vec3 normal, float radius, vec3 sigma_a,
                  vec3 sigma_t, vec3 irradiance) {
    // using marble parameters from Jensen'01
    // TODO: make it configurable
    vec3 sigma_a_m = sigma_a * 1e3;
    vec3 sigma_s_prime_m = (sigma_t - sigma_a) * 1e3;
    return Surfel(position, normal, radius, irradiance, sigma_a_m,
                  sigma_s_prime_m);
}

#endif /* HDSSS_SHADERS_INCLUDE_SURFEL_GLSL */
