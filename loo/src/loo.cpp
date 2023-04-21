#include "loo/loo.hpp"

#include <glog/logging.h>

namespace loo {
void initialize(const char* argv0) {
    FLAGS_logtostderr = 1;
    google::InitGoogleLogging(argv0);
}
}  // namespace loo