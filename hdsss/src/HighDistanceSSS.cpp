#include "HighDistanceSSS.hpp"

#include <algorithm>
#include <loo/Camera.hpp>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

#include "glog/logging.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext.hpp"
using namespace loo;
using namespace std;
using namespace glm;
HighDistanceSSS::HighDistanceSSS(int width, int height)
    : m_width(width), m_height(height) {}