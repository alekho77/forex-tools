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

}  // namespace fxlib
