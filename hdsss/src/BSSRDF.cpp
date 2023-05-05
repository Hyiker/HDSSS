#include "BSSRDF.hpp"
#include <corecrt_math.h>
#include <corecrt_math_defines.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include "glm/detail/qualifier.hpp"
using namespace std;
using namespace glm;

BSSRDFTabulator::BSSRDFTabulator()
    : m_table(BSSRDF_TABLE_SIZE * BSSRDF_TABLE_SIZE) {}
constexpr double BSSRDF_TABLE_MIN_CONTRIBUTION = 1e-7;

dvec3 sourceFunction(dvec3 albedo, dvec3 sigmaT, double r) {
    return albedo * sigmaT * exp(-sigmaT * r);
}

double QC1x2(double eta) {
    double eta2 = eta * eta, eta3 = eta2 * eta, eta4 = eta3 * eta,
           eta5 = eta4 * eta;
    if (eta < 1)
        return 0.919317 - 3.47930 * eta + 6.753350 * eta2 - 7.809890 * eta3 +
               4.985540 * eta4 - 1.368810 * eta5;
    else
        return -9.23372 + 22.2272 * eta - 20.9292 * eta2 + 10.2291 * eta3 -
               2.54396 * eta4 + 0.254913 * eta5;
}
double QC2x3(double eta) {
    double eta2 = eta * eta, eta3 = eta2 * eta, eta4 = eta3 * eta,
           eta5 = eta4 * eta;
    if (eta < 1)
        return 0.828421 - 2.62051 * eta + 3.362310 * eta2 - 1.952840 * eta3 +
               0.236494 * eta4 + 0.145787 * eta5;
    else
        return -1641.1 + 135.926 / eta3 - 656.175 / eta2 + 1376.53 / eta +
               1213.67 * eta - 568.556 * eta2 + 164.798 * eta3 -
               27.0181 * eta4 + 1.91826 * eta5;
}
dvec3 linearstep(dvec3 edge0, dvec3 edge1, dvec3 t) {
    t = ((t - edge0) / (edge1 - edge0));
    t = max(min(t, dvec3(1)), dvec3(0));
    return t;
}

tuple<double, double> equiangular_sampling(double xi, double u0, double u1,
                                           double h) {

    h = std::max(h, 1e-12);
    double theta_min = atan(u0 / h);
    double theta_max = atan(u1 / h);

    double t = h * tan(lerp(theta_min, theta_max, xi));
    double pdf = h / ((theta_max - theta_min) * (pow(h, 2) + pow(t, 2)));
    return std::make_tuple(t, pdf);
}
tuple<double, double> exponential_sampling(double xi, dvec3 sigma_t) {
    double t = -log(1 - xi) / sigma_t.r;
    double pdf_t = sigma_t.r * exp(-sigma_t.r * t);
    return std::make_tuple(t, pdf_t);
}

double equiangular_sampling_pdfeval(double t, double u0, double u1, double h) {

    h = std::max(h, 1e-12);
    double theta_min = atan(u0 / h);
    double theta_max = atan(u1 / h);
    return h / ((theta_max - theta_min) * (pow(h, 2) + pow(t, 2)));
}

double exponential_sampling_pdfeval(double t, dvec3 sigma_t) {
    return sigma_t.r * exp(-sigma_t.r * t);
}

