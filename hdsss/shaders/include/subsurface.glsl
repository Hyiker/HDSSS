#ifndef HDSSS_SHADERS_INCLUDE_SUBSURFACE_HPP
#define HDSSS_SHADERS_INCLUDE_SUBSURFACE_HPP

#extension GL_GOOGLE_include_directive : enable
#include "./surfel.glsl"

struct SplatReceiver {
    vec3 position;
    vec3 normal;
};
float eta = 1.3;
float clampDistance = 1e-3;
float sizeFactor = 1.0;
vec3 computeRadiantExitance(const in vec3 x_in, const in vec3 x_out,
                            const in float sampleArea,
                            const in vec3 sigma_s_prime,
                            const in vec3 sigma_a) {
    vec3 v = x_out - x_in;  // Vector from receiving point to surfel center
    float r_square =
        dot(v, v);  // Squared distance between surfel and exiting point

    r_square *= sizeFactor * sizeFactor;
    r_square = max(clampDistance * clampDistance, r_square);

    // vec3 sigma_s_prime = (1.0 - g) * sigma_s;
    // // Reduced scattering coefficient

    vec3 sigma_t_prime =
        sigma_s_prime + sigma_a;  // Reduced extinction coefficient
    vec3 sigma_tr =
        sqrt(3.0 * sigma_a *
             sigma_t_prime);  // Effective transport extinction coefficient

    vec3 l_u = 1.0 / sigma_t_prime;  // Mean-free path

    float F_dr = -1.44 / (eta * eta) + 0.71 / eta + 0.668 +
                 0.0636 * eta;  // Diffuse Fresnel term
    float A = (1.0 + F_dr) / (1.0 - F_dr);

    vec3 z_r = l_u;  // Distance of inner dipole light to surface
    vec3 z_v =
        l_u * (1.0 + 1.3333 * A);  // Distance of outer dipole light to surface

    vec3 d_r = sqrt(r_square + z_r * z_r);  // Distance to real light source
    vec3 d_v = sqrt(r_square + z_v * z_v);  // Distance to virtual light source

    vec3 C_1 = z_r * (sigma_tr + 1.0 / d_r);
    vec3 C_2 = z_v * (sigma_tr + 1.0 / d_v);

    return (1.0 - F_dr) *
           (C_1 * exp(-sigma_tr * d_r) / (d_r * d_r) +
            C_2 * exp(-sigma_tr * d_v) / (d_v * d_v)) *
           0.25 * (1.0 / PI) * sampleArea;
}

float radianceFactor(const in vec3 direction) {
    // float F_t =
    // // Fresnel transmittance
    // float F_dr = -1.44f/(eta*eta) + 0.71f/eta +
    // 0.668f
    // + 0.0636f*eta;	// Diffuse Fresnel term
    // return F_t / (F_dr*M_PI);
    return 1.0 / PI;
}

vec3 computeEffect(const in Surfel surfel, const in vec3 adjustedXi,
                   const in SplatReceiver receiver) {
    vec3 sigma_a = surfel.sigma_a;
    vec3 sigma_s_prime = surfel.sigma_s_prime;

    return radianceFactor(vec3(0.0, 0.0, 0.0)) *
           computeRadiantExitance(adjustedXi, receiver.position,
                                  surfel.radius * surfel.radius * PI,
                                  sigma_s_prime, sigma_a) *
           surfel.light;
}

#endif /* HDSSS_SHADERS_INCLUDE_SUBSURFACE_HPP */
