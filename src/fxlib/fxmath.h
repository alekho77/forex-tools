#pragma once

#include <vector>

namespace fxlib {

struct fxrate_sample {
  double rate;    // Expected rate
  double period;  // Time of awaiting that rate, min
};

using fxrate_samples = std::vector<fxrate_sample>;

struct fxdensity_range {
  double rate_low;  // Lower bound of rate range
  double rate_up;   // Upper bound of rate range
  size_t count;     // Number of samples that are match the rate range
};

using fxrate_distribution = std::vector<fxdensity_range>;

void RateStats(const fxrate_samples& samples, double& mean, double& variance);
fxrate_distribution BuildDistribution(const fxrate_samples& samples, size_t distr_size, const double rate_from, const double rate_step);

}  // namespace fxlib
