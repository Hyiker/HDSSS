#ifndef HDSSS_SHADERS_INCLUDE_SUBSURFACE_HPP
#define HDSSS_SHADERS_INCLUDE_SUBSURFACE_HPP

#extension GL_GOOGLE_include_directive : enable
#include "./surfel.glsl"

struct SplatReceiver {
    vec3 position;
    vec3 normal;
};
float eta = 1.5;
float clampDistance = 1e-3;
float sizeFactor = 1.3;
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

float fresnelTransmittance(in float cosTheta, in float eta) {
    float c = abs(cosTheta);
    float g_s = sqrt(pow(eta, 2) + c * c - 1);
    float gmc = g_s - c;
    float gpc = g_s + c;
    float R = ((gmc / gpc) * (gmc / gpc)) / 2 *
              (1 + pow(((c * gpc - 1) / (c * gmc + 1)), 2));

    return clamp01(1 - R);
}

vec3 sampleFromRdProfile(in sampler2D RdProfile, in float RdMaxArea,
                         in float RdMaxDistance, in float area,
                         in float distance) {
    float v = 1.0 - (area / RdMaxArea);
    float u = distance / RdMaxDistance;
    return texture(RdProfile, vec2(u, v)).rgb;
}

float QC1x2(float eta) {
    float eta2 = eta * eta, eta3 = eta2 * eta, eta4 = eta3 * eta,
          eta5 = eta4 * eta;
    if (eta < 1)
        return 0.919317 - 3.47930 * eta + 6.753350 * eta2 - 7.809890 * eta3 +
               4.985540 * eta4 - 1.368810 * eta5;
    else
        return -9.23372 + 22.2272 * eta - 20.9292 * eta2 + 10.2291 * eta3 -
               2.54396 * eta4 + 0.254913 * eta5;
}
float CPhi(float eta) {
    return 1.0 / 4. * (1 - QC1x2(eta));
}

vec3 computeFragmentEffect(in sampler2D RdProfile, in float RdMaxArea,
                           in float RdMaxDistance, in vec3 xo, in vec3 no,
                           in float area, in vec3 xi, in vec3 cameraPos,
                           in vec3 transmittedIrradiance) {
    vec3 v = normalize(cameraPos - xo);
    // outgoing fresnel term
    float fresnelTermXo = fresnelTransmittance(dot(no, v), eta);
    // incident fresnel term, assuming perpendicular incidence
    float fresnelTermXi = fresnelTransmittance(1, eta);

    vec3 Rd = sampleFromRdProfile(RdProfile, RdMaxArea, RdMaxDistance, area,
                                  length(xo - xi));
    return fresnelTermXo * fresnelTermXi * Rd * transmittedIrradiance * PI_INV *
           0.25 / CPhi(eta);
}
#endif /* HDSSS_SHADERS_INCLUDE_SUBSURFACE_HPP */
