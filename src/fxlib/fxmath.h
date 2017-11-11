#pragma once

#include <vector>

namespace fxlib {

struct fxrate_sample {
  double rate;    // Expected rate
  double period;  // Time of awaiting that rate, min
};

using fxrate_samples = std::vector<fxrate_sample>;

struct fxdensity_sample {
  double bound;  // Bound of rate
  size_t count;  // Number of samples that are match the rate bound
};

using fxrate_distribution = std::vector<fxdensity_sample>;

struct fxprobab_sample : fxdensity_sample {
  fxprobab_sample(double b, size_t c, double p) : fxdensity_sample({b, c}), prob(p) {}
  fxprobab_sample(double b, size_t c) : fxprobab_sample(b, c, 0) {}
  double prob;
};

using fxrate_probability = std::vector<fxprobab_sample>;

/// Calculate the mean and the variance values for sequence of samples.
void RateStats(const fxrate_samples& samples, double& mean, double& variance);

/// Build probability distribution for sequence of rate samples.
/**
  The sequence of rate samples will be sorted.
*/
fxrate_distribution RateDistribution(fxrate_samples& samples, size_t distr_size, const double rate_from, const double rate_step);

/// Build probability of samples rate.
fxrate_probability RateProbability(fxrate_samples& samples, size_t distr_size, const double rate_from, const double rate_step);

}  // namespace fxlib
