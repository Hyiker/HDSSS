#include "BSSRDF.hpp"
#include <gtest/gtest.h>
#include <cmath>
using namespace std;
using namespace glm;

constexpr double FLOAT_ERROR = 1e-5;

TEST(QC1x2Test, eta13) {
    EXPECT_NEAR(QC1x2(1.3), 0.445294669089997, FLOAT_ERROR);
}

TEST(PBDProfileTest, RegularValues) {
    dvec3 sigma_a = dvec3(0.0021, 0.0041, 0.0071);
    dvec3 sigma_s = dvec3(2.19, 2.62, 2.00);
    dvec3 sigma_t = sigma_a + sigma_s;
    const float rs[]{0.001, 0.01, 0.1, 1.0};
    const float expectedValues[]{
        1.563158833022761e+00,
        1.332657127862100e+00,
        4.527810272874374e-01,
        4.090240542620984e-02,
    };
    for (int i = 0; i < 1; i++) {
        dvec3 value = PBDProfile(sigma_a, sigma_t, 1.3, rs[i]);
        EXPECT_NEAR(value.r, expectedValues[i], FLOAT_ERROR) << "r = " << rs[i];
    }
}