#ifndef HDSSS_INCLUDE_BSSRDF_HPP
#define HDSSS_INCLUDE_BSSRDF_HPP
#include <memory>
#include "PBRMaterials.hpp"

constexpr int BSSRDF_TABLE_SIZE = 512;

// compute the BSSRDF table
// parameters: distance, integrated area

class BSSRDFTabulator {
   private:
    std::vector<glm::vec3> m_table{};

   public:
    double maxDistance = 0, maxArea = 0;
    BSSRDFTabulator();
    void tabulate(const PBRMetallicMaterial& material);
    void read(const std::string& filename);
    void save(const std::string& filename);
    std::unique_ptr<loo::Texture2D> generateTexture();
};

double QC1x2(double eta);

double QC2x3(double eta);

glm::dvec3 PBDProfile(glm::dvec3 sigma_a, glm::dvec3 sigma_t, double eta,
                      double r);

#endif /* HDSSS_INCLUDE_BSSRDF_HPP */