double PBDProfile(dvec3 sigma_a, dvec3 sigma_t, float eta, double r) {

    int num_samples_equi = 5;
    int num_samples_exp = 5;

    dvec3 blending_range_min = 0.9 * sigma_t;
    dvec3 blending_range_max = 1.1 * sigma_t;
    // set precision
    dvec3 sigma_s = sigma_t - sigma_a;

    dvec3 D_g =
        (2.0 * sigma_a + sigma_s) / (3.0 * pow(sigma_a + sigma_s, dvec3(2)));

    dvec3 sigma_tr = sqrt(sigma_a / D_g);
    dvec3 alpha = sigma_s / sigma_t;

    double A_boundary = (1 + QC2x3(eta)) / (1 - QC1x2(eta));
    dvec3 z_b = 2 * A_boundary * D_g;

    double C_phi = 1 / 4. * (1 - QC1x2(eta));
    double C_E = 1 / 2. * (1 - QC2x3(eta));

    double rn = 0.5;

    dvec3 weight = linearstep(blending_range_min, blending_range_max, dvec3(r));

    double KP_phi_equi = 0;
    double KP_E_equi = 0;

    auto PBDEvalSample = [=](double in_r, double t) {
        double zr = t;

        double dr = sqrt(in_r * in_r + t * t);
        double dv = sqrt(in_r * in_r + pow(t + pow(2, z_b.r), 2));

        double kappa = 1 - exp(-2 * sigma_t.r * (dr + t));

        double Q = alpha.r * sigma_t.r * exp(-sigma_t.r * t);

        double R_phi =
            alpha.r / (4 * M_PI) / D_g.r *
            (exp(-sigma_tr.r * dr) / dr - exp(-sigma_tr.r * dv) / dv);
        double R_E =
            alpha.r / (4 * M_PI) *
            (zr * (1 + sigma_tr.r * dr) * exp(-sigma_tr.r * dr) / pow(dr, 3) +
             (zr + 2 * z_b.r) * (1 + sigma_tr.r * dv) * exp(-sigma_tr.r * dv) /
                 pow(dv, 3));

        double S_phi = R_phi * Q * kappa;
        double S_E = R_E * Q * kappa;
        return std::make_tuple(S_phi, S_E);
    };

    if (r < blending_range_max.r) {
        for (int j_equi = 0; j_equi < num_samples_equi - 1; j_equi++) {

            double x = (j_equi + rn) / num_samples_equi;
            auto [t_equi, pdf_t_equi] = equiangular_sampling(
                x, 0, numeric_limits<double>::infinity(), r);
            auto [S_phi_equi, S_E_equi] = PBDEvalSample(r, t_equi);
            double pdf_t_exp = exponential_sampling_pdfeval(t_equi, sigma_t);

            dvec3 w_t_equi_MIS =
                (dvec3(1) - weight) * double(num_samples_equi) * pdf_t_equi /
                ((dvec3(1) - weight) * double(num_samples_equi) * pdf_t_equi +
                 weight * double(num_samples_exp) * pdf_t_exp);
            KP_phi_equi =
                KP_phi_equi + S_phi_equi * w_t_equi_MIS.r / pdf_t_equi;
            KP_E_equi = KP_E_equi + S_E_equi * w_t_equi_MIS.r / pdf_t_equi;
        }

        KP_phi_equi = KP_phi_equi / num_samples_equi;
        KP_E_equi = KP_E_equi / num_samples_equi;
    }

    double KP_phi_exp = 0;
    double KP_E_exp = 0;
    if (r > blending_range_min.r) {
        for (int j_exp = 0; j_exp < num_samples_exp; j_exp++) {

            double x = (j_exp + rn) / num_samples_exp;
            auto [t_exp, pdf_t_exp] = exponential_sampling(x, sigma_t);
            auto [S_phi_exp, S_E_exp] = PBDEvalSample(r, t_exp);
            double pdf_t_equi = equiangular_sampling_pdfeval(
                t_exp, 0, numeric_limits<double>::infinity(), r);

            double w_t_exp_MIS =
                weight.r * num_samples_exp * pdf_t_exp /
                ((1 - weight.r) * num_samples_equi * pdf_t_equi +
                 weight.r * num_samples_exp * pdf_t_exp);

            KP_phi_exp = KP_phi_exp + S_phi_exp * w_t_exp_MIS / pdf_t_exp;
            KP_E_exp = KP_E_exp + S_E_exp * w_t_exp_MIS / pdf_t_exp;
        }

        KP_phi_exp = KP_phi_exp / num_samples_exp;
        KP_E_exp = KP_E_exp / num_samples_exp;
    }
    return C_phi * (KP_phi_equi + KP_phi_exp) + C_E * (KP_E_equi + KP_E_exp);
}
dvec3 interpolate(dvec3 v0, dvec3 v1, double t) {
    return v0 * (1 - t) + v1 * t;
}
void BSSRDFTabulator::tabulate(const PBRMetallicMaterial& material) {
    const auto shaderMaterial = material.getShaderMaterial();
    dvec3 sigmaA(shaderMaterial.sigmaARoughness.r,
                 shaderMaterial.sigmaARoughness.g,
                 shaderMaterial.sigmaARoughness.b),
        sigmaT(shaderMaterial.transmissionSigmaT.g,
               shaderMaterial.transmissionSigmaT.b,
               shaderMaterial.transmissionSigmaT.a),
        albedo(shaderMaterial.baseColorMetallic.r,
               shaderMaterial.baseColorMetallic.g,
               shaderMaterial.baseColorMetallic.b);
    double rLeft = 0, rRight = 100, rMax = (rLeft + rRight) / 2.0;
    dvec3 Qr;
    double QrMax;
    // binary search for rMax
    while (rRight - rLeft > 1e-3) {
        rMax = (rLeft + rRight) / 2.0;
        Qr = sourceFunction(albedo, sigmaT, rMax);
        QrMax = std::max(Qr.r, std::max(Qr.g, Qr.b));
        if (QrMax < BSSRDF_TABLE_MIN_CONTRIBUTION) {
            rRight = rMax;
        } else {
            rLeft = rMax;
        }
    }
    double Amax = M_PI * rMax * rMax;
    double eta = 1.3;
    maxArea = Amax;
    maxDistance = rMax;
    LOG(INFO) << "rMax: " << rMax << " Amax: " << Amax;
    // step1: precompute the Rd profile
    LOG(INFO) << "Precomputing Rd profile...";
    dvec3 RdProfile[BSSRDF_TABLE_SIZE];
    double deltaX = rMax / (BSSRDF_TABLE_SIZE - 1), xi = 0;
    for (int y = 0; y < BSSRDF_TABLE_SIZE; y++) {
        double Rd = PBDProfile(sigmaA, sigmaT, eta, xi);
        RdProfile[y] = dvec3(Rd);
        xi += deltaX;
    }
    auto interpolateRd = [=](double xi) {
        double x = xi / rMax * (BSSRDF_TABLE_SIZE - 1);
        x = std::max(x, 1e-6);
        int x0 = std::floor(x), x1 = std::ceil(x);
        double t = x - x0;
        return interpolate(RdProfile[x0], RdProfile[x1], t);
    };
    // step2: precompute the Rd' integral
    LOG(INFO) << "Precomputing Rd' integral...";
    for (int y = 0; y < BSSRDF_TABLE_SIZE; y++) {
        // use y for A
        double A = Amax * y / BSSRDF_TABLE_SIZE;
        A = std::max(A, 1e-6);
        double rA = sqrt(A * M_1_PI);
        for (int x = 0; x < BSSRDF_TABLE_SIZE; x++) {
            // use x for r
            double r = rMax * x / (BSSRDF_TABLE_SIZE - 1);
            r = std::max(r, 1e-6);
            double xi = std::max(0.0, rA - rMax), xEnd = rA + r;
            dvec3 value(0.0);
            while (xi < xEnd) {
                double xi1 = xi + deltaX;
                double bi = 2 * xi *
                            acos(std::clamp(
                                (xi * xi - rA * rA + r * r) / (2.0 * r * xi),
                                -1.0, 1.0)),
                       bi1 = 2 * xi1 *
                             acos(std::clamp((xi1 * xi1 - rA * rA + r * r) /
                                                 (2.0 * r * xi1),
                                             -1.0, 1.0));
                dvec3 arcRdIntegral =
                    interpolateRd(xi) * bi + interpolateRd(xi1) * bi1;
                arcRdIntegral *= deltaX;
                value += arcRdIntegral;
                xi = xi1;
            }
            value /= A;
            m_table[y * BSSRDF_TABLE_SIZE + x] = value;
        }
        LOG(INFO) << "y: " << y;
    }
}

void BSSRDFTabulator::read(const std::string& filename) {
    ifstream ifs(filename);
    ifs >> maxDistance >> maxArea;
    for (int y = 0; y < BSSRDF_TABLE_SIZE; y++) {
        for (int x = 0; x < BSSRDF_TABLE_SIZE; x++) {
            vec3 buf;
            ifs >> buf.r >> buf.g >> buf.b;
            m_table[y * BSSRDF_TABLE_SIZE + x] = buf;
        }
    }
    ifs.close();
}

void BSSRDFTabulator::save(const std::string& filename) {
    ofstream ofs(filename);
    ofs << maxDistance << " " << maxArea << endl;
    for (int y = 0; y < BSSRDF_TABLE_SIZE; y++) {
        for (int x = 0; x < BSSRDF_TABLE_SIZE; x++) {
            auto value = m_table[y * BSSRDF_TABLE_SIZE + x];
            ofs << value.r << " " << value.g << " " << value.b << " ";
        }
        ofs << endl;
    }
    ofs.close();
}
unique_ptr<loo::Texture2D> BSSRDFTabulator::generateTexture() {
    auto tex = make_unique<loo::Texture2D>();
    tex->init();
    tex->setup(m_table.data(), BSSRDF_TABLE_SIZE, BSSRDF_TABLE_SIZE, GL_RGB32F,
               GL_RGB, GL_FLOAT, 1);
    tex->setSizeFilter(GL_LINEAR, GL_LINEAR);
    tex->setWrapFilter(GL_CLAMP_TO_EDGE);
    return tex;
}
